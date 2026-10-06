#!/usr/bin/env python3
"""End to end: a real Chromium on the page served by the real server (Server-Sent Events included).

Usage: python3 tests/overlay_e2e_test.py <path to overlay_demo> [screenshot_dir]
"""
import pathlib
import subprocess
import sys

from playwright.sync_api import sync_playwright

demo_path = sys.argv[1]
OUT = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else None
failures = checks = 0


def check(cond, what):
    global failures, checks
    checks += 1
    if not cond:
        failures += 1
        print("FAIL", what)


demo = subprocess.Popen([demo_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)


def command(line):
    demo.stdin.write(line + "\n")
    demo.stdin.flush()
    check(demo.stdout.readline().strip() == "OK", "command accepted: " + line)


ready = demo.stdout.readline().split()
check(ready[:1] == ["READY"], "server started")
port = int(ready[1])
url = "http://127.0.0.1:%d/" % port

try:
    with sync_playwright() as p:
        browser = p.chromium.launch()
        page = browser.new_page(viewport={"width": 1920, "height": 1080})
        errors = []
        page.on("pageerror", lambda e: errors.append(str(e)))
        page.goto(url)

        def opacity():
            return float(page.evaluate("getComputedStyle(document.getElementById('panel')).opacity"))

        def texts():
            return page.evaluate("Array.from(document.querySelectorAll('.slide.shown .text')).map(e => e.textContent)")

        def ref():
            return page.evaluate("(document.querySelector('.slide.shown .ref') || {}).textContent || ''")

        page.wait_for_timeout(500)
        check(opacity() == 0, "nothing on the air at the start")

        # Preview alone must not reach the screen.
        command("preview Jn 3:16")
        page.wait_for_timeout(500)
        check(opacity() == 0, "preview is not broadcast")

        command("air")
        page.wait_for_function("document.querySelector('.slide.shown .ref')", timeout=3000)
        page.wait_for_timeout(400)
        check(opacity() == 1 and ref().lower() == "jean 3:16", "ON AIR reaches the page: %r" % ref())
        check(texts() and texts()[0].startswith("Car Dieu a tant aimé le monde"), "the verse text arrives intact")

        # Changing the preview does not change the screen; ON AIR does.
        command("preview Ps 23:1")
        command("next")
        page.wait_for_timeout(500)
        check(ref().lower() == "jean 3:16", "moving the preview never touches the live screen")
        command("air")
        page.wait_for_function("document.querySelector('.slide.shown .ref').textContent.toLowerCase().includes('psaumes')", timeout=3000)
        page.wait_for_timeout(400)
        check(ref().lower() == "psaumes 23:2", "new passage after ON AIR: %r" % ref())
        if OUT:
            page.screenshot(path=str(OUT / "e2e_live.png"), omit_background=True)

        # Hide, then put back.
        command("hide")
        page.wait_for_timeout(700)
        check(opacity() == 0, "Hide empties the screen")
        command("air")
        page.wait_for_timeout(700)
        check(opacity() == 1 and ref().lower() == "psaumes 23:2", "ON AIR again shows the held passage")

        # A page opened while a passage is on the air shows it at once.
        late = browser.new_page(viewport={"width": 1280, "height": 720})
        late.goto(url)
        late.wait_for_function("document.querySelector('.slide.shown .ref')", timeout=3000)
        check("23:2" in late.evaluate("document.querySelector('.slide.shown .ref').textContent"),
              "a late page is up to date as soon as it connects")
        late.close()

        check(not errors, "no JavaScript error: %s" % errors)
        browser.close()
finally:
    try:
        demo.stdin.write("quit\n")
        demo.stdin.flush()
    except Exception:
        pass
    demo.wait(timeout=5)

print("%d checks, %d failure(s)" % (checks, failures))
sys.exit(1 if failures else 0)
