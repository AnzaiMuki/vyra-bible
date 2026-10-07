#!/usr/bin/env python3
"""Tests of the overlay page (data/overlay) in a real Chromium, without the plugin.

The page is opened with ?manual=1 (no connection to a server) and frames are fed by hand through
window.vyraShow. Needs: python3, playwright and a Chromium (PLAYWRIGHT_BROWSERS_PATH).
Usage: python3 tests/overlay_page_test.py [screenshot_dir] [path to frames_dump]
(the second argument adds the test of the fullest pages of the whole Bible, for every theme).
"""
import tempfile
import io
import json
import pathlib
import struct
import sys
import zlib

from playwright.sync_api import sync_playwright

ROOT = pathlib.Path(__file__).resolve().parent.parent
PAGE = (ROOT / "data" / "overlay" / "index.html").as_uri() + "?manual=1"
OUT = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 and sys.argv[1] else None
DUMP = sys.argv[2] if len(sys.argv) > 2 else None

failures = 0
checks = 0


def check(cond, what):
    global failures, checks
    checks += 1
    if not cond:
        failures += 1
        print("FAIL", what)


def load_verses(book, chapter):
    verses = []
    for line in (ROOT / "data" / "bibles" / "lsg1910.tsv").read_text(encoding="utf-8").splitlines():
        if line.startswith("#"):
            continue
        b, c, v, t = line.split("\t", 3)
        if int(b) == book and int(c) == chapter:
            verses.append({"c": int(c), "v": int(v), "t": t})
    return verses


def frame(rev, visible, ref="", verses=(), theme="lower", page=1, pages=1):
    return {"rev": rev, "visible": visible, "theme": theme, "page": page, "pages": pages, "reference": ref,
            "verses": list(verses)}


def png_pixels(data):
    """Decodes an 8-bit RGBA PNG (what Chromium writes) into rows of bytes. Minimal, for the checks below."""
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    pos, idat, width, height = 8, b"", 0, 0
    while pos < len(data):
        (length,) = struct.unpack(">I", data[pos:pos + 4])
        kind = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            width, height, depth, ctype = struct.unpack(">IIBB", body[:10])
            assert depth == 8 and ctype == 6, "expected 8-bit RGBA"
        elif kind == b"IDAT":
            idat += body
        pos += 12 + length
    raw = zlib.decompress(idat)
    stride = width * 4
    rows, prev = [], bytearray(stride)
    i = 0
    for _ in range(height):
        f = raw[i]
        line = bytearray(raw[i + 1:i + 1 + stride])
        i += 1 + stride
        for x in range(stride):
            a = line[x - 4] if x >= 4 else 0
            b = prev[x]
            c = prev[x - 4] if x >= 4 else 0
            if f == 1:
                line[x] = (line[x] + a) & 255
            elif f == 2:
                line[x] = (line[x] + b) & 255
            elif f == 3:
                line[x] = (line[x] + (a + b) // 2) & 255
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[x] = (line[x] + pr) & 255
        rows.append(line)
        prev = line
    return width, height, rows


def alpha_at(rows, x, y):
    return rows[y][x * 4 + 3]


def main():
    jn316 = load_verses(43, 3)[15]
    ps119 = load_verses(19, 119)
    check(jn316["v"] == 16 and ps119[0]["v"] == 1 and len(ps119) == 176, "test data loaded")

    with sync_playwright() as p:
        browser = p.chromium.launch(args=["--allow-file-access-from-files"])
        page = browser.new_page(viewport={"width": 1920, "height": 1080})
        errors = []
        page.on("pageerror", lambda e: errors.append(str(e)))
        page.on("console", lambda m: errors.append(m.text) if m.type == "error" else None)
        page.goto(PAGE)
        page.wait_for_function("typeof window.vyraShow === 'function'")

        def panel_opacity():
            return float(page.evaluate("getComputedStyle(document.getElementById('panel')).opacity"))

        def shown_text():
            return page.evaluate("Array.from(document.querySelectorAll('.slide.shown .text')).map(e => e.textContent)")

        def settle():
            page.wait_for_timeout(500)

        # 1. Nothing on the air at the start.
        check(panel_opacity() == 0, "panel invisible before any frame")

        # 2. One verse on the air.
        page.evaluate("f => window.vyraShow(f)", frame(1, True, "Jean 3:16", [jn316]))
        settle()
        check(panel_opacity() == 1, "panel visible when on the air")
        check(shown_text() == [jn316["t"]], "the verse text is displayed unchanged")
        check(page.evaluate("document.querySelector('.slide.shown .ref').textContent") == "Jean 3:16", "reference displayed")
        check(page.evaluate("document.querySelector('.slide.shown .text sup')") is None, "single verse has no number")

        # 3. Transparency: screenshot with a transparent default background.
        shot = page.screenshot(omit_background=True)
        w, h, rows = png_pixels(shot)
        check((w, h) == (1920, 1080), "screenshot size")
        check(alpha_at(rows, 5, 5) == 0 and alpha_at(rows, w - 5, 5) == 0 and alpha_at(rows, w // 2, 100) == 0,
              "background is fully transparent")
        box = page.evaluate("(() => { const r = document.getElementById('panel').getBoundingClientRect(); return [r.left, r.top, r.width, r.height]; })()")
        px, py = int(box[0] + box[2] / 2), int(box[1] + 4)
        check(alpha_at(rows, px, py) >= 200, "panel itself is nearly opaque")
        check(box[1] > h * 0.5 and box[1] + box[3] < h, "lower third: panel in the bottom half, inside the screen")
        if OUT:
            (OUT / "overlay_single.png").write_bytes(shot)

        # 4. Changing verse: crossfade, never empty, never two verses left behind.
        v17 = {"c": 3, "v": 17, "t": "Dieu, en effet, n'a pas envoyé son Fils dans le monde pour qu'il juge le monde."}
        page.evaluate("f => window.vyraShow(f)", frame(2, True, "Jean 3:17", [v17]))
        page.wait_for_timeout(120)
        check(panel_opacity() > 0.5, "panel does not blink out while the verse changes")
        check(page.evaluate("document.querySelectorAll('.slide').length") == 2, "old and new slide coexist during the fade")
        settle()
        check(shown_text() == [v17["t"]], "new verse displayed")
        check(page.evaluate("document.querySelectorAll('.slide').length") == 1, "old slide removed after the fade")

        # 5. Stale frame is ignored; same-revision too.
        page.evaluate("f => window.vyraShow(f)", frame(1, True, "Jean 3:16", [jn316]))
        settle()
        check(shown_text() == [v17["t"]], "an older frame does not replace a newer one")

        # 6. A range shows verse numbers, and the text fits the room.
        rng = load_verses(43, 3)[15:18]
        page.evaluate("f => window.vyraShow(f)", frame(3, True, "Jean 3:16–18", rng))
        settle()
        check(page.evaluate("document.querySelectorAll('.slide.shown .text sup').length") == 3, "numbers on a range")

        def fits():
            return page.evaluate("(() => { const t = document.querySelector('.slide.shown .text'); return t.scrollHeight <= t.clientHeight + 1; })()")

        check(fits(), "range fits")
        if OUT:
            (OUT / "overlay_range.png").write_bytes(page.screenshot(omit_background=True))

        # 7. Safety net: if a frame is too long anyway (should not happen, pages are cut to fit), whole verses
        #    are dropped and "..." is added; it still fits, and the slide says so.
        page.evaluate("f => window.vyraShow(f)", frame(4, True, "Psaumes 119", ps119))
        settle()
        check(fits(), "very long passage fits")
        txt = shown_text()[0]
        check(txt.endswith("…"), "very long passage is marked as cut")
        shown_verses = page.evaluate("document.querySelectorAll('.slide.shown .text sup').length")
        check(1 <= shown_verses < 176, "whole verses were dropped, not a line cut (%d shown)" % shown_verses)
        last = ps119[shown_verses - 1]["t"]
        check(last in txt, "the last kept verse is complete")
        if OUT:
            (OUT / "overlay_long.png").write_bytes(page.screenshot(omit_background=True))

        # 8. Text that looks like HTML stays text.
        evil = {"c": 1, "v": 1, "t": "<img src=x onerror=\"window.__pwned=1\"><script>window.__pwned=1</script>"}
        page.evaluate("f => window.vyraShow(f)", frame(5, True, "<b>x</b>", [evil]))
        settle()
        check(page.evaluate("window.__pwned") is None, "no markup in the text is ever executed")
        check(shown_text() == [evil["t"]], "markup is displayed as plain text")
        check(page.evaluate("document.querySelector('.slide.shown .ref').textContent") == "<b>x</b>", "reference too")

        # 9. Hide: fades out, then empties.
        page.evaluate("f => window.vyraShow(f)", frame(6, False))
        settle()
        check(panel_opacity() == 0, "panel invisible when hidden")
        check(page.evaluate("document.querySelectorAll('.slide').length") == 0, "hidden panel holds no stale text")
        w, h, rows = png_pixels(page.screenshot(omit_background=True))
        check(all(alpha_at(rows, x, y) == 0 for x in range(0, w, 97) for y in range(0, h, 89)),
              "hidden: every sampled pixel is transparent")

        # 10. Back on the air after a hide, with the same content as before: it must show again.
        page.evaluate("f => window.vyraShow(f)", frame(7, True, "Jean 3:16", [jn316]))
        settle()
        check(panel_opacity() == 1 and shown_text() == [jn316["t"]], "shown again after a hide")
        # Hide then show quickly: the late cleanup of the hide must not erase the new slide.
        page.evaluate("f => window.vyraShow(f)", frame(8, False))
        page.evaluate("f => window.vyraShow(f)", frame(9, True, "Jean 3:16", [jn316]))
        settle()
        check(panel_opacity() == 1 and shown_text() == [jn316["t"]], "quick hide-then-show keeps the slide")

        # 11. Different viewport sizes: still a lower third that fits.
        for (vw, vh) in ((1280, 720), (3840, 2160), (1080, 1920)):
            page.set_viewport_size({"width": vw, "height": vh})
            page.wait_for_timeout(200)
            page.evaluate("f => window.vyraShow(f)", frame(10 + vw, True, "Psaumes 119", ps119[:12]))
            settle()
            check(fits(), "fits at %dx%d" % (vw, vh))
            r = page.evaluate("(() => { const r = document.getElementById('panel').getBoundingClientRect(); return [r.left, r.right, r.bottom]; })()")
            check(r[0] >= 0 and r[1] <= vw and r[2] <= vh, "panel inside the screen at %dx%d" % (vw, vh))

        # 12. Resizing while on the air fits the text again, with no blink.
        page.set_viewport_size({"width": 1920, "height": 1080})
        page.wait_for_timeout(200)
        page.evaluate("f => window.vyraShow(f)", frame(900, True, "Psaumes 119", ps119[:12]))
        settle()
        page.set_viewport_size({"width": 1080, "height": 1920})
        page.wait_for_timeout(400)
        check(fits(), "after a resize while on the air, the text fits again")
        check(panel_opacity() == 1, "panel stays visible during a resize")

        # 13. Themes: one panel geometry each, all inside the screen, text fits.
        page.set_viewport_size({"width": 1920, "height": 1080})
        page.wait_for_timeout(200)
        v16 = jn316
        geometry = {}
        rev = 12000
        for theme in ("lower", "full", "minimal"):
            rev += 1
            page.evaluate("f => window.vyraShow(f)", frame(rev, True, "Jean 3:16", [v16], theme=theme))
            page.wait_for_timeout(900)
            check(page.evaluate("document.getElementById('panel').className.indexOf('theme-%s') >= 0" % theme),
                  "theme class applied: " + theme)
            check(panel_opacity() == 1 and fits() and shown_text() == [v16["t"]], "theme shows the verse: " + theme)
            box = page.evaluate("(() => { const r = document.getElementById('panel').getBoundingClientRect(); return [r.left, r.top, r.right, r.bottom]; })()")
            geometry[theme] = box
            check(box[0] >= 0 and box[1] >= 0 and box[2] <= 1920 and box[3] <= 1080, "panel inside the screen: " + theme)
            shot = page.screenshot(omit_background=True)
            w, h, rows = png_pixels(shot)
            check(alpha_at(rows, 3, 3) == (255 if theme == "full" else 0) or theme == "full" and alpha_at(rows, 3, 3) >= 200,
                  "corner of the screen: opaque only in full screen (%s)" % theme)
            if OUT:
                (OUT / ("overlay_theme_%s.png" % theme)).write_bytes(shot)
        check(geometry["full"][0] == 0 and geometry["full"][3] == 1080, "full screen covers the whole picture")
        check(geometry["lower"][1] > 540 and geometry["minimal"][1] > 540, "lower and minimal stay in the bottom half")

        # A change of theme on the air: the panel fades out, changes, comes back; no blink to the old geometry.
        page.evaluate("f => window.vyraShow(f)", frame(12100, True, "Jean 3:16", [v16], theme="lower"))
        page.wait_for_timeout(900)
        page.evaluate("f => window.vyraShow(f)", frame(12101, True, "Jean 3:16", [v16], theme="full"))
        page.wait_for_timeout(100)
        check(panel_opacity() < 1, "theme change: the panel is fading out")
        # newer frames arriving during the switch are not lost
        page.evaluate("f => window.vyraShow(f)", frame(12102, True, "Jean 3:17", [v17], theme="full"))
        page.wait_for_timeout(1000)
        check(panel_opacity() == 1 and shown_text() == [v17["t"]], "theme change: the newest frame is the one shown")
        check(page.evaluate("document.getElementById('panel').className") == "theme-full on" or
              "theme-full" in page.evaluate("document.getElementById('panel').className"), "theme change applied")
        check(page.evaluate("document.querySelectorAll('.slide').length") == 1, "theme change: one slide only")
        # an unknown theme falls back to the lower third
        page.evaluate("f => window.vyraShow(f)", frame(12103, False))
        page.wait_for_timeout(700)
        page.evaluate("f => window.vyraShow(f)", frame(12104, True, "Jean 3:16", [v16], theme="inconnu"))
        page.wait_for_timeout(700)
        check("theme-lower" in page.evaluate("document.getElementById('panel').className"), "unknown theme: lower third")

        # 14. Pages: indicator, numbers, continuation pieces without number.
        piece1 = {"c": 1, "v": 5, "t": "Debut du verset coupe"}
        piece2 = {"c": 1, "v": 5, "t": "suite du verset coupe", "k": True}
        page.evaluate("f => window.vyraShow(f)", frame(12200, True, "Psaumes 119", [piece1], page=1, pages=3))
        page.wait_for_timeout(700)
        check(page.evaluate("document.querySelector('.slide.shown .ref .page').textContent") == "1/3", "page indicator 1/3")
        check(page.evaluate("document.querySelectorAll('.slide.shown .text sup').length") == 1, "first piece keeps its number")
        page.evaluate("f => window.vyraShow(f)", frame(12201, True, "Psaumes 119", [piece2], page=2, pages=3))
        page.wait_for_timeout(700)
        check(page.evaluate("document.querySelector('.slide.shown .ref .page').textContent") == "2/3", "page indicator 2/3")
        check(page.evaluate("document.querySelectorAll('.slide.shown .text sup').length") == 0,
              "a continuation has no verse number")
        page.evaluate("f => window.vyraShow(f)", frame(12202, True, "Jean 3:16", [v16]))
        page.wait_for_timeout(700)
        check(page.evaluate("document.querySelector('.slide.shown .ref .page')") is None, "no indicator for one page")

        # 15. The fullest pages of the whole Bible, for every theme, must fit without any truncation.
        if DUMP:
            frames_file = pathlib.Path(tempfile.mkdtemp()) / "frames.jsonl"
            import subprocess
            subprocess.run([DUMP, str(frames_file)], check=True, capture_output=True)
            frames = [json.loads(l) for l in frames_file.read_text(encoding="utf-8").splitlines()]
            check(len(frames) > 300, "frames to render: %d" % len(frames))
            bad = []
            smallest = {}
            for size in ((1920, 1080), (1280, 720)):
                page.set_viewport_size({"width": size[0], "height": size[1]})
                page.wait_for_timeout(150)
                rev = 1000000 * size[0]
                for fr in frames:
                    rev += 1
                    fr = dict(fr, rev=rev)
                    result = page.evaluate("""f => {
                        document.getElementById('panel').classList.remove('on');
                        window.vyraShow(f);
                        return null;
                    }""", fr)
                    page.wait_for_timeout(1 if fr["theme"] == "lower" else 1)
                    # layout is measured at once by the page itself; read what it decided
                    info = page.evaluate("""() => {
                        const slides = Array.from(document.querySelectorAll('.slide'));
                        const s = slides[slides.length - 1];
                        if (!s) return null;
                        const t = s.querySelector('.text');
                        return {truncated: s.getAttribute('data-truncated'), fits: t.scrollHeight <= t.clientHeight + 1,
                                font: parseFloat(t.style.fontSize)};
                    }""")
                    if info:
                        smallest[fr["theme"]] = min(smallest.get(fr["theme"], 99), info["font"])
                    if not info or info["truncated"] or not info["fits"]:
                        bad.append((size, fr["theme"], fr["reference"], fr["page"], info))
            print("smallest font used on the fullest pages (vh of the screen):", {k: round(v, 2) for k, v in smallest.items()})
            check(not bad, "pages that do not fit without truncation: %d, first: %s" % (len(bad), bad[:2]))

        check(not errors, "no JavaScript error: %s" % errors)
        browser.close()

    print("%d checks, %d failure(s)" % (checks, failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
