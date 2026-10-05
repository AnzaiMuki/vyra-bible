/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

/*
 * Search Engine, part 1: the 66 books, their names and abbreviations.
 *
 * Independent of OBS and Qt. Names are matched after normalization (lower case, no accents,
 * no spaces or dots), so "Genèse", "genese" and "GENÈSE" are the same key.
 */

#include <string>
#include <string_view>
#include <vector>

namespace vyra::search {

/**
 * Normalizes a UTF-8 text for matching: lower case, accents removed (é -> e, œ -> oe),
 * Unicode dashes -> '-', every other character that is not a letter, a digit or one of
 * ": . , ; -" becomes a single space. Invalid UTF-8 bytes become spaces.
 */
std::string normalizeText(std::string_view utf8);

/** Name of a book (1..66) as printed in the Louis Segond Bible, e.g. "1 Corinthiens". Empty if out of range. */
std::string_view frenchBookName(int book);

/** English name of a book (1..66), e.g. "1 Corinthians". Empty if out of range. */
std::string_view englishBookName(int book);

/**
 * Book whose name or abbreviation is exactly @p key (already normalized, without spaces:
 * "1co", "genese", "jn"). Returns 0 if there is none.
 */
int findBookExact(std::string_view key);

/** Distinct books (ascending) having a name or abbreviation that starts with @p key (normalized, no spaces). */
std::vector<int> findBooksByPrefix(std::string_view key);

/**
 * Names that two different books claim at the same time (empty when the table is consistent).
 * Each item reads "alias: book A / book B". Checked by the tests.
 */
const std::vector<std::string> &catalogConflicts();

/** Number of aliases in the table (for the tests). */
std::size_t catalogAliasCount();

} // namespace vyra::search
