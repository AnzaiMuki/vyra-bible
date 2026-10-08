#!/usr/bin/env python3
"""Timing of the overlay page (not a pass/fail test): how long vyraShow takes, synchronously, to fit the text.

Usage: python3 tests/overlay_bench.py
Prints milliseconds. Depends on the machine; compare numbers on the same machine only.
"""
import importlib.util
import pathlib
import statistics

from playwright.sync_api import sync_playwright

HERE = pathlib.Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("opt", HERE / "overlay_page_test.py")
opt = importlib.util.module_from_spec(spec)
spec.loader.exec_module(opt)  # only defines helpers: main() is not called

# Each call uses a different reference: the page ignores a frame identical to the one shown, which would measure nothing.
def run(page, label, make_frames, repeat=15):
    times = []
    for i in range(repeat):
        for k, f in enumerate(make_frames(i)):
            t = page.evaluate("f => { const t0 = performance.now(); window.vyraShow(f); return performance.now() - t0; }", f)
            times.append(t)
        page.wait_for_timeout(450)  # let the crossfade end, as between two real changes
    print(f"{label:<44} median {statistics.median(times):6.2f} ms   worst {max(times):6.2f} ms   (n={len(times)})")

ps = opt.load_verses(19, 119)
jn = opt.load_verses(43, 3)

with sync_playwright() as p:
    browser = p.chromium.launch(args=["--allow-file-access-from-files"])
    page = browser.new_page(viewport={"width": 1920, "height": 1080})
    page.goto(opt.PAGE)
    page.wait_for_function("typeof window.vyraShow === 'function'")
    rev = [0]
    def fr(ref, verses, theme="lower", page_no=1, pages=1):
        rev[0] += 1
        return opt.frame(rev[0], True, ref, verses, theme, page_no, pages)
    run(page, "one verse, lower third", lambda i: [fr(f"Jean 3:16 #{i}", [jn[15]])])
    run(page, "full page of verses (Ps 119:1-12), full screen", lambda i: [fr(f"Psaumes 119 #{i}", ps[:12], "full", 1, 15)])
    run(page, "long page (Ps 119:1-4), minimal", lambda i: [fr(f"Psaumes 119 #{i}", ps[:4], "minimal", 1, 40)])
    browser.close()
