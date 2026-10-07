/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

// Tests of the pagination. VYRA_LSG_PATH = data/bibles/lsg1910.tsv

#include <cstdio>
#include <string>

#include "src/bible/bible_module.hpp"
#include "src/search/passage_query.hpp"
#include "src/stage/paginator.hpp"
#include "src/stage/stage_controller.hpp"
#include "tests/check.hpp"

using namespace vyra::bible;
using namespace vyra::stage;

namespace {

BibleModule g_bible;

Slide slideOf(const char *query)
{
	StageController s(g_bible);
	s.showInPreview(vyra::search::resolveQuery(query, g_bible).passage);
	return s.slide(*s.preview());
}

Slide synthetic(std::initializer_list<std::string> texts)
{
	Slide s;
	s.reference = "T 1";
	int v = 1;
	for (const auto &t : texts)
		s.verses.push_back({1, v++, t});
	return s;
}

// Joining the pieces of every verse must give back the original text, in order, nothing else.
bool lossless(const Slide &slide, const std::vector<Page> &pages)
{
	std::size_t at = 0; // index in slide.verses
	std::string rebuilt;
	for (const Page &p : pages) {
		if (p.verses.empty())
			return false;
		for (const PageVerse &pv : p.verses) {
			if (!pv.continuation) {
				if (!rebuilt.empty() || at != 0) {
					if (rebuilt != slide.verses[at - 1].text)
						return false;
					rebuilt.clear();
				}
				if (at >= slide.verses.size())
					return false;
				if (pv.verse != slide.verses[at].verse || pv.chapter != slide.verses[at].chapter)
					return false;
				++at;
			}
			rebuilt += pv.text;
		}
	}
	return at == slide.verses.size() && (slide.verses.empty() || rebuilt == slide.verses[at - 1].text);
}

bool valid(const std::string &utf8)
{
	std::size_t i = 0;
	while (i < utf8.size()) {
		const unsigned char c = static_cast<unsigned char>(utf8[i]);
		const int n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 0;
		if (!n || i + static_cast<std::size_t>(n) > utf8.size())
			return false;
		for (int k = 1; k < n; ++k)
			if ((static_cast<unsigned char>(utf8[i + static_cast<std::size_t>(k)]) & 0xC0) != 0x80)
				return false;
		i += static_cast<std::size_t>(n);
	}
	return true;
}

void testBasics()
{
	CHECK(charCount("abc") == 3 && charCount("\xC3\xA9\xC3\xA8") == 2 && charCount("") == 0);
	CHECK(paginate(Slide{}, 100).empty());

	const Slide one = synthetic({"Car Dieu a tant aime le monde"});
	const auto p1 = paginate(one, 100);
	CHECK(p1.size() == 1 && p1[0].verses.size() == 1 && !p1[0].verses[0].continuation);

	// Whole verses are kept together while they fit, then move to the next page.
	const Slide three = synthetic({std::string(40, 'a'), std::string(40, 'b'), std::string(40, 'c')});
	const auto p3 = paginate(three, 90);
	CHECK(p3.size() == 2 && p3[0].verses.size() == 2 && p3[1].verses.size() == 1);
	CHECK(paginate(three, 1000).size() == 1);
	CHECK(paginate(three, 40).size() == 3);   // one verse per page: each exactly fits
	CHECK(lossless(three, p3));
}

void testLongVerseIsCutAtSpaces()
{
	std::string words;
	for (int i = 0; i < 60; ++i)
		words += "mot" + std::to_string(i) + " ";
	words.pop_back();
	const Slide s = synthetic({"debut", words, "fin"});
	const auto pages = paginate(s, 100);
	CHECK(pages.size() > 3);
	CHECK(lossless(s, pages));
	for (const Page &p : pages) {
		for (const PageVerse &pv : p.verses) {
			CHECK(charCount(pv.text) <= 100);
			CHECK(!pv.text.empty());
		}
	}
	// every piece starts on a word ("mot...") and every piece but the last ends after a space
	{
		std::vector<std::string> pieces;
		for (const Page &p : pages)
			for (const PageVerse &pv : p.verses)
				if (pv.verse == 2)
					pieces.push_back(pv.text);
		CHECK(pieces.size() >= 3);
		for (std::size_t i = 0; i < pieces.size(); ++i) {
			CHECK(pieces[i].compare(0, 3, "mot") == 0);
			if (i + 1 < pieces.size())
				CHECK(pieces[i].back() == ' ');
		}
	}
	// the first piece is not a continuation, the others are
	bool seen = false;
	for (const Page &p : pages)
		for (const PageVerse &pv : p.verses)
			if (pv.verse == 2) {
				CHECK(pv.continuation == seen);
				seen = true;
			}
	// A verse alone on its pages: never mixed with another verse
	for (const Page &p : pages)
		for (const PageVerse &pv : p.verses)
			if (pv.verse == 2)
				CHECK(p.verses.size() == 1);
}

void testWordWithoutSpaceAndAccents()
{
	// A word longer than the page, in UTF-8 with 2-byte letters: cut on a letter boundary.
	std::string word;
	for (int i = 0; i < 90; ++i)
		word += "\xC3\xA9";
	const Slide s = synthetic({word});
	const auto pages = paginate(s, 40);
	CHECK(pages.size() == 3);
	CHECK(lossless(s, pages));
	for (const Page &p : pages)
		for (const PageVerse &pv : p.verses)
			CHECK(valid(pv.text) && charCount(pv.text) <= 40);
}

void testWholeBible()
{
	// Every chapter of the Bible, at three budgets: lossless, valid UTF-8, within budget, no empty page.
	std::size_t chapters = 0, pagesTotal = 0, longest = 0;
	bool ok = true;
	for (const std::size_t budget : {140u, 260u, 700u}) {
		for (int book = 1; book <= 66; ++book) {
			for (int chapter = 1; chapter <= g_bible.chapterCount(book); ++chapter) {
				StageController s(g_bible);
				s.showInPreview({book, chapter, 1, chapter, g_bible.verseCount(book, chapter)});
				const Slide slide = s.slide(*s.preview());
				const auto pages = paginate(slide, budget);
				++chapters;
				pagesTotal += pages.size();
				if (pages.empty() || !lossless(slide, pages))
					ok = false;
				for (const Page &p : pages) {
					std::size_t used = 0;
					for (const PageVerse &pv : p.verses) {
						used += charCount(pv.text) + 3;
						if (!valid(pv.text) || pv.text.empty())
							ok = false;
					}
					longest = std::max(longest, used);
					// a page holding several pieces never exceeds the budget by more than 3 chars per verse
					if (used > budget + 3 * p.verses.size())
						ok = false;
				}
			}
		}
	}
	CHECK(ok);
	CHECK(chapters == 3u * 1189u);
	std::printf("pagination: %zu chapters x budgets, %zu pages, fullest page %zu characters\n", chapters, pagesTotal, longest);
}

void testPsalm119()
{
	const Slide s = slideOf("Ps 119");
	const auto pages = paginate(s, 260);
	CHECK(pages.size() > 10 && pages.size() < 60);
	CHECK(lossless(s, pages));
	CHECK(pages.front().verses.front().verse == 1);
	CHECK(pages.back().verses.back().verse == 176);
}

} // namespace

int main()
{
	const auto loaded = g_bible.loadFromFile(VYRA_LSG_PATH);
	if (!loaded.ok()) {
		std::printf("FAIL cannot load %s: %s\n", VYRA_LSG_PATH, loaded.detail.c_str());
		return 1;
	}
	testBasics();
	testLongVerseIsCutAtSpaces();
	testWordWithoutSpaceAndAccents();
	testWholeBible();
	testPsalm119();
	return vyra::testing::finish();
}
