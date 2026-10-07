/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

// Test helper: writes, one JSON frame per line, the pages most likely to overflow the overlay (the fullest
// pages of the whole Bible, for each theme) plus a regular sample of all the others.
// Usage: frames_dump <output file>      VYRA_LSG_PATH as in the other tests.

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <vector>

#include "src/bible/bible_module.hpp"
#include "src/overlay/overlay_frame.hpp"
#include "src/stage/stage_controller.hpp"

using namespace vyra;

int main(int argc, char **argv)
{
	if (argc < 2)
		return 2;
	bible::BibleModule bible;
	if (!bible.loadFromFile(VYRA_LSG_PATH).ok())
		return 3;

	std::ofstream out(argv[1], std::ios::binary);
	stage::StageController s(bible);
	unsigned long long revision = 0;
	std::size_t total = 0;

	for (const stage::Theme theme : {stage::Theme::LowerThird, stage::Theme::FullScreen, stage::Theme::Minimal}) {
		s.setTheme(theme);
		struct Item {
			std::size_t chars;
			std::string json;
		};
		std::vector<Item> all;
		for (int book = 1; book <= 66; ++book) {
			for (int chapter = 1; chapter <= bible.chapterCount(book); ++chapter) {
				s.showInPreview({book, chapter, 1, chapter, bible.verseCount(book, chapter)});
				s.takeOnAir();
				do {
					const overlay::Frame f = overlay::makeFrame(s, ++revision);
					std::size_t chars = 0;
					for (const auto &v : f.verses)
						chars += stage::charCount(v.text) + 3;
					all.push_back({chars, overlay::toJson(f)});
				} while (s.nextPage());
			}
		}
		total += all.size();
		std::vector<std::size_t> order(all.size());
		for (std::size_t i = 0; i < order.size(); ++i)
			order[i] = i;
		std::stable_sort(order.begin(), order.end(),
				 [&](std::size_t a, std::size_t b) { return all[a].chars > all[b].chars; });
		std::vector<bool> chosen(all.size(), false);
		for (std::size_t i = 0; i < std::min<std::size_t>(60, order.size()); ++i)
			chosen[order[i]] = true;
		for (std::size_t i = 0; i < all.size(); i += 400)
			chosen[i] = true;
		for (std::size_t i = 0; i < all.size(); ++i)
			if (chosen[i])
				out << all[i].json << '\n';
	}
	std::printf("%zu pages in all themes\n", total);
	return 0;
}
