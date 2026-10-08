/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

/*
 * Search Engine, part 2: from what the operator types ("Jn 3 16", "1 Co 13.4-7") to a passage.
 *
 * Independent of OBS and Qt. The query is always checked against the loaded Bible, so a
 * passage returned with status Ok exists and can be sent to the preview without surprise.
 * Search by words (keywords) is not part of this milestone.
 *
 * Accepted forms (accents, case, spaces and dots are free):
 *   Jean 3:16        Jn 3 16       jn3.16        Jean 3,16 (French notation)    Jean chapitre 3 verset 16
 *   1 Co 13:4-7      I Co 13 4-7   1Co 13.4–7    Jean 3:16 à 18
 *   Jean 3:16-4:2    (across chapters)
 *   Jean 3           (whole chapter)             Jean 3-4        (chapters 3 to 4)
 *   Jude 5           Jude 5-7      (books of one chapter: the number is a verse)
 * Not accepted: lists such as "Jean 3:16;18" (status Unsupported).
 */

#include <string>
#include <string_view>
#include <vector>

#include "src/bible/bible_module.hpp"

namespace vyra::search {

/** An inclusive range of verses inside one book, possibly across chapters. Always valid when returned with Ok. */
struct Passage {
	int book = 0;
	int startChapter = 0;
	int startVerse = 0;
	int endChapter = 0;
	int endVerse = 0;
};

inline bool operator==(const Passage &a, const Passage &b)
{
	return a.book == b.book && a.startChapter == b.startChapter && a.startVerse == b.startVerse &&
	       a.endChapter == b.endChapter && a.endVerse == b.endVerse;
}

enum class QueryStatus {
	Ok,
	Empty,             // nothing typed
	Syntax,            // cannot be understood
	Unsupported,       // understood but not supported yet (lists with ';')
	UnknownBook,       // no book has this name
	AmbiguousBook,     // several books match: see QueryResult::candidates
	BookNotInModule,   // the book exists but the loaded Bible does not contain it
	NeedChapter,       // typing in progress: the book is known ("Jn")
	Incomplete,        // typing in progress: ends with a separator ("Jn 3:", "Jn 3:16-")
	ChapterOutOfRange, // see QueryResult::limit = last chapter
	VerseOutOfRange,   // see QueryResult::limit = last verse of the chapter
	ReversedRange,     // the end comes before the start ("Jn 3:18-16")
};

struct QueryResult {
	QueryStatus status = QueryStatus::Empty;
	Passage passage;             // meaningful only when status == Ok
	int book = 0;                // the book, whenever it could be determined (even for errors)
	std::vector<int> candidates; // AmbiguousBook: the matching books, ascending
	int limit = 0;               // ChapterOutOfRange / VerseOutOfRange: the largest valid number

	bool ok() const { return status == QueryStatus::Ok; }
	/** True while the operator is probably still typing (not an error to show in red). */
	bool inProgress() const
	{
		return status == QueryStatus::Empty || status == QueryStatus::NeedChapter ||
		       status == QueryStatus::Incomplete;
	}
};

/**
 * Understands @p query and checks it against @p module. Never throws; any text is accepted as input.
 *
 * @p context is the passage being read (the Preview), for the shorthand used while following a reading:
 *   17        verse 17 of the chapter of the context (its end chapter)
 *   17-19     verses 17 to 19 of that chapter        :17  v17   the same as 17
 *   4:1       chapter 4 verse 1 of the book of the context       4 1    the same
 * A query that names a book ("Jn 17") is never relative. Without a context, a query that is only
 * numbers is not understood (Syntax).
 */
QueryResult resolveQuery(std::string_view query, const bible::BibleModule &module, const Passage *context = nullptr);

/** Every verse of a passage, in reading order. */
std::vector<bible::Reference> passageVerses(const Passage &passage, const bible::BibleModule &module);

/**
 * Printed form of a passage, in French, e.g. "Jean 3:16", "Jean 3:16–18", "Jean 3", "Jean 3–4",
 * "Jean 3:16–4:2", "Jude 5". Ranges use the en dash.
 */
std::string formatPassage(const Passage &passage, const bible::BibleModule &module);

/** Books whose name or abbreviation starts with what is typed (for completion), at most @p maxCount. */
std::vector<int> suggestBooks(std::string_view typed, std::size_t maxCount = 8);

} // namespace vyra::search
