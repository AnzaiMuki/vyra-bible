#!/usr/bin/env python3
"""Convert a USFM Bible (one .sfm file per book) to the VYRA module format (TSV).

Only the verse text is kept. Introductions, headings, cross references, footnotes
and formatting markers are dropped.

Usage:
    usfm_to_vyra_tsv.py <usfm_dir> <output.tsv> --code LSG1910 --name "Louis Segond 1910" \
        --language fr --license "Public Domain" --source "<where the text comes from>"

Output format ("VYRA module", version 1), UTF-8, LF line endings:
    #vyra-module<TAB>1
    #code<TAB>...        (header lines start with '#', key then value)
    <book><TAB><chapter><TAB><verse><TAB><text>
where <book> is the canonical number 1..66 (Genesis=1 ... Revelation=66).
"""
import argparse
import re
import sys
from pathlib import Path

# USFM book ids in canonical order: index + 1 = book number.
BOOK_IDS = (
    "GEN EXO LEV NUM DEU JOS JDG RUT 1SA 2SA 1KI 2KI 1CH 2CH EZR NEH EST JOB PSA PRO ECC SNG "
    "ISA JER LAM EZK DAN HOS JOL AMO OBA JON MIC NAM HAB ZEP HAG ZEC MAL "
    "MAT MRK LUK JHN ACT ROM 1CO 2CO GAL EPH PHP COL 1TH 2TH 1TI 2TI TIT PHM HEB JAS 1PE 2PE 1JN 2JN 3JN JUD REV"
).split()
assert len(BOOK_IDS) == 66

# Paragraph / poetry markers whose text belongs to the current verse. Verified on the whole
# Segond 1910 source: inside chapters only \\v \\q \\p \\m \\pi carry verse text.
TEXT_MARKERS = {"p", "m", "pi", "pi1", "pi2", "mi", "nb", "li", "li1", "li2", "q", "q1", "q2", "q3", "q4", "qm", "qm1", "qm2", "qr", "qc"}

RE_NOTE = re.compile(r"\\(x|f|fe)\s.*?\\\1\*", re.S)          # cross references and footnotes
RE_CHAR = re.compile(r"\\\+?[a-z][a-z0-9]*\*?")                  # remaining inline markers (\wj, \wj*, \add ...)
RE_LINE = re.compile(r"^\\([a-z0-9]+)\s*(.*)$")

# Known typos of the source files. Each fix must match exactly once, otherwise the script stops:
# a silent mismatch would mean the source changed and the fix is no longer right.
ERRATA = {
    # Psalm 119:115 is written "\v 11 5" (a stray space inside the verse number).
    "PSA": [("\\v 11 5\u00c9loignez-vous", "\\v 115 \u00c9loignez-vous")],
}


def clean(text: str) -> str:
    text = RE_NOTE.sub("", text)
    text = RE_CHAR.sub("", text)
    return re.sub(r"\s+", " ", text).strip()


def parse_book(path: Path, book: int, bid: str):
    verses = []          # (chapter, verse, text)
    chapter = 0
    cur = None           # [chapter, verse, [parts]]
    source = path.read_text(encoding="utf-8")
    for old, new in ERRATA.get(bid, []):
        if source.count(old) != 1:
            raise ValueError(f"{bid}: erratum {old!r} matches {source.count(old)} times (expected 1)")
        source = source.replace(old, new)
    for raw in source.splitlines():
        m = RE_LINE.match(raw.strip())
        if not m:
            if cur is not None and raw.strip():
                cur[2].append(raw.strip())
            continue
        marker, rest = m.group(1), m.group(2)
        if marker == "c":
            if cur:
                verses.append((cur[0], cur[1], cur[2]))
            cur = None
            chapter = int(rest.split()[0])
        elif marker == "v":
            if cur:
                verses.append((cur[0], cur[1], cur[2]))
            vm = re.match(r"(\d+)(?:-(\d+))?\s*(.*)$", rest, re.S)
            if not vm:
                raise ValueError(f"{path.name}: bad verse line: {raw!r}")
            if vm.group(2):
                raise ValueError(f"{path.name}: verse bridge not supported: {raw!r}")
            cur = [chapter, int(vm.group(1)), [vm.group(3)]]
        elif marker in TEXT_MARKERS:
            if cur is not None and rest:
                cur[2].append(rest)
        # Any other marker (headings \\s \\ms, parallel references \\r \\mr, blank line \\b, ...)
        # carries no verse text. It must NOT close the verse: in the Psalms the first line of the
        # text comes after a "\\b" that follows the numbered title ("\\v 1 Cantique de David.").
    if cur:
        verses.append((cur[0], cur[1], cur[2]))
    return [(c, v, clean(" ".join(parts))) for c, v, parts in verses]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("usfm_dir")
    ap.add_argument("output")
    ap.add_argument("--code", required=True)
    ap.add_argument("--name", required=True)
    ap.add_argument("--language", required=True)
    ap.add_argument("--license", required=True)
    ap.add_argument("--source", required=True)
    args = ap.parse_args()

    files = {}
    for p in Path(args.usfm_dir).glob("*.sfm"):
        m = re.search(r"-\d\d-([A-Z0-9]{3})\.", p.name)
        if m:
            files[m.group(1)] = p
    missing = [b for b in BOOK_IDS if b not in files]
    if missing:
        print(f"missing books: {missing}", file=sys.stderr)
        return 1

    lines = [
        "#vyra-module\t1",
        f"#code\t{args.code}",
        f"#name\t{args.name}",
        f"#language\t{args.language}",
        f"#license\t{args.license}",
        f"#source\t{args.source}",
    ]
    total = 0
    for number, bid in enumerate(BOOK_IDS, start=1):
        last = {}
        for chapter, verse, text in parse_book(files[bid], number, bid):
            # Verses must follow each other 1, 2, 3... inside a chapter: catches typos and lost verses.
            if verse != last.get(chapter, 0) + 1:
                raise ValueError(f"{bid} {chapter}:{verse} follows verse {last.get(chapter, 0)}")
            last[chapter] = verse
            if not text:
                print(f"warning: empty verse {bid} {chapter}:{verse}", file=sys.stderr)
            lines.append(f"{number}\t{chapter}\t{verse}\t{text}")
            total += 1
    Path(args.output).write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    print(f"{total} verses written to {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
