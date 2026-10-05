#!/usr/bin/env python3
"""Independent check of usfm_to_vyra_tsv.py: no verse text may be lost or invented.

For every book, it counts the non-space characters of all text-bearing lines of the USFM
source (after removing footnotes, cross references and markers) and compares them with the
characters of the converted verses. Any difference means text was dropped or duplicated.

Usage: audit_usfm_conversion.py <usfm_dir> <module.tsv>
"""
import re
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from usfm_to_vyra_tsv import ERRATA  # noqa: E402

BOOK_IDS = (
    "GEN EXO LEV NUM DEU JOS JDG RUT 1SA 2SA 1KI 2KI 1CH 2CH EZR NEH EST JOB PSA PRO ECC SNG "
    "ISA JER LAM EZK DAN HOS JOL AMO OBA JON MIC NAM HAB ZEP HAG ZEC MAL "
    "MAT MRK LUK JHN ACT ROM 1CO 2CO GAL EPH PHP COL 1TH 2TH 1TI 2TI TIT PHM HEB JAS 1PE 2PE 1JN 2JN 3JN JUD REV"
).split()

# Everything that is NOT a verse line must be one of these known non-text markers,
# otherwise the audit stops: a new, unknown marker may hide verse text.
KNOWN_NON_TEXT = {"c", "r", "s", "mr", "ms1", "ms2", "b", "h", "mte9", "mte7", "sd3"}
TEXT = {"v", "q", "p", "m", "pi"}


def source_chars(path: Path, bid: str) -> int:
    total = 0
    in_chapters = False
    text = path.read_text(encoding="utf-8")
    # Known typos of the source, fixed on purpose by the converter (same list, so that the
    # audit only reports unintended differences).
    for old, new in ERRATA.get(bid, []):
        text = text.replace(old, new)
    for raw in text.splitlines():
        m = re.match(r"\\([a-z0-9]+)\b\s*(.*)$", raw)
        if not m:
            if in_chapters and raw.strip():
                raise SystemExit(f"{path.name}: unexpected line without marker: {raw[:60]!r}")
            continue
        marker, rest = m.groups()
        if marker == "c":
            in_chapters = True
        if not in_chapters:
            continue
        if marker == "v":
            rest = re.sub(r"^\d+(-\d+)?\s*", "", rest)
        elif marker not in TEXT:
            if marker not in KNOWN_NON_TEXT:
                raise SystemExit(f"{path.name}: unknown marker \\{marker} inside a chapter")
            continue
        rest = re.sub(r"\\(x|f|fe)\s.*?\\\1\*", "", rest, flags=re.S)
        rest = re.sub(r"\\\+?[a-z][a-z0-9]*\*?", "", rest)
        total += len(re.sub(r"\s+", "", rest))
    return total


def main() -> int:
    usfm_dir, tsv = Path(sys.argv[1]), Path(sys.argv[2])
    files = {}
    for p in usfm_dir.glob("*.sfm"):
        m = re.search(r"-\d\d-([A-Z0-9]{3})\.", p.name)
        if m:
            files[m.group(1)] = p
    converted = defaultdict(int)
    for line in tsv.read_text(encoding="utf-8").splitlines():
        if line.startswith("#"):
            continue
        book, _c, _v, text = line.split("\t", 3)
        converted[int(book)] += len(re.sub(r"\s+", "", text))
    bad = 0
    for number, bid in enumerate(BOOK_IDS, start=1):
        src = source_chars(files[bid], bid)
        if src != converted[number]:
            bad += 1
            print(f"MISMATCH {bid}: source {src} characters, converted {converted[number]}")
    print("audit OK: all 66 books identical" if not bad else f"audit FAILED: {bad} book(s)")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
