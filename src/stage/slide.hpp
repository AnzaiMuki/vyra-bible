/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <string>
#include <vector>

namespace vyra::stage {

struct SlideVerse {
	int chapter = 0;
	int verse = 0;
	std::string text;
};

/** Everything needed to draw a passage: its printed reference and its verses, in reading order. */
struct Slide {
	std::string reference;
	std::vector<SlideVerse> verses;
};

} // namespace vyra::stage
