/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

// Stress and robustness tests: random and hostile input must never crash, hang or break the rules.
// Deterministic (fixed seeds), so a failure can be replayed. Meant to run under ASan/UBSan.
// VYRA_LSG_PATH = data/bibles/lsg1910.tsv

#include <chrono>
#include <cstdio>
#include <random>
#include <sstream>
#include <vector>
#include <string>

#include "src/bible/bible_module.hpp"
#include "src/library/passage_library.hpp"
#include "src/overlay/overlay_frame.hpp"
#include "src/search/passage_query.hpp"
#include "src/stage/paginator.hpp"
#include "src/stage/stage_controller.hpp"
#include "tests/check.hpp"

using namespace vyra;

namespace {

bible::BibleModule g_bible;

std::string randomBytes(std::mt19937 &rng, std::size_t n)
{
	std::string s(n, '\0');
	for (char &c : s)
		c = static_cast<char>(rng() & 0xff);
	return s;
}

/** Text that looks like queries: pieces of real ones, glued at random. */
std::string randomQuery(std::mt19937 &rng)
{
	static const char *const pieces[] = {"Jn",
					     "Jean",
					     "Ps",
					     "Psaumes",
					     "1",
					     "3",
					     "16",
					     "119",
					     "176",
					     ":",
					     "-",
					     " ",
					     "  ",
					     "v",
					     ";",
					     ",",
					     "–",
					     "—",
					     "99999999999999999999",
					     "0",
					     "-1",
					     "Ap",
					     "Gn",
					     "é",
					     "è",
					     "\xC3",
					     "\xE2\x80",
					     "\xFF",
					     ".",
					     "1 Co",
					     "2 R",
					     "Cant",
					     "Ex",
					     "\t",
					     "\n",
					     "\r",
					     "%",
					     "<b>",
					     "\"",
					     "'"};
	std::string q;
	const int count = static_cast<int>(rng() % 8);
	for (int i = 0; i < count; ++i)
		q += pieces[rng() % (sizeof(pieces) / sizeof(*pieces))];
	return q;
}

search::Passage randomPassage(std::mt19937 &rng)
{
	return {static_cast<int>(rng() % 70) - 2, static_cast<int>(rng() % 200) - 2, static_cast<int>(rng() % 200) - 2,
		static_cast<int>(rng() % 200) - 2, static_cast<int>(rng() % 200) - 2};
}

void testQueries()
{
	std::mt19937 rng(1);
	std::size_t ok = 0;
	for (int i = 0; i < 200000; ++i) {
		const std::string q = (i % 3 == 0) ? randomBytes(rng, rng() % 40) : randomQuery(rng);
		const search::QueryResult r = search::resolveQuery(q, g_bible);
		if (r.ok()) {
			++ok;
			// What the engine calls valid must really exist: this is what protects the live display.
			CHECK(library::isValidPassage(r.passage, g_bible));
			CHECK(!search::passageVerses(r.passage, g_bible).empty());
			CHECK(!search::formatPassage(r.passage, g_bible).empty());
		}
		const search::Passage context{43, 3, 16, 3, 16};
		const search::QueryResult rel = search::resolveQuery(q, g_bible, &context);
		if (rel.ok())
			CHECK(library::isValidPassage(rel.passage, g_bible));
	}
	CHECK(ok > 100); // the generator does produce valid queries, the loop above is not vacuous
	// Very long inputs: no quadratic blow-up, no crash.
	const auto t = std::chrono::steady_clock::now();
	search::resolveQuery(std::string(1000000, '1'), g_bible);
	search::resolveQuery(std::string(1000000, ' '), g_bible);
	search::resolveQuery(std::string(200000, '-') + "1", g_bible);
	search::resolveQuery("Jn 3:" + std::string(500000, '1'), g_bible);
	const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t).count();
	std::printf("long inputs: %.0f ms\n", ms);
	CHECK(ms < 5000);
}

void testPassagesNeverCrash()
{
	std::mt19937 rng(2);
	stage::StageController s(g_bible);
	for (int i = 0; i < 100000; ++i) {
		const search::Passage p = randomPassage(rng);
		const bool valid = library::isValidPassage(p, g_bible);
		const bool accepted = s.showInPreview(p);
		// A passage the Bible does not fully contain must be refused and change nothing.
		if (!valid)
			CHECK(!accepted || library::isValidPassage(*s.preview(), g_bible));
		if (accepted)
			CHECK(valid);
	}
}

void testStageRandomOperations()
{
	std::mt19937 rng(3);
	stage::StageController s(g_bible);
	std::optional<search::Passage> lastProgram;
	bool wasLive = false;
	for (int i = 0; i < 200000; ++i) {
		const auto previewBefore = s.preview();
		const auto programBefore = s.program();
		const bool liveBefore = s.programLive();
		const std::size_t pageBefore = s.programPage();
		const int op = static_cast<int>(rng() % 10);
		bool changed = false;
		switch (op) {
		case 0:
			changed = s.showInPreview(search::resolveQuery(randomQuery(rng), g_bible).passage);
			break;
		case 1:
			changed = s.previewNext();
			break;
		case 2:
			changed = s.previewPrevious();
			break;
		case 3:
			changed = s.takeOnAir();
			break;
		case 4:
			changed = s.hideProgram();
			break;
		case 5:
			changed = s.nextPage();
			break;
		case 6:
			changed = s.previousPage();
			break;
		case 7:
			s.setTheme(static_cast<stage::Theme>(rng() % 3));
			break;
		case 8:
			s.showInPreview({1 + static_cast<int>(rng() % 66), 1, 1, 1, 1});
			break;
		default:
			s.showInPreview(randomPassage(rng));
			break;
		}
		(void)changed;
		// Invariants, after every single operation.
		const auto same = [](const std::optional<search::Passage> &a, const std::optional<search::Passage> &b) {
			return a.has_value() == b.has_value() && (!a || *a == *b);
		};
		if (s.preview())
			CHECK(library::isValidPassage(*s.preview(), g_bible));
		if (s.program())
			CHECK(library::isValidPassage(*s.program(), g_bible));
		// Only taking on air changes the program passage.
		if (op != 3)
			CHECK(same(programBefore, s.program()));
		// Only these operations may change the preview.
		if (op == 3 || op == 4 || op == 5 || op == 6 || op == 7)
			CHECK(same(previewBefore, s.preview()));
		// Pages: always a valid index, only moved by the page operations, theme or on air.
		if (s.program()) {
			CHECK(!s.programPages().empty());
			CHECK(s.programPage() < s.programPages().size());
		} else {
			CHECK(s.programPages().empty());
		}
		if (op != 3 && op != 5 && op != 6 && op != 7)
			CHECK(s.programPage() == pageBefore || !s.program());
		// Hiding then taking on air again shows the same passage (the program is kept, not lost).
		if (op == 4 && liveBefore)
			CHECK(!s.programLive() && s.program().has_value());
		if (op == 5 || op == 6)
			CHECK(!changed || liveBefore); // pages move only while the program is live
		lastProgram = s.program();
		wasLive = s.programLive();
	}
	(void)lastProgram;
	(void)wasLive;
	// The frame sent to the overlay is always well formed, whatever happened.
	const overlay::Frame f = overlay::makeFrame(s, 1);
	CHECK(!overlay::toJson(f).empty());
}

void testPaginatorHostile()
{
	std::mt19937 rng(4);
	for (int i = 0; i < 3000; ++i) {
		stage::Slide slide;
		slide.reference = randomBytes(rng, rng() % 20);
		const int verses = static_cast<int>(rng() % 12);
		std::size_t totalLetters = 0;
		for (int v = 0; v < verses; ++v) {
			std::string text;
			const int words = static_cast<int>(rng() % 90);
			for (int w = 0; w < words; ++w) {
				text += std::string(1 + rng() % 14, static_cast<char>('a' + rng() % 26));
				text += (rng() % 7 == 0) ? "  " : " ";
			}
			if (rng() % 11 == 0)
				text += std::string(rng() % 3000, 'x'); // a "word" longer than any page
			if (rng() % 13 == 0)
				text += "\xC3\xA9\xE2\x80\x99\xE4\xB8\xAD"; // multi-byte letters
			totalLetters += text.size();
			slide.verses.push_back({1, v + 1, text});
		}
		const std::size_t budget = 20 + rng() % 900;
		const auto pages = stage::paginate(slide, budget);
		if (verses > 0)
			CHECK(!pages.empty());
		// Lossless: for every verse, the pieces put end to end hold exactly its letters (spaces may be re-cut).
		const auto letters = [](const std::string &t) {
			std::string r;
			for (char c : t)
				if (c != ' ')
					r += c;
			return r;
		};
		std::vector<std::string> rebuilt(slide.verses.size());
		for (const stage::Page &page : pages) {
			CHECK(!page.verses.empty());
			for (const stage::PageVerse &pv : page.verses) {
				CHECK(pv.verse >= 1 && static_cast<std::size_t>(pv.verse) <= slide.verses.size());
				if (pv.verse >= 1 && static_cast<std::size_t>(pv.verse) <= slide.verses.size())
					rebuilt[pv.verse - 1] += letters(pv.text);
				const unsigned char first = pv.text.empty() ? 0
									    : static_cast<unsigned char>(pv.text[0]);
				CHECK((first & 0xC0) != 0x80); // never cut inside a UTF-8 sequence
			}
		}
		for (std::size_t v = 0; v < slide.verses.size(); ++v)
			CHECK(rebuilt[v] == letters(slide.verses[v].text));
		(void)totalLetters;
	}
}

void testLibraryFuzz()
{
	std::mt19937 rng(5);
	for (int i = 0; i < 20000; ++i) {
		library::PassageLibrary lib;
		std::string text = (i % 2) ? "VYRA-LIBRARY 1\n" : "";
		const int lines = static_cast<int>(rng() % 10);
		for (int l = 0; l < lines; ++l) {
			if (rng() % 3 == 0) {
				text += randomBytes(rng, rng() % 30);
			} else {
				const search::Passage p = randomPassage(rng);
				text += std::string(1, "HFXh"[rng() % 4]) + " " + std::to_string(p.book) + " " +
					std::to_string(p.startChapter) + " " + std::to_string(p.startVerse) + " " +
					std::to_string(p.endChapter) + " " + std::to_string(p.endVerse);
			}
			text += (rng() % 5 == 0) ? "\r\n" : "\n";
		}
		lib.parse(text, g_bible);
		for (const auto &p : lib.history())
			CHECK(library::isValidPassage(p, g_bible));
		for (const auto &p : lib.favorites())
			CHECK(library::isValidPassage(p, g_bible));
		CHECK(lib.history().size() <= library::PassageLibrary::kMaxHistory);
		CHECK(lib.favorites().size() <= library::PassageLibrary::kMaxFavorites);
		// What we write we can read back unchanged.
		library::PassageLibrary again;
		again.parse(lib.serialize(), g_bible);
		CHECK(again.serialize() == lib.serialize());
	}
	// Huge numbers must not overflow into something valid.
	library::PassageLibrary lib;
	const auto r = lib.parse("VYRA-LIBRARY 1\nH 4294967299 3 16 3 16\nH 43 4294967299 16 3 16\nH 43 3 16 3 16 \n",
				 g_bible);
	CHECK(lib.history().size() <= 1);
	(void)r;
}

void testBibleLoaderHostile()
{
	std::mt19937 rng(6);
	const std::string header = "#vyra-module\t1\n#code\tT\n#name\tT\n#language\tfr\n#license\tx\n#source\tx\n";
	for (int i = 0; i < 20000; ++i) {
		std::string body;
		const int lines = static_cast<int>(rng() % 8);
		for (int l = 0; l < lines; ++l) {
			if (rng() % 3 == 0)
				body += randomBytes(rng, rng() % 40);
			else
				body += std::to_string(static_cast<int>(rng() % 70) - 1) + "\t" +
					std::to_string(static_cast<int>(rng() % 5) - 1) + "\t" +
					std::to_string(static_cast<int>(rng() % 5) - 1) + "\t" +
					randomBytes(rng, rng() % 20);
			body += '\n';
		}
		bible::BibleModule m;
		std::istringstream in(((i % 4) ? header : std::string()) + body);
		const bible::LoadResult r = m.loadFromStream(in);
		if (!r.ok())
			CHECK(m.empty()); // a failed load leaves nothing half loaded
		else
			CHECK(m.verseCount() > 0);
		m.verse({static_cast<int>(rng() % 70), static_cast<int>(rng() % 5), static_cast<int>(rng() % 5)});
	}
}

} // namespace

int main()
{
	CHECK(g_bible.loadFromFile(VYRA_LSG_PATH).ok());
	testQueries();
	testPassagesNeverCrash();
	testStageRandomOperations();
	testPaginatorHostile();
	testLibraryFuzz();
	testBibleLoaderHostile();
	return testing::finish();
}
