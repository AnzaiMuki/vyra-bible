/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

/*
 * History (what went on the air) and favorites (what the operator wants to find fast).
 * Independent of OBS and Qt. The file format is plain text, one passage per line:
 *
 *   VYRA-LIBRARY 1
 *   H 43 3 16 3 16        (H = history, F = favorite; book, start chapter, start verse, end chapter, end verse)
 *
 * Loading never fails the plugin: a missing file is an empty library, a damaged line is skipped and counted.
 * Passages are checked against the Bible, so a file written for another translation cannot put a verse
 * that does not exist on the air.
 */

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "src/bible/bible_module.hpp"
#include "src/search/passage_query.hpp"

namespace vyra::library {

/** True if both ends exist in @p module and the start is not after the end. */
bool isValidPassage(const search::Passage &passage, const bible::BibleModule &module);

struct LoadReport {
	bool fileFound = false;
	std::size_t skippedLines = 0; // damaged, unknown or no longer valid
};

class PassageLibrary {
public:
	static constexpr std::size_t kMaxHistory = 50;
	static constexpr std::size_t kMaxFavorites = 200;

	/** History, most recent first. */
	const std::vector<search::Passage> &history() const { return history_; }
	/** Favorites, in the order they were added. */
	const std::vector<search::Passage> &favorites() const { return favorites_; }

	/** Records a passage put on the air: goes to the top; an older identical entry is removed. Oldest dropped past the cap. */
	void addToHistory(const search::Passage &passage);
	void clearHistory() { history_.clear(); }

	bool isFavorite(const search::Passage &passage) const;
	/** Adds (true) or removes (false) the passage. Returns the new state. At the cap, adding is refused and false is returned. */
	bool toggleFavorite(const search::Passage &passage);
	bool atFavoritesCap() const { return favorites_.size() >= kMaxFavorites; }

	std::string serialize() const;
	/** Replaces the content with what @p text holds. Lines that are not valid for @p module are skipped. */
	LoadReport parse(const std::string &text, const bible::BibleModule &module);

	/** Writes the file atomically (temporary file then rename): a crash never leaves half a file. */
	bool saveToFile(const std::filesystem::path &path) const;
	LoadReport loadFromFile(const std::filesystem::path &path, const bible::BibleModule &module);

private:
	std::vector<search::Passage> history_;
	std::vector<search::Passage> favorites_;
};

} // namespace vyra::library
