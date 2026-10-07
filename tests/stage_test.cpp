/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

// Tests of the Preview / Program rules. VYRA_LSG_PATH = data/bibles/lsg1910.tsv

#include <cstdio>
#include <sstream>
#include <string>

#include "src/bible/bible_module.hpp"
#include "src/search/passage_query.hpp"
#include "src/stage/stage_controller.hpp"
#include "tests/check.hpp"

using namespace vyra::bible;
using namespace vyra::search;
using namespace vyra::stage;

namespace {

BibleModule g_bible;

Passage resolved(const char *query, const BibleModule &m = g_bible)
{
	const QueryResult r = resolveQuery(query, m);
	CHECK(r.ok());
	return r.passage;
}

bool same(const Passage &a, const Passage &b)
{
	return a.book == b.book && a.startChapter == b.startChapter && a.startVerse == b.startVerse &&
	       a.endChapter == b.endChapter && a.endVerse == b.endVerse;
}

void testInitialState()
{
	StageController s(g_bible);
	CHECK(!s.preview());
	CHECK(!s.program());
	CHECK(!s.programLive());
	CHECK(!s.hasNext() && !s.hasPrevious());
	CHECK(!s.takeOnAir());      // nothing to send
	CHECK(!s.hideProgram());    // nothing on the air
	CHECK(!s.previewNext());
	CHECK(!s.previewPrevious());
}

void testPreviewThenOnAir()
{
	StageController s(g_bible);
	CHECK(s.showInPreview(resolved("Jn 3:16")));
	CHECK(!s.program());             // preview never touches the program
	CHECK(s.takeOnAir());
	CHECK(s.programLive());
	CHECK(same(*s.program(), *s.preview()));
}

void testChangingPreviewNeverTouchesProgram()
{
	StageController s(g_bible);
	s.showInPreview(resolved("Jn 3:16"));
	s.takeOnAir();
	const Passage onAir = *s.program();

	CHECK(s.showInPreview(resolved("Ps 23")));
	CHECK(s.previewNext());
	CHECK(s.previewPrevious());
	CHECK(s.previewNext());
	CHECK(s.programLive());
	CHECK(same(*s.program(), onAir));       // the live display never moved
	CHECK(!same(*s.preview(), onAir));

	CHECK(s.takeOnAir());                   // only this changes the program
	CHECK(!same(*s.program(), onAir));
	CHECK(same(*s.program(), *s.preview()));
}

void testHideKeepsThePassage()
{
	StageController s(g_bible);
	s.showInPreview(resolved("Jn 3:16"));
	s.takeOnAir();
	CHECK(s.hideProgram());
	CHECK(!s.programLive());
	CHECK(s.program().has_value());          // still held, ready to be shown again
	CHECK(!s.hideProgram());                 // already hidden
	CHECK(s.takeOnAir());                    // preview is still there
	CHECK(s.programLive());
}

void testHiddenProgramIsNotChangedByPreview()
{
	StageController s(g_bible);
	s.showInPreview(resolved("Jn 3:16"));
	s.takeOnAir();
	s.hideProgram();
	const Passage held = *s.program();
	s.showInPreview(resolved("Gn 1:1"));
	CHECK(!s.programLive());
	CHECK(same(*s.program(), held));
}

void testRefusedPassagesChangeNothing()
{
	StageController s(g_bible);
	s.showInPreview(resolved("Jn 3:16"));
	const Passage before = *s.preview();
	const Passage bad[] = {
		{},                      // all zero
		{43, 3, 16, 3, 15},      // reversed
		{43, 3, 16, 3, 99},      // verse out of range
		{43, 99, 1, 99, 1},      // chapter out of range
		{43, 3, 1, 2, 1},        // reversed across chapters
		{67, 1, 1, 1, 1},        // no such book
		{-1, 1, 1, 1, 1},
	};
	for (const Passage &p : bad) {
		CHECK(!s.showInPreview(p));
		CHECK(same(*s.preview(), before));
	}
}

void testNavigationCrossesChaptersAndBooks()
{
	StageController s(g_bible);

	s.showInPreview(resolved("Jn 3:36"));        // last verse of the chapter
	CHECK(s.previewNext());
	CHECK(same(*s.preview(), resolved("Jn 4:1")));
	CHECK(s.previewPrevious());
	CHECK(same(*s.preview(), resolved("Jn 3:36")));

	s.showInPreview(resolved("Mal 4:6"));        // last verse of a book
	CHECK(s.previewNext());
	CHECK(same(*s.preview(), resolved("Mt 1:1")));
	CHECK(s.previewPrevious());
	CHECK(same(*s.preview(), resolved("Mal 4:6")));

	s.showInPreview(resolved("Gn 1:1"));         // the very first verse
	CHECK(!s.hasPrevious());
	CHECK(!s.previewPrevious());
	CHECK(same(*s.preview(), resolved("Gn 1:1")));

	s.showInPreview(resolved("Ap 22:21"));       // the very last verse
	CHECK(!s.hasNext());
	CHECK(!s.previewNext());
	CHECK(same(*s.preview(), resolved("Ap 22:21")));
}

void testNavigationFromARange()
{
	StageController s(g_bible);
	s.showInPreview(resolved("Jn 3:16-18"));
	CHECK(s.previewNext());                      // the verse after the end
	CHECK(same(*s.preview(), resolved("Jn 3:19")));

	s.showInPreview(resolved("Jn 3:16-18"));
	CHECK(s.previewPrevious());                  // the verse before the start
	CHECK(same(*s.preview(), resolved("Jn 3:15")));

	s.showInPreview(resolved("Jn 3"));           // a whole chapter
	CHECK(s.previewNext());
	CHECK(same(*s.preview(), resolved("Jn 4:1")));
}

void testWalkThroughTheWholeBible()
{
	StageController s(g_bible);
	s.showInPreview(resolved("Gn 1:1"));
	std::size_t forward = 1;
	while (s.previewNext())
		++forward;
	CHECK(forward == g_bible.verseCount());
	CHECK(same(*s.preview(), resolved("Ap 22:21")));

	std::size_t backward = 1;
	while (s.previewPrevious())
		++backward;
	CHECK(backward == g_bible.verseCount());
	CHECK(same(*s.preview(), resolved("Gn 1:1")));
}

void testModuleWithMissingBooks()
{
	// Only Genesis (1), Matthew (40) and Jude (65): navigation must skip the books that are absent.
	const std::string text =
		"#vyra-module\t1\n#code\tTST\n#name\tTest\n#language\tfr\n#license\tPD\n#source\ttest\n"
		"1\t1\t1\tG11\n1\t1\t2\tG12\n40\t1\t1\tM11\n65\t1\t1\tJ1\n65\t1\t2\tJ2\n";
	BibleModule small;
	std::istringstream in(text);
	CHECK(small.loadFromStream(in).ok());

	StageController s(small);
	s.showInPreview(resolved("Gn 1:2", small));
	CHECK(s.previewNext());
	CHECK(same(*s.preview(), resolved("Mt 1:1", small)));
	CHECK(s.previewNext());
	CHECK(same(*s.preview(), resolved("Jude 1", small)));
	CHECK(s.previewNext());
	CHECK(s.hasNext() == false);
	CHECK(s.previewPrevious());
	CHECK(s.previewPrevious());
	CHECK(same(*s.preview(), resolved("Mt 1:1", small)));
	CHECK(s.previewPrevious());
	CHECK(same(*s.preview(), resolved("Gn 1:2", small)));
}

void testSlide()
{
	StageController s(g_bible);
	const Slide one = s.slide(resolved("Jn 3:16"));
	CHECK(one.reference == "Jean 3:16");
	CHECK(one.verses.size() == 1);
	CHECK(one.verses[0].chapter == 3 && one.verses[0].verse == 16);
	CHECK(one.verses[0].text == std::string(*g_bible.verse({43, 3, 16})));

	const Slide range = s.slide(resolved("Jn 3:36-4:2"));
	CHECK(range.verses.size() == 3);
	CHECK(range.verses[0].chapter == 3 && range.verses[0].verse == 36);
	CHECK(range.verses[1].chapter == 4 && range.verses[1].verse == 1);
	CHECK(range.verses[2].chapter == 4 && range.verses[2].verse == 2);

	CHECK(s.slide(resolved("Ps 119")).verses.size() == 176);
	CHECK(s.slide(Passage{}).verses.empty());
}

void testThemeAndPages()
{
	StageController s(g_bible);
	CHECK(s.theme() == Theme::LowerThird);
	CHECK(std::string(themeName(Theme::LowerThird)) == "lower" && std::string(themeName(Theme::FullScreen)) == "full" &&
	      std::string(themeName(Theme::Minimal)) == "minimal");
	CHECK(pageBudget(Theme::FullScreen) > pageBudget(Theme::LowerThird));

	// Nothing on the air: no pages, no page changes.
	CHECK(s.programPages().empty() && !s.nextPage() && !s.previousPage());

	s.showInPreview(resolved("Ps 119"));
	CHECK(s.programPages().empty());                       // the preview is not paginated for the program
	s.takeOnAir();
	const std::size_t lower = s.programPages().size();
	CHECK(lower > 10 && s.programPage() == 0);

	// Pages move the PROGRAM only, one at a time, and stop at both ends.
	CHECK(!s.previousPage());
	CHECK(s.nextPage() && s.programPage() == 1);
	CHECK(s.nextPage() && s.programPage() == 2);
	CHECK(s.previousPage() && s.programPage() == 1);
	while (s.nextPage()) {}
	CHECK(s.programPage() == lower - 1 && !s.nextPage());
	CHECK(s.preview().has_value() && s.preview()->endChapter == 119); // preview untouched

	// A bigger screen holds more: fewer pages; the program goes back to page 1; what is on the air is the same passage.
	const Passage onAir = *s.program();
	s.setTheme(Theme::FullScreen);
	CHECK(s.theme() == Theme::FullScreen && s.programPage() == 0);
	CHECK(s.programPages().size() < lower && s.programLive() && same(*s.program(), onAir));
	s.setTheme(Theme::FullScreen);                          // same theme: nothing moves
	CHECK(s.nextPage()); s.setTheme(Theme::FullScreen); CHECK(s.programPage() == 1);

	// A new passage on the air always starts at page 1.
	s.showInPreview(resolved("Ps 23")); s.takeOnAir();
	CHECK(s.programPage() == 0);

	// Hidden: pages cannot be turned; shown again, page 1.
	s.showInPreview(resolved("Ps 119")); s.takeOnAir(); s.nextPage();
	s.hideProgram();
	CHECK(!s.nextPage() && !s.previousPage());
	s.takeOnAir();
	CHECK(s.programPage() == 0 && s.programLive());

	// A single short verse is a single page.
	s.showInPreview(resolved("Jn 3:16")); s.takeOnAir();
	CHECK(s.programPages().size() == 1 && !s.nextPage());
}

} // namespace

int main()
{
	const auto loaded = g_bible.loadFromFile(VYRA_LSG_PATH);
	if (!loaded.ok()) {
		std::printf("FAIL cannot load %s: %s\n", VYRA_LSG_PATH, loaded.detail.c_str());
		return 1;
	}
	testInitialState();
	testPreviewThenOnAir();
	testChangingPreviewNeverTouchesProgram();
	testHideKeepsThePassage();
	testHiddenProgramIsNotChangedByPreview();
	testRefusedPassagesChangeNothing();
	testNavigationCrossesChaptersAndBooks();
	testNavigationFromARange();
	testWalkThroughTheWholeBible();
	testModuleWithMissingBooks();
	testSlide();
	testThemeAndPages();
	return vyra::testing::finish();
}
