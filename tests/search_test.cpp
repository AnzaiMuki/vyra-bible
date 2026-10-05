/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

// Tests of the search engine: book names and the understanding of what the operator types.
//
// VYRA_LSG_PATH       data/bibles/lsg1910.tsv
// VYRA_CROSSREFS_PATH tests/data/segond_crossrefs.tsv (made by scripts/extract_crossrefs.py)
// VYRA_ANOMALIES_PATH tests/data/segond_crossref_anomalies.tsv (typos of the source, see testSegondCrossReferences)

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include "src/bible/bible_module.hpp"
#include "src/search/book_catalog.hpp"
#include "src/search/passage_query.hpp"
#include "tests/check.hpp"

using namespace vyra::bible;
using namespace vyra::search;

namespace {

BibleModule g_bible; // the real Louis Segond 1910

const char *statusName(QueryStatus s)
{
	switch (s) {
	case QueryStatus::Ok:
		return "Ok";
	case QueryStatus::Empty:
		return "Empty";
	case QueryStatus::Syntax:
		return "Syntax";
	case QueryStatus::Unsupported:
		return "Unsupported";
	case QueryStatus::UnknownBook:
		return "UnknownBook";
	case QueryStatus::AmbiguousBook:
		return "AmbiguousBook";
	case QueryStatus::BookNotInModule:
		return "BookNotInModule";
	case QueryStatus::NeedChapter:
		return "NeedChapter";
	case QueryStatus::Incomplete:
		return "Incomplete";
	case QueryStatus::ChapterOutOfRange:
		return "ChapterOutOfRange";
	case QueryStatus::VerseOutOfRange:
		return "VerseOutOfRange";
	case QueryStatus::ReversedRange:
		return "ReversedRange";
	}
	return "?";
}

// "<query>" must resolve to exactly book b, from (sc:sv) to (ec:ev).
void expectPassage(const std::string &query, int b, int sc, int sv, int ec, int ev)
{
	const QueryResult r = resolveQuery(query, g_bible);
	++vyra::testing::checks();
	const Passage &p = r.passage;
	if (!r.ok() || p.book != b || p.startChapter != sc || p.startVerse != sv || p.endChapter != ec ||
	    p.endVerse != ev) {
		++vyra::testing::failures();
		std::printf("FAIL '%s': got %s %d %d:%d-%d:%d, expected %d %d:%d-%d:%d\n", query.c_str(),
			    statusName(r.status), p.book, p.startChapter, p.startVerse, p.endChapter, p.endVerse, b, sc,
			    sv, ec, ev);
	}
}

void expectStatus(const std::string &query, QueryStatus status, int limit = -1)
{
	const QueryResult r = resolveQuery(query, g_bible);
	++vyra::testing::checks();
	if (r.status != status || (limit >= 0 && r.limit != limit)) {
		++vyra::testing::failures();
		std::printf("FAIL '%s': got %s (limit %d), expected %s (limit %d)\n", query.c_str(),
			    statusName(r.status), r.limit, statusName(status), limit);
	}
}

// ---------- book catalog ----------

void testCatalog()
{
	CHECK(catalogConflicts().empty());
	for (const auto &c : catalogConflicts())
		std::printf("conflict: %s\n", c.c_str());
	CHECK(catalogAliasCount() > 300);

	CHECK(frenchBookName(1) == "Genèse" && frenchBookName(43) == "Jean" && frenchBookName(66) == "Apocalypse");
	CHECK(englishBookName(1) == "Genesis" && englishBookName(43) == "John" && englishBookName(66) == "Revelation");
	CHECK(frenchBookName(0).empty() && frenchBookName(67).empty() && englishBookName(-1).empty());

	// Every printed name, French and English, finds its own book, and the names are all different.
	std::set<std::string> seen;
	for (int b = 1; b <= kBookCount; ++b) {
		for (const auto name : {frenchBookName(b), englishBookName(b)}) {
			std::string key;
			for (const char c : normalizeText(name))
				if (c != ' ')
					key.push_back(c);
			CHECK(findBookExact(key) == b);
		}
		CHECK(seen.insert(std::string(frenchBookName(b))).second);
	}
	CHECK(seen.size() == 66);

	CHECK(normalizeText("Éz  34:11") == "ez  34:11");
	CHECK(normalizeText("GENÈSE") == "genese");
	CHECK(normalizeText("Cœur") == "coeur");
	CHECK(normalizeText("3\xE2\x80\x93"
			    "5") == "3-5"); // en dash
	CHECK(normalizeText("3\xE2\x80\x94"
			    "5") == "3-5"); // em dash
	CHECK(normalizeText("l\xE2\x80\x99"
			    "Eternel") == "leternel"); // typographic apostrophe
	CHECK(normalizeText("a\xC2\xA0"
			    "b") == "a b");        // no-break space
	CHECK(normalizeText("caf\xE9") == "caf "); // invalid UTF-8 (Latin-1) becomes a space, never crashes
	CHECK(normalizeText("").empty());
}

void testAbbreviationsOfTheSegond()
{
	// Abbreviations printed in the Segond cross references: the number is the book (independent of the code).
	const std::map<std::string, int> known = {
		{"Ge", 1},    {"Ex", 2},   {"Lé", 3},    {"No", 4},    {"De", 5},    {"Jos", 6},   {"Jg", 7},
		{"Ru", 8},    {"1 S", 9},  {"2 S", 10},  {"1 R", 11},  {"2 R", 12},  {"1 Ch", 13}, {"2 Ch", 14},
		{"Esd", 15},  {"Né", 16},  {"Est", 17},  {"Job", 18},  {"Ps", 19},   {"Pr", 20},   {"Ec", 21},
		{"Ca", 22},   {"És", 23},  {"Jé", 24},   {"La", 25},   {"Éz", 26},   {"Da", 27},   {"Os", 28},
		{"Joë", 29},  {"Am", 30},  {"Ab", 31},   {"Jon", 32},  {"Mi", 33},   {"Na", 34},   {"Ha", 35},
		{"So", 36},   {"Ag", 37},  {"Za", 38},   {"Mal", 39},  {"Mt", 40},   {"Mc", 41},   {"Lu", 42},
		{"Jn", 43},   {"Ac", 44},  {"Ro", 45},   {"1 Co", 46}, {"2 Co", 47}, {"Ga", 48},   {"Ép", 49},
		{"Ph", 50},   {"Col", 51}, {"1 Th", 52}, {"2 Th", 53}, {"1 Ti", 54}, {"2 Ti", 55}, {"Tit", 56},
		{"Phm", 57},  {"Hé", 58},  {"Ja", 59},   {"1 Pi", 60}, {"2 Pi", 61}, {"1 Jn", 62}, {"2 Jn", 63},
		{"3 Jn", 64}, {"Jud", 65}, {"Ap", 66},
	};
	CHECK(known.size() == 66);
	for (const auto &[abbr, book] : known) {
		const QueryResult r = resolveQuery(abbr + " 1", g_bible);
		++vyra::testing::checks();
		if (r.book != book) {
			++vyra::testing::failures();
			std::printf("FAIL abbreviation '%s' -> book %d, expected %d (%s)\n", abbr.c_str(), r.book, book,
				    statusName(r.status));
		}
	}
}

// ---------- reading the query ----------

void testSameVerseManyWays()
{
	for (const char *q :
	     {"Jean 3:16", "Jn 3 16", "jn 3.16", "JEAN 3,16", "Jn3:16", "jn3 16", "Jean 3 v 16", "Jean 3 verset 16",
	      "Jean chapitre 3 verset 16", "  Jn   3 : 16  ", "Jn. 3:16", "jean 3:16", "Jn 3:16-16", "John 3:16"})
		expectPassage(q, 43, 3, 16, 3, 16);
}

void testRangesAndNumberedBooks()
{
	for (const char *q : {"1 Co 13.4-7", "1 Corinthiens 13:4-7", "1Co 13 4-7",
			      "I Co 13:4\xE2\x80\x93"
			      "7",
			      "1 co. 13,4-7", "1 Corinthians 13:4-7", "1ère Corinthiens 13:4-7", "1Cor 13:4 à 7",
			      "1 Co 13:4 - 7", "i co 13:4-7"})
		expectPassage(q, 46, 13, 4, 13, 7);
	expectPassage("2Co 5:17", 47, 5, 17, 5, 17);
	expectPassage("II Co 5:17", 47, 5, 17, 5, 17);
	expectPassage("1 Jn 1:9", 62, 1, 9, 1, 9);
	expectPassage("1 Jean 1:9", 62, 1, 9, 1, 9);
	expectPassage("I Jean 1:9", 62, 1, 9, 1, 9);
	expectPassage("1 John 1:9", 62, 1, 9, 1, 9);
	expectPassage("1 S 3:10", 9, 3, 10, 3, 10);
	expectPassage("2 R 2:11", 12, 2, 11, 2, 11);
	expectPassage("1 Ch 16:34", 13, 16, 34, 16, 34); // "Ch" is the book here, not "chapter"
}

void testChaptersAndCrossChapterRanges()
{
	expectPassage("Jn 3", 43, 3, 1, 3, 36);
	expectPassage("Jean chapitre 3", 43, 3, 1, 3, 36);
	expectPassage("Ps 23", 19, 23, 1, 23, 6);
	expectPassage("Ps 119", 19, 119, 1, 119, 176);
	expectPassage("Jn 3-4", 43, 3, 1, 4, 54);
	expectPassage("Jn 3 - 4", 43, 3, 1, 4, 54);
	expectPassage("Jn 3:16-4:2", 43, 3, 16, 4, 2);
	expectPassage("Jn 3,16-4,2", 43, 3, 16, 4, 2);
	expectPassage("Gn 1:31-2:3", 1, 1, 31, 2, 3);
	expectPassage("Ps 119:175-176", 19, 119, 175, 119, 176);
	expectPassage("Ap 22:21", 66, 22, 21, 22, 21);
}

void testSingleChapterBooks()
{
	expectPassage("Jude 5", 65, 1, 5, 1, 5);
	expectPassage("Jude 5-7", 65, 1, 5, 1, 7);
	expectPassage("Jude 1:5", 65, 1, 5, 1, 5);
	expectPassage("Jude 1:5-7", 65, 1, 5, 1, 7);
	expectPassage("3 Jn 4", 64, 1, 4, 1, 4);
	expectPassage("3 Jean 15", 64, 1, 15, 1, 15); // the Segond has 15 verses
	expectPassage("Phm 6", 57, 1, 6, 1, 6);
	expectPassage("Abdias 21", 31, 1, 21, 1, 21);
	expectStatus("Jude 30", QueryStatus::VerseOutOfRange, 25);
	expectStatus("Jude 2:1", QueryStatus::ChapterOutOfRange, 1);
	expectStatus("Jude 0", QueryStatus::VerseOutOfRange, 25);
}

void testAccentsAndLanguages()
{
	expectPassage("Genèse 1:1", 1, 1, 1, 1, 1);
	expectPassage("genese 1 1", 1, 1, 1, 1, 1);
	expectPassage("GENÈSE 1:1", 1, 1, 1, 1, 1);
	expectPassage("Genesis 1:1", 1, 1, 1, 1, 1);
	expectPassage("Gn 1:1", 1, 1, 1, 1, 1);
	expectPassage("Ésaïe 53:5", 23, 53, 5, 53, 5);
	expectPassage("Isaïe 53:5", 23, 53, 5, 53, 5);
	expectPassage("Es 53:5", 23, 53, 5, 53, 5);
	expectPassage("Isaiah 53:5", 23, 53, 5, 53, 5);
	expectPassage("Éz 34:11", 26, 34, 11, 34, 11);
	expectPassage("Ezekiel 34:11", 26, 34, 11, 34, 11);
	expectPassage("Hé 11:1", 58, 11, 1, 11, 1);
	expectPassage("Hebreux 11:1", 58, 11, 1, 11, 1);
	expectPassage("Joë 2:28", 29, 2, 28, 2, 28);
	expectPassage("Joël 2:28", 29, 2, 28, 2, 28);
	expectPassage("Psalm 23:1", 19, 23, 1, 23, 1);
	expectPassage("Psaume 23:1", 19, 23, 1, 23, 1);
	expectPassage("Rev 22:21", 66, 22, 21, 22, 21);
	expectPassage("Apocalypse 22:21", 66, 22, 21, 22, 21);
	expectPassage("Song of Solomon 2:1", 22, 2, 1, 2, 1);
	expectPassage("Cantique des Cantiques 2:1", 22, 2, 1, 2, 1);
	expectPassage("Actes des Apôtres 2:1", 44, 2, 1, 2, 1);
}

void testUniquePrefixes()
{
	expectPassage("Jea 3:16", 43, 3, 16, 3, 16);
	expectPassage("Genes 1:1", 1, 1, 1, 1, 1);
	expectPassage("Romai 8:28", 45, 8, 28, 8, 28);
	expectPassage("1 Corin 13:4", 46, 13, 4, 13, 4);
}

void testErrorsAndTypingInProgress()
{
	expectStatus("", QueryStatus::Empty);
	expectStatus("   ", QueryStatus::Empty);
	expectStatus("Xyz 3:1", QueryStatus::UnknownBook);
	expectStatus("3:16", QueryStatus::Syntax);
	expectStatus("3 16", QueryStatus::Syntax);
	expectStatus("Jn", QueryStatus::NeedChapter);
	expectStatus("Jean chapitre", QueryStatus::NeedChapter);
	expectStatus("Jn 3:", QueryStatus::Incomplete);
	expectStatus("Jn 3:16-", QueryStatus::Incomplete);
	expectStatus("Jn 3:16-4:", QueryStatus::Incomplete);
	expectStatus("Jn -", QueryStatus::Syntax);
	expectStatus("Jn 3::16", QueryStatus::Syntax);
	expectStatus("Jn 3 16 18", QueryStatus::Syntax);
	expectStatus("Jn 3:16:18", QueryStatus::Syntax);
	expectStatus("Jn 1 2 3 4 5", QueryStatus::Syntax);
	expectStatus("Jn trois", QueryStatus::UnknownBook); // every word before the first number belongs to the name
	expectStatus("Jn 3:16;18", QueryStatus::Unsupported);
	expectStatus("Jn 3:16; 4:2", QueryStatus::Unsupported);

	expectStatus("Jn 22:1", QueryStatus::ChapterOutOfRange, 21);
	expectStatus("Jn 0", QueryStatus::ChapterOutOfRange, 21);
	expectStatus("Jn 99999999", QueryStatus::ChapterOutOfRange, 21);
	expectStatus("Jn 3-22", QueryStatus::ChapterOutOfRange, 21);
	expectStatus("Jn 3:40", QueryStatus::VerseOutOfRange, 36);
	expectStatus("Jn 3:0", QueryStatus::VerseOutOfRange, 36);
	expectStatus("Jn 3:16-40", QueryStatus::VerseOutOfRange, 36);
	expectStatus("Jn 3:16-4:99", QueryStatus::VerseOutOfRange, 54);
	expectStatus("Jn 3:18-16", QueryStatus::ReversedRange);
	expectStatus("Jn 4-3", QueryStatus::ReversedRange);
	expectStatus("Jn 3:16-3:10", QueryStatus::ReversedRange);
	expectStatus("Jn 4:1-3:1", QueryStatus::ReversedRange);

	// The book is reported whenever it is known, so the interface can show it while the operator types.
	CHECK(resolveQuery("Jn", g_bible).book == 43);
	CHECK(resolveQuery("Jn 3:", g_bible).book == 43);
	CHECK(resolveQuery("Jn 3:40", g_bible).book == 43);
	CHECK(resolveQuery("Jn 3", g_bible).inProgress() == false);
	CHECK(resolveQuery("Jn", g_bible).inProgress() && resolveQuery("", g_bible).inProgress());
	CHECK(!resolveQuery("Xyz 1", g_bible).inProgress());
}

void testAmbiguousBooks()
{
	const QueryResult r = resolveQuery("Jo 3:1", g_bible);
	CHECK(r.status == QueryStatus::AmbiguousBook);
	const std::set<int> c(r.candidates.begin(), r.candidates.end());
	CHECK(c.count(18) && c.count(6) && c.count(29) && c.count(32)); // Job, Josué, Joël, Jonas
	CHECK(r.candidates.size() >= 4);
	CHECK(std::is_sorted(r.candidates.begin(), r.candidates.end()));

	const QueryResult ju = resolveQuery("Ju 1:1", g_bible); // Juges or Jude: the operator must choose
	CHECK(ju.status == QueryStatus::AmbiguousBook);
	CHECK(ju.candidates == (std::vector<int>{7, 65}));

	expectPassage("Phil 1:1", 50, 1, 1, 1, 1); // "Phil" is Philippiens (Philémon is "Phm" or "Philem")
	expectPassage("Philem 1", 57, 1, 1, 1, 1);
	expectPassage("Ph 1:1", 50, 1, 1, 1, 1); // the Segond abbreviation is exact
	expectPassage("Phm 1", 57, 1, 1, 1, 1);
	expectPassage("Job 1:1", 18, 1, 1, 1, 1); // exact name beats the longer names that start with it
	expectPassage("Jon 1:1", 32, 1, 1, 1, 1);
	expectPassage("Est 1:1", 17, 1, 1, 1, 1);
}

void testBookMissingFromModule()
{
	BibleModule onlyJohn;
	std::istringstream in(
		"#vyra-module\t1\n#code\tT\n#name\tT\n#language\tfr\n#license\tPD\n#source\ts\n43\t1\t1\tAu commencement\n43\t1\t2\tEt\n");
	CHECK(onlyJohn.loadFromStream(in).ok());
	CHECK(resolveQuery("Gn 1:1", onlyJohn).status == QueryStatus::BookNotInModule);
	CHECK(resolveQuery("Gn 1:1", onlyJohn).book == 1);
	CHECK(resolveQuery("Jn 1:2", onlyJohn).ok());
	CHECK(resolveQuery("Jn 1:3", onlyJohn).status == QueryStatus::VerseOutOfRange);
	CHECK(resolveQuery("Jn 1:3", onlyJohn).limit == 2);

	BibleModule none; // nothing loaded: every question gets a clean answer
	CHECK(resolveQuery("Jn 3:16", none).status == QueryStatus::BookNotInModule);
}

// ---------- passage helpers ----------

void testPassageVersesAndFormat()
{
	auto verses = [](const char *q) {
		const QueryResult r = resolveQuery(q, g_bible);
		return passageVerses(r.passage, g_bible);
	};
	CHECK(verses("Jn 3:16").size() == 1);
	CHECK(verses("Jn 3:16-4:2").size() == (36 - 16 + 1) + 2);
	CHECK(verses("Jn 3:16-4:2").front().chapter == 3 && verses("Jn 3:16-4:2").front().verse == 16);
	CHECK(verses("Jn 3:16-4:2").back().chapter == 4 && verses("Jn 3:16-4:2").back().verse == 2);
	CHECK(verses("Ps 119").size() == 176);
	CHECK(verses("Jude 5-7").size() == 3);
	CHECK(passageVerses(Passage{}, g_bible).empty());

	auto fmt = [](const char *q) {
		return formatPassage(resolveQuery(q, g_bible).passage, g_bible);
	};
	CHECK(fmt("Jn 3:16") == "Jean 3:16");
	CHECK(fmt("Jn 3:16-18") == "Jean 3:16\xE2\x80\x93"
				   "18");
	CHECK(fmt("Jn 3") == "Jean 3");
	CHECK(fmt("Jn 3-4") == "Jean 3\xE2\x80\x93"
			       "4");
	CHECK(fmt("Jn 3:16-4:2") == "Jean 3:16\xE2\x80\x93"
				    "4:2");
	CHECK(fmt("Jude 5") == "Jude 5");
	CHECK(fmt("Jude 5-7") == "Jude 5\xE2\x80\x93"
				 "7");
	CHECK(fmt("1 Co 13:4-7") == "1 Corinthiens 13:4\xE2\x80\x93"
				    "7");
	CHECK(fmt("Ps 23") == "Psaumes 23");
	CHECK(formatPassage(Passage{}, g_bible).empty());
}

// The printed form of every verse of the Bible must be understood back as the same verse.
void testRoundTripWholeBible()
{
	long verses = 0, wrong = 0;
	for (int b = 1; b <= kBookCount; ++b) {
		for (int c = 1; c <= g_bible.chapterCount(b); ++c) {
			const int last = g_bible.verseCount(b, c);
			// the whole chapter
			Passage whole{b, c, 1, c, last};
			const std::string printed = formatPassage(whole, g_bible);
			const QueryResult r = resolveQuery(printed, g_bible);
			++vyra::testing::checks();
			if (!r.ok() || r.passage.book != b || r.passage.startChapter != c ||
			    r.passage.startVerse != 1 || r.passage.endChapter != c || r.passage.endVerse != last) {
				++vyra::testing::failures();
				std::printf("FAIL round trip chapter '%s'\n", printed.c_str());
			}
			for (int v = 1; v <= last; ++v) {
				Passage p{b, c, v, c, v};
				const QueryResult q = resolveQuery(formatPassage(p, g_bible), g_bible);
				++verses;
				if (!q.ok() || q.passage.book != b || q.passage.startChapter != c ||
				    q.passage.startVerse != v || q.passage.endChapter != c || q.passage.endVerse != v) {
					if (++wrong <= 5)
						std::printf("FAIL round trip verse '%s'\n",
							    formatPassage(p, g_bible).c_str());
				}
			}
		}
	}
	std::printf("round trip: %ld verses, %ld wrong\n", verses, wrong);
	CHECK(verses == 31170);
	CHECK(wrong == 0);
}

// ---------- the Segond's own cross references ----------

void testSegondCrossReferences()
{
	// The cross reference notes of the source contain typos of their own (e.g. Revelation 19:16 points to
	// "1 Th 6:15" for "1 Ti 6:15"; Genesis 32:28 to "2 Ch 17:34" for "2 R 17:34"). They are pinned in
	// segond_crossref_anomalies.tsv: the test fails if that set changes in any way. Every other reference
	// must resolve to a verse that exists.
	std::set<std::string> known;
	{
		std::ifstream anomalies(VYRA_ANOMALIES_PATH);
		CHECK(anomalies.is_open());
		std::string line;
		while (std::getline(anomalies, line)) {
			std::istringstream fields(line);
			std::string abbr, chapter, verse, kind;
			std::getline(fields, abbr, '\t');
			std::getline(fields, chapter, '\t');
			std::getline(fields, verse, '\t');
			std::getline(fields, kind, '\t');
			known.insert(abbr + " " + chapter + ":" + verse + " " + kind);
		}
	}
	CHECK(known.size() == 45);

	std::ifstream in(VYRA_CROSSREFS_PATH);
	CHECK(in.is_open());
	std::string line;
	long pairs = 0, references = 0;
	std::map<std::string, int> bookOf;
	std::map<std::string, std::pair<int, int>> failuresPerAbbreviation; // abbreviation -> (unresolved, pairs)
	std::set<std::string> unresolved;
	bool onlyOutOfRange = true;
	while (std::getline(in, line)) {
		std::istringstream fields(line);
		std::string abbr, chapter, verse, count;
		std::getline(fields, abbr, '\t');
		std::getline(fields, chapter, '\t');
		std::getline(fields, verse, '\t');
		std::getline(fields, count, '\t');
		const QueryResult r = resolveQuery(abbr + " " + chapter + ":" + verse, g_bible);
		++pairs;
		references += std::stol(count);
		++failuresPerAbbreviation[abbr].second;
		if (r.ok()) {
			bookOf[abbr] = r.book;
			continue;
		}
		++failuresPerAbbreviation[abbr].first;
		const bool chapterError = r.status == QueryStatus::ChapterOutOfRange;
		const bool verseError = r.status == QueryStatus::VerseOutOfRange;
		// An unresolved reference must be a number that is too large, never an unknown or ambiguous book.
		if (!chapterError && !verseError) {
			onlyOutOfRange = false;
			std::printf("FAIL cross reference '%s %s:%s': %s\n", abbr.c_str(), chapter.c_str(),
				    verse.c_str(), statusName(r.status));
		}
		unresolved.insert(abbr + " " + chapter + ":" + verse + (chapterError ? " chapter" : " verse"));
	}
	std::printf(
		"cross references: %ld (abbreviation, chapter) pairs from %ld references, %zu unresolved (all known source typos)\n",
		pairs, references, unresolved.size());
	CHECK(pairs == 1152);
	CHECK(onlyOutOfRange);
	CHECK(unresolved == known);
	if (unresolved != known) {
		for (const auto &u : unresolved)
			if (!known.count(u))
				std::printf("  new unresolved: %s\n", u.c_str());
		for (const auto &k : known)
			if (!unresolved.count(k))
				std::printf("  no longer unresolved: %s\n", k.c_str());
	}
	// A wrong alias would make most references of an abbreviation fail: allow one typo, or at most 1 in 6.
	for (const auto &[abbr, p] : failuresPerAbbreviation) {
		const bool fine = p.first <= 1 || p.first * 6 <= p.second;
		CHECK(fine);
		if (!fine)
			std::printf("too many unresolved for '%s': %d of %d\n", abbr.c_str(), p.first, p.second);
	}
	std::set<int> distinct;
	for (const auto &[abbr, book] : bookOf)
		distinct.insert(book);
	CHECK(bookOf.size() == 61);
	CHECK(distinct.size() == 61); // 61 different abbreviations, 61 different books
}

// ---------- completion ----------

void testSuggestions()
{
	auto has = [](const std::vector<int> &v, int b) {
		return std::find(v.begin(), v.end(), b) != v.end();
	};
	const auto jo = suggestBooks("jo");
	CHECK(jo.size() >= 4 && has(jo, 18) && has(jo, 6) && has(jo, 29) && has(jo, 32));
	CHECK(suggestBooks("1 co") == std::vector<int>{46});
	CHECK(suggestBooks("I co") == std::vector<int>{46});
	CHECK(suggestBooks("gen") == std::vector<int>{1});
	CHECK(suggestBooks("Genèse 1:1") == std::vector<int>{1}); // the numbers after the name are ignored
	const auto first = suggestBooks("1");
	CHECK(first.size() == 8 && has(first, 62) && has(first, 46) && !has(first, 40));
	CHECK(suggestBooks("j", 3).size() == 3);
	CHECK(suggestBooks("").empty());
	CHECK(suggestBooks("zzz").empty());
	CHECK(suggestBooks("9").empty());
	CHECK(suggestBooks(std::string(1000, 'a')).empty());
}

// ---------- robustness and speed ----------

void testGarbageNeverCrashes()
{
	std::uint32_t state = 12345;
	auto next = [&state]() {
		state = state * 1664525u + 1013904223u;
		return state >> 8;
	};
	const char *const books[] = {"Jn",     "Jean", "1 Co", "II Co", "Ps",       "Jude",
				     "Genèse", "Ap",   "3 Jn", "Jo",    "1er Jean", "xyz"};
	const char *const seps[] = {":", ".", ",", " ", "-", "\xE2\x80\x93", " à ", " v ", ";", "::", " - "};
	const std::string alphabet = "Jn jean 1 2 3 16 :.,;- àéÉ\xE2\x80\x93 v ch 0 99999999";
	long okCount = 0, total = 0;
	for (int i = 0; i < 200000; ++i) {
		std::string q;
		if (next() % 4 == 0) { // pure noise: any character, any byte
			const unsigned len = next() % 40;
			for (unsigned k = 0; k < len; ++k)
				q.push_back(next() % 8 == 0 ? static_cast<char>(next() & 0xFF)
							    : alphabet[next() % alphabet.size()]);
		} else { // a plausible reference, then maybe damaged
			q = books[next() % 12];
			q += ' ' + std::to_string(next() % 160);
			for (unsigned parts = next() % 4; parts > 0; --parts)
				q += std::string(seps[next() % 11]) + std::to_string(next() % 60);
			if (next() % 4 == 0 && !q.empty())
				q[next() % q.size()] = static_cast<char>(next() & 0xFF);
		}
		const QueryResult r = resolveQuery(q, g_bible);
		++total;
		if (r.ok()) {
			++okCount;
			// Whatever was typed, an Ok passage is a real, ordered range of verses.
			const auto verses = passageVerses(r.passage, g_bible);
			if (verses.empty() || !g_bible.verse(verses.front()) || !g_bible.verse(verses.back())) {
				CHECK(false);
				std::printf("bad Ok passage for '%s'\n", q.c_str());
			}
		}
		(void)suggestBooks(q);
	}
	std::printf("garbage test: %ld queries, %ld understood\n", total, okCount);
	CHECK(okCount > 5000); // the generator really produces valid queries too

	CHECK(resolveQuery(std::string(100000, 'a'), g_bible).status == QueryStatus::Syntax);
	CHECK(resolveQuery(std::string(100000, '1'), g_bible).status == QueryStatus::Syntax);
	CHECK(resolveQuery(std::string(300, '\xFF'), g_bible).status == QueryStatus::Syntax);
	CHECK(resolveQuery(std::string("Jn 3:16\0 junk", 13), g_bible).status ==
	      QueryStatus::Syntax); // an embedded NUL is a space
}

void testSpeed()
{
	const auto start = std::chrono::steady_clock::now();
	long ok = 0;
	for (int i = 0; i < 100000; ++i)
		ok += resolveQuery("1 Co 13.4-7", g_bible).ok();
	const auto us =
		std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
	std::printf("100000 queries in %lld ms (%.2f us each)\n", static_cast<long long>(us / 1000), us / 100000.0);
	CHECK(ok == 100000);
	CHECK(us < 10'000'000); // loose bound; the objective (50 ms per search) is measured in Milestone 10
}

} // namespace

int main()
{
	const auto loaded = g_bible.loadFromFile(VYRA_LSG_PATH);
	if (!loaded.ok()) {
		std::printf("FAIL cannot load %s: %s\n", VYRA_LSG_PATH, loaded.detail.c_str());
		return 1;
	}

	testCatalog();
	testAbbreviationsOfTheSegond();
	testSameVerseManyWays();
	testRangesAndNumberedBooks();
	testChaptersAndCrossChapterRanges();
	testSingleChapterBooks();
	testAccentsAndLanguages();
	testUniquePrefixes();
	testErrorsAndTypingInProgress();
	testAmbiguousBooks();
	testBookMissingFromModule();
	testPassageVersesAndFormat();
	testRoundTripWholeBible();
	testSegondCrossReferences();
	testSuggestions();
	testGarbageNeverCrashes();
	testSpeed();
	return vyra::testing::finish();
}
