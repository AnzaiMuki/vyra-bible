/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

/*
 * Bible Engine, part 1: one Bible translation loaded in memory.
 *
 * This file has no dependency on OBS or Qt, so it is unit-tested on its own (see tests/).
 *
 * File format ("VYRA module", version 1): UTF-8 text, one record per line.
 *   #vyra-module<TAB>1          first line, mandatory
 *   #<key><TAB><value>          header: code, name, language, license, source (all mandatory)
 *   <book><TAB><chapter><TAB><verse><TAB><text>
 * Books are numbered 1..66 (Genesis = 1, Revelation = 66). Records must be in strictly
 * increasing (book, chapter, verse) order. Windows line endings are accepted.
 */

#include <array>
#include <cstdint>
#include <filesystem>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vyra::bible {

constexpr int kBookCount = 66;

struct Reference {
	int book = 0;    // 1..66
	int chapter = 0; // 1..
	int verse = 0;   // 1..
};

enum class LoadError {
	None,
	FileNotFound,  // the file cannot be opened
	NotAModule,    // missing or unknown "#vyra-module" first line
	MissingHeader, // a mandatory header key is absent
	CorruptRecord, // malformed line, number out of range, empty text, wrong order
	BadEncoding,   // the text is not valid UTF-8
	Empty,         // the module contains no verse
};

struct LoadResult {
	LoadError error = LoadError::None;
	int line = 0;       // 1-based line of the problem, 0 when not applicable
	std::string detail; // technical detail, for the log (not meant for the operator)

	bool ok() const { return error == LoadError::None; }
};

struct ModuleInfo {
	std::string code;     // e.g. "LSG1910"
	std::string name;     // e.g. "Louis Segond 1910"
	std::string language; // e.g. "fr"
	std::string license;
	std::string source;
};

class BibleModule {
public:
	/** Loads a module file. On failure the module is left empty. */
	LoadResult loadFromFile(const std::filesystem::path &path);

	/** Loads a module from a stream (used by the tests). On failure the module is left empty. */
	LoadResult loadFromStream(std::istream &in);

	bool empty() const { return entries_.empty(); }
	const ModuleInfo &info() const { return info_; }
	std::size_t verseCount() const { return entries_.size(); }

	/** Text of a verse, or nullopt if the module has no such verse. The view stays valid until the module is reloaded. */
	std::optional<std::string_view> verse(const Reference &ref) const;

	/** Number of the last chapter of a book (0 if the book is absent). */
	int chapterCount(int book) const;

	/** Number of the last verse of a chapter (0 if the chapter is absent). */
	int verseCount(int book, int chapter) const;

private:
	struct Entry {
		std::uint32_t key;    // book * 1'000'000 + chapter * 1'000 + verse
		std::uint32_t offset; // position of the text in text_
		std::uint32_t length;
	};

	static std::uint32_t makeKey(int book, int chapter, int verse);

	void clear();
	LoadResult parse(std::istream &in);

	ModuleInfo info_;
	std::vector<Entry> entries_;                    // sorted by key (guaranteed by the loader)
	std::string text_;                              // all verse texts, back to back
	std::array<int, kBookCount + 1> lastChapter_{}; // index = book number
};

} // namespace vyra::bible
