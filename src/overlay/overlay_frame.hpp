/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

/*
 * What the overlay page must show, as data. Independent of OBS and Qt.
 * One Frame is the complete picture: the page never has to guess what changed.
 */

#include <string>
#include <vector>

#include "src/stage/stage_controller.hpp"

namespace vyra::overlay {

struct Frame {
	unsigned long long revision = 0; // grows with every publication; lets a client ignore old frames
	bool visible = false;            // false: the page shows nothing (but keeps nothing stale either)
	std::string theme = "lower";     // "lower", "full" or "minimal"
	int page = 1;                    // 1-based, of @c pages
	int pages = 1;
	std::string reference;                // printed reference of the whole passage
	std::vector<stage::PageVerse> verses; // the verses of this page only; meaningful only when visible
};

/** The frame that shows what is on the PROGRAM right now (visible only if it is on the air). */
Frame makeFrame(const stage::StageController &controller, unsigned long long revision);

/**
 * JSON of a frame: {"rev":1,"visible":true,"theme":"lower","page":1,"pages":1,"reference":"Jean 3:16",
 *                    "verses":[{"c":3,"v":16,"t":"..."}]}   (a verse cut over pages carries "k":true on its later pieces)
 * Always valid JSON, whatever the text (quotes, backslashes, control characters, any UTF-8).
 * '<' '>' '&' and U+2028/2029 are escaped so the JSON can also be embedded in HTML safely.
 */
std::string toJson(const Frame &frame);

} // namespace vyra::overlay
