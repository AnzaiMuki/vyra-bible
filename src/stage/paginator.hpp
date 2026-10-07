/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

/*
 * Cuts a long passage into screens ("pages") that fit the room of a theme.
 * Independent of OBS and Qt. The cut is made here, not in the browser, so that the dock knows how many
 * pages there are and the operator can move through them.
 *
 * Rules: a page holds whole verses while they fit; a verse that is alone too long for a page is cut at a
 * space (never inside a word) and its pieces are marked as continuations; no page is ever empty; no text
 * is ever lost or changed (joining the pieces gives back the verse).
 */

#include <string>
#include <vector>

#include "src/stage/slide.hpp"

namespace vyra::stage {

struct PageVerse {
	int chapter = 0;
	int verse = 0;
	std::string text;
	bool continuation = false; // second or later piece of a verse cut over several pages
};

struct Page {
	std::vector<PageVerse> verses;
};

/**
 * @param maxChars  budget of one page in characters (UTF-8 letters, not bytes), at least 20.
 * A page may exceed the budget only by the words that a verse number and spaces add (a few characters).
 */
std::vector<Page> paginate(const Slide &slide, std::size_t maxChars);

/** Number of characters (code points) of a UTF-8 text. */
std::size_t charCount(const std::string &utf8);

} // namespace vyra::stage
