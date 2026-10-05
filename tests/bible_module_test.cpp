/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

// Tests of the Bible module loader. No framework: a failed CHECK prints the line and the
// program exits with a non-zero code (so CTest and CI see it).
//
// VYRA_LSG_PATH (defined by tests/CMakeLists.txt) points to data/bibles/lsg1910.tsv.

#include "src/bible/bible_module.hpp"
#include "tests/check.hpp"

#include <chrono>
#include <cstdio>
#include <sstream>
#include <string>

using namespace vyra::bible;

namespace {

const char *kHeader = "#vyra-module\t1\n#code\tTST\n#name\tTest\n#language\tfr\n#license\tPD\n#source\ttest\n";

LoadResult loadText(BibleModule &m, const std::string &text)
{
	std::istringstream in(text);
	return m.loadFromStream(in);
}

std::string verseOrEmpty(const BibleModule &m, int b, int c, int v)
{
	const auto t = m.verse({b, c, v});
	return t ? std::string(*t) : std::string("<none>");
}

// ---------- small synthetic modules ----------

void testValidModule()
{
	BibleModule m;
	const auto r = loadText(m, std::string(kHeader) +
					   "1\t1\t1\tAlpha\n1\t1\t2\tBeta\n1\t2\t1\tGamma é\n43\t3\t16\tJean\n");
	CHECK(r.ok());
	CHECK(m.verseCount() == 4);
	CHECK(m.info().code == "TST" && m.info().name == "Test" && m.info().language == "fr");
	CHECK(verseOrEmpty(m, 1, 1, 2) == "Beta");
	CHECK(verseOrEmpty(m, 1, 2, 1) == "Gamma é");
	CHECK(verseOrEmpty(m, 43, 3, 16) == "Jean");
	CHECK(verseOrEmpty(m, 1, 1, 3) == "<none>"); // no such verse
	CHECK(verseOrEmpty(m, 2, 1, 1) == "<none>"); // book absent
	CHECK(m.chapterCount(1) == 2 && m.chapterCount(43) == 3 && m.chapterCount(2) == 0);
	CHECK(m.verseCount(1, 1) == 2 && m.verseCount(1, 2) == 1 && m.verseCount(1, 3) == 0 && m.verseCount(2, 1) == 0);
}

void testOutOfRangeReferences()
{
	BibleModule m;
	CHECK(loadText(m, std::string(kHeader) + "1\t1\t1\tAlpha\n").ok());
	CHECK(verseOrEmpty(m, 0, 1, 1) == "<none>");
	CHECK(verseOrEmpty(m, 67, 1, 1) == "<none>");
	CHECK(verseOrEmpty(m, -1, 1, 1) == "<none>");
	CHECK(verseOrEmpty(m, 1, 0, 1) == "<none>");
	CHECK(verseOrEmpty(m, 1, 1, 0) == "<none>");
	CHECK(verseOrEmpty(m, 1, 1, 5000) == "<none>"); // must not alias another verse through the key arithmetic
	CHECK(verseOrEmpty(m, 1, 5000, 1) == "<none>");
	CHECK(m.chapterCount(0) == 0 && m.chapterCount(99) == 0 && m.verseCount(1, 0) == 0 && m.verseCount(99, 1) == 0);
}

void testEmptyModuleQueries()
{
	BibleModule m; // never loaded
	CHECK(m.empty());
	CHECK(verseOrEmpty(m, 1, 1, 1) == "<none>");
	CHECK(m.chapterCount(1) == 0 && m.verseCount(1, 1) == 0);
}

void testWindowsLineEndingsAndBom()
{
	std::string crlf =
		"\xEF\xBB\xBF#vyra-module\t1\r\n#code\tTST\r\n#name\tTest\r\n#language\tfr\r\n#license\tPD\r\n#source\ttest\r\n"
		"1\t1\t1\tAlpha\r\n1\t1\t2\tBeta\r\n";
	BibleModule m;
	CHECK(loadText(m, crlf).ok());
	CHECK(verseOrEmpty(m, 1, 1, 1) == "Alpha"); // no stray '\r' at the end of the text
	CHECK(verseOrEmpty(m, 1, 1, 2) == "Beta");
	CHECK(m.info().source == "test");
}

void testRejections()
{
	struct Case {
		const char *name;
		std::string text;
		LoadError expected;
		int line;
	};
	const std::string h = kHeader;
	const Case cases[] = {
		{"empty file", "", LoadError::NotAModule, 0},
		{"no magic line", "1\t1\t1\tAlpha\n", LoadError::NotAModule, 1},
		{"unknown version", "#vyra-module\t2\n", LoadError::NotAModule, 1},
		{"no verse", h, LoadError::Empty, 0},
		{"missing license", "#vyra-module\t1\n#code\tT\n#name\tT\n#language\tfr\n#source\ts\n1\t1\t1\tA\n",
		 LoadError::MissingHeader, 6},
		{"empty header value",
		 "#vyra-module\t1\n#code\t\n#name\tT\n#language\tfr\n#license\tPD\n#source\ts\n1\t1\t1\tA\n",
		 LoadError::MissingHeader, 7},
		{"header after verses", h + "1\t1\t1\tA\n#code\tX\n", LoadError::CorruptRecord, 8},
		{"three fields", h + "1\t1\tA\n", LoadError::CorruptRecord, 7},
		{"book 0", h + "0\t1\t1\tA\n", LoadError::CorruptRecord, 7},
		{"book 67", h + "67\t1\t1\tA\n", LoadError::CorruptRecord, 7},
		{"chapter 0", h + "1\t0\t1\tA\n", LoadError::CorruptRecord, 7},
		{"verse 1000", h + "1\t1\t1000\tA\n", LoadError::CorruptRecord, 7},
		{"negative number", h + "1\t-1\t1\tA\n", LoadError::CorruptRecord, 7},
		{"number with letters", h + "1\t1x\t1\tA\n", LoadError::CorruptRecord, 7},
		{"number with space", h + "1\t 1\t1\tA\n", LoadError::CorruptRecord, 7},
		{"empty text", h + "1\t1\t1\t\n", LoadError::CorruptRecord, 7},
		{"tab in text", h + "1\t1\t1\tA\tB\n", LoadError::CorruptRecord, 7},
		{"duplicate verse", h + "1\t1\t1\tA\n1\t1\t1\tB\n", LoadError::CorruptRecord, 8},
		{"verse out of order", h + "1\t1\t2\tA\n1\t1\t1\tB\n", LoadError::CorruptRecord, 8},
		{"book out of order", h + "2\t1\t1\tA\n1\t1\t1\tB\n", LoadError::CorruptRecord, 8},
		{"invalid UTF-8 (lone 0xE9, Latin-1 'é')", h + "1\t1\t1\tcaf\xE9\n", LoadError::BadEncoding, 7},
		{"truncated UTF-8 at end of line", h + "1\t1\t1\tcaf\xC3\n", LoadError::BadEncoding, 7},
		{"overlong UTF-8", h + "1\t1\t1\t\xC0\xAF\n", LoadError::BadEncoding, 7},
		{"UTF-16 surrogate", h + "1\t1\t1\t\xED\xA0\x80\n", LoadError::BadEncoding, 7},
	};
	for (const auto &c : cases) {
		BibleModule m;
		const auto r = loadText(m, c.text);
		++vyra::testing::checks();
		if (r.error != c.expected || (c.line != 0 && r.line != c.line)) {
			++vyra::testing::failures();
			std::printf("FAIL rejection '%s': error=%d line=%d (expected error=%d line=%d) detail=%s\n",
				    c.name, static_cast<int>(r.error), r.line, static_cast<int>(c.expected), c.line,
				    r.detail.c_str());
		}
		CHECK(m.empty()); // a failed load never leaves a half-loaded Bible
		CHECK(!r.ok());
	}
}

void testReloadAfterFailureResets()
{
	BibleModule m;
	CHECK(loadText(m, std::string(kHeader) + "1\t1\t1\tAlpha\n").ok());
	CHECK(!loadText(m, "garbage").ok());
	CHECK(m.empty() && verseOrEmpty(m, 1, 1, 1) == "<none>" && m.info().code.empty());
	CHECK(loadText(m, std::string(kHeader) + "2\t1\t1\tBeta\n").ok());
	CHECK(verseOrEmpty(m, 1, 1, 1) == "<none>" &&
	      verseOrEmpty(m, 2, 1, 1) == "Beta"); // nothing left from the first load
}

void testMissingFile()
{
	BibleModule m;
	const auto r = m.loadFromFile("/this/path/does/not/exist.tsv");
	CHECK(r.error == LoadError::FileNotFound);
	CHECK(m.empty());
}

// ---------- the real Louis Segond 1910 module ----------

void testRealSegond()
{
	BibleModule m;
	const auto start = std::chrono::steady_clock::now();
	const auto r = m.loadFromFile(VYRA_LSG_PATH);
	const auto ms =
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
	if (!r.ok()) {
		++vyra::testing::failures();
		std::printf("FAIL cannot load %s: error=%d line=%d %s\n", VYRA_LSG_PATH, static_cast<int>(r.error),
			    r.line, r.detail.c_str());
		return;
	}
	std::printf("Segond 1910 loaded: %zu verses in %lld ms\n", m.verseCount(), static_cast<long long>(ms));

	CHECK(m.info().code == "LSG1910" && m.info().language == "fr" && m.info().license == "Public Domain");
	CHECK(m.verseCount() ==
	      31170);     // regression value: the Segond numbers the psalm titles, so it exceeds the usual 31,102
	CHECK(ms < 1000); // loose bound: the objective (section 19) is verified in Milestone 10

	// Well-known verses, checked word for word against the printed Segond 1910.
	CHECK(verseOrEmpty(m, 1, 1, 1) == "Au commencement, Dieu créa les cieux et la terre.");
	CHECK(verseOrEmpty(m, 43, 3, 16) ==
	      "Car Dieu a tant aimé le monde qu’il a donné son Fils unique, afin que quiconque croit en lui ne périsse point, "
	      "mais qu’il ait la vie éternelle.");
	CHECK(verseOrEmpty(m, 66, 22, 21) == "Que la grâce du Seigneur Jésus soit avec tous!");
	// Psalm 23:1 in the Segond: the numbered title and the first line form one verse.
	CHECK(verseOrEmpty(m, 19, 23, 1) == "Cantique de David. L’Éternel est mon berger: je ne manquerai de rien.");
	CHECK(verseOrEmpty(m, 19, 119, 115).rfind("Éloignez-vous de moi, méchants", 0) ==
	      0); // fixed typo "11 5" in the source

	// Structure of the canon.
	CHECK(m.chapterCount(1) == 50);   // Genesis
	CHECK(m.chapterCount(19) == 150); // Psalms
	CHECK(m.chapterCount(40) == 28);  // Matthew
	CHECK(m.chapterCount(43) == 21);  // John
	CHECK(m.chapterCount(66) == 22);  // Revelation
	CHECK(m.verseCount(1, 1) == 31);
	CHECK(m.verseCount(19, 119) == 176);
	CHECK(m.verseCount(43, 3) == 36);
	CHECK(m.verseCount(64, 1) == 15); // 3 John (the Segond has 15 verses, the KJV 14)
	CHECK(m.verseCount(65, 1) == 25); // Jude
	CHECK(m.verseCount(31, 1) == 21); // Obadiah
	CHECK(m.verseCount(57, 1) == 25); // Philemon

	// Every book, every chapter is present: 1189 chapters in the Protestant canon.
	int chapters = 0;
	bool allBooks = true;
	for (int b = 1; b <= kBookCount; ++b) {
		if (m.chapterCount(b) == 0)
			allBooks = false;
		for (int c = 1; c <= m.chapterCount(b); ++c) {
			if (m.verseCount(b, c) == 0) {
				allBooks = false;
				std::printf("missing chapter %d:%d\n", b, c);
			}
			++chapters;
		}
	}
	CHECK(allBooks);
	CHECK(chapters == 1189);

	// No verse may be empty or contain a USFM leftover.
	bool clean = true;
	for (int b = 1; b <= kBookCount && clean; ++b)
		for (int c = 1; c <= m.chapterCount(b) && clean; ++c)
			for (int v = 1; v <= m.verseCount(b, c); ++v) {
				const auto t = m.verse({b, c, v});
				if (!t || t->empty() || t->find('\\') != std::string_view::npos || t->front() == ' ' ||
				    t->back() == ' ' || t->find("  ") != std::string_view::npos) {
					clean = false;
					std::printf("dirty or missing verse %d:%d:%d\n", b, c, v);
					break;
				}
			}
	CHECK(clean);
}

} // namespace

int main()
{
	testValidModule();
	testOutOfRangeReferences();
	testEmptyModuleQueries();
	testWindowsLineEndingsAndBom();
	testRejections();
	testReloadAfterFailureResets();
	testMissingFile();
	testRealSegond();

	return vyra::testing::finish();
}
