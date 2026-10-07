/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "src/stage/paginator.hpp"

#include <algorithm>

namespace vyra::stage {

std::size_t charCount(const std::string &utf8)
{
	std::size_t n = 0;
	for (const char c : utf8)
		if ((static_cast<unsigned char>(c) & 0xC0) != 0x80)
			++n;
	return n;
}

namespace {

// Byte index just after the first @p chars code points of @p text starting at @p from.
std::size_t advance(const std::string &text, std::size_t from, std::size_t chars)
{
	std::size_t i = from;
	std::size_t seen = 0;
	while (i < text.size()) {
		if ((static_cast<unsigned char>(text[i]) & 0xC0) != 0x80) {
			if (seen == chars)
				break;
			++seen;
		}
		++i;
	}
	return i;
}

// Cuts @p text (from byte @p from) into a piece of at most @p chars code points, at a space when there is one
// in the second half of the piece. Returns the end of the piece (always > from when text remains).
std::size_t pieceEnd(const std::string &text, std::size_t from, std::size_t chars)
{
	const std::size_t hard = advance(text, from, chars);
	if (hard >= text.size())
		return text.size();
	// Look back for a space, but not so far that the piece becomes tiny.
	const std::size_t minEnd = advance(text, from, chars / 2);
	for (std::size_t i = hard; i > minEnd && i > from; --i) {
		if (text[i - 1] == ' ')
			return i;
	}
	return hard; // no space (very long word): cut at the limit, on a code point boundary
}

} // namespace

std::vector<Page> paginate(const Slide &slide, std::size_t maxChars)
{
	maxChars = std::max<std::size_t>(maxChars, 20);
	std::vector<Page> pages;
	Page current;
	std::size_t used = 0;

	const auto flush = [&] {
		if (!current.verses.empty()) {
			pages.push_back(std::move(current));
			current = Page{};
			used = 0;
		}
	};

	for (const SlideVerse &v : slide.verses) {
		const std::size_t length = charCount(v.text);
		// A verse that fits in the room left goes on this page; otherwise it starts the next one.
		if (used + length <= maxChars) {
			current.verses.push_back({v.chapter, v.verse, v.text, false});
			used += length + 3; // the verse number and the space
			continue;
		}
		if (length <= maxChars) {
			flush();
			current.verses.push_back({v.chapter, v.verse, v.text, false});
			used = length + 3;
			continue;
		}
		// Alone too long for a page: cut it in pieces, each on its own page.
		flush();
		std::size_t from = 0;
		bool first = true;
		while (from < v.text.size()) {
			const std::size_t end = pieceEnd(v.text, from, maxChars);
			current.verses.push_back({v.chapter, v.verse, v.text.substr(from, end - from), !first});
			first = false;
			from = end;
			flush();
		}
	}
	flush();
	return pages;
}

} // namespace vyra::stage
