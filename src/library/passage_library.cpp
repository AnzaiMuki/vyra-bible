/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "src/library/passage_library.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace vyra::library {

namespace {

constexpr const char *kHeader = "VYRA-LIBRARY 1";

bool contains(const std::vector<search::Passage> &list, const search::Passage &p)
{
	return std::find(list.begin(), list.end(), p) != list.end();
}

void appendLines(std::ostringstream &out, char tag, const std::vector<search::Passage> &list)
{
	for (const search::Passage &p : list)
		out << tag << ' ' << p.book << ' ' << p.startChapter << ' ' << p.startVerse << ' ' << p.endChapter << ' '
		    << p.endVerse << '\n';
}

} // namespace

bool isValidPassage(const search::Passage &p, const bible::BibleModule &module)
{
	if (!module.verse({p.book, p.startChapter, p.startVerse}) || !module.verse({p.book, p.endChapter, p.endVerse}))
		return false;
	return p.startChapter < p.endChapter || (p.startChapter == p.endChapter && p.startVerse <= p.endVerse);
}

void PassageLibrary::addToHistory(const search::Passage &passage)
{
	history_.erase(std::remove(history_.begin(), history_.end(), passage), history_.end());
	history_.insert(history_.begin(), passage);
	if (history_.size() > kMaxHistory)
		history_.resize(kMaxHistory);
}

bool PassageLibrary::isFavorite(const search::Passage &passage) const
{
	return contains(favorites_, passage);
}

bool PassageLibrary::toggleFavorite(const search::Passage &passage)
{
	const auto it = std::find(favorites_.begin(), favorites_.end(), passage);
	if (it != favorites_.end()) {
		favorites_.erase(it);
		return false;
	}
	if (atFavoritesCap())
		return false;
	favorites_.push_back(passage);
	return true;
}

std::string PassageLibrary::serialize() const
{
	std::ostringstream out;
	out << kHeader << '\n';
	// History is written oldest first so that parse() (which adds each line to the top) restores the order.
	std::vector<search::Passage> oldestFirst(history_.rbegin(), history_.rend());
	appendLines(out, 'H', oldestFirst);
	appendLines(out, 'F', favorites_);
	return out.str();
}

LoadReport PassageLibrary::parse(const std::string &text, const bible::BibleModule &module)
{
	LoadReport report;
	report.fileFound = true;
	history_.clear();
	favorites_.clear();

	std::istringstream in(text);
	std::string line;
	bool first = true;
	while (std::getline(in, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (first) {
			first = false;
			if (line == kHeader)
				continue;
			// Unknown header (newer version or not our file): keep nothing rather than guess.
			++report.skippedLines;
			return report;
		}
		if (line.empty())
			continue;
		std::istringstream fields(line);
		char tag = 0;
		search::Passage p;
		std::string extra;
		if (!(fields >> tag >> p.book >> p.startChapter >> p.startVerse >> p.endChapter >> p.endVerse) ||
		    (fields >> extra) || (tag != 'H' && tag != 'F') || !isValidPassage(p, module)) {
			++report.skippedLines;
			continue;
		}
		if (tag == 'H')
			addToHistory(p);
		else if (!contains(favorites_, p) && favorites_.size() < kMaxFavorites)
			favorites_.push_back(p);
	}
	return report;
}

bool PassageLibrary::saveToFile(const std::filesystem::path &path) const
{
	std::error_code ec;
	if (path.has_parent_path())
		std::filesystem::create_directories(path.parent_path(), ec);
	std::filesystem::path tmp = path;
	tmp += ".tmp";
	{
		std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
		if (!out)
			return false;
		out << serialize();
		out.flush();
		if (!out)
			return false;
	}
	std::filesystem::rename(tmp, path, ec); // replaces an existing file, also on Windows
	if (ec) {
		std::filesystem::remove(tmp, ec);
		return false;
	}
	return true;
}

LoadReport PassageLibrary::loadFromFile(const std::filesystem::path &path, const bible::BibleModule &module)
{
	std::ifstream in(path, std::ios::binary);
	if (!in)
		return LoadReport{};
	std::ostringstream buffer;
	buffer << in.rdbuf();
	return parse(buffer.str(), module);
}

} // namespace vyra::library
