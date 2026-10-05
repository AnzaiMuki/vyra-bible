#!/usr/bin/env python3
"""Extract the cross references of the Segond USFM source, to test the reference parser.

Every footnote cross reference of the source ("\\xt Job 38:4. Ps 33:6; 89:12") names a book by the
abbreviation the Segond itself uses. For each distinct (abbreviation, chapter) the script keeps the
highest verse mentioned. The parser test resolves "<abbreviation> <chapter>:<verse>" and checks that
the verse exists: a wrong book in the alias table fails on hundreds of references.

Usage: extract_crossrefs.py <usfm_dir> <output.tsv>
Output: abbreviation<TAB>chapter<TAB>highest verse<TAB>number of references
"""
import re
import sys
from collections import defaultdict
from pathlib import Path

REF = re.compile(
    r"(?:(?P<book>(?:[123]\s)?[A-ZÉÈ][A-Za-zéèêëôûîïç]*)\s+)?"
    r"(?P<ch>\d+):(?P<v>\d+)(?:\s*[-–]\s*(?P<v2>\d+))?(?P<more>(?:\s*,\s*\d+)*)"
)


def main() -> int:
    src, out = Path(sys.argv[1]), Path(sys.argv[2])
    best = defaultdict(int)
    count = defaultdict(int)
    for path in sorted(src.glob("*.sfm")):
        for line in path.read_text(encoding="utf-8").splitlines():
            for note in re.finditer(r"\\xt ([^\\]*)", line):
                book = None
                for m in REF.finditer(note.group(1)):
                    if m.group("book"):
                        book = m.group("book")
                    if book is None:
                        continue
                    verses = [int(m.group("v"))]
                    if m.group("v2"):
                        verses.append(int(m.group("v2")))
                    verses += [int(x) for x in re.findall(r"\d+", m.group("more"))]
                    key = (book, int(m.group("ch")))
                    best[key] = max(best[key], max(verses))
                    count[key] += 1
    rows = sorted(best, key=lambda k: (k[0], k[1]))
    out.write_text(
        "".join(f"{b}\t{c}\t{best[(b, c)]}\t{count[(b, c)]}\n" for b, c in rows), encoding="utf-8", newline="\n"
    )
    print(f"{len(rows)} (abbreviation, chapter) pairs, {len({b for b, _ in rows})} abbreviations")
    return 0


if __name__ == "__main__":
    sys.exit(main())
