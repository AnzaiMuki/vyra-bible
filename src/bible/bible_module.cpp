/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "src/bible/bible_module.hpp"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <limits>

namespace vyra::bible {

namespace {

constexpr int kMaxChapter = 999;
constexpr int kMaxVerse = 999;

bool isValidUtf8(std::string_view s)
{
	std::size_t i = 0;
	const std::size_t n = s.size();
	while (i < n) {
		const auto c = static_cast<unsigned char>(s[i]);
		std::size_t extra = 0;
		std::uint32_t cp = 0;
		if (c < 0x80) {
			++i;
			continue;
		} else if (c >= 0xC2 && c <= 0xDF) {
			extra = 1;
			cp = c & 0x1F;
		} else if (c >= 0xE0 && c <= 0xEF) {
			extra = 2;
			cp = c & 0x0F;
		} else if (c >= 0xF0 && c <= 0xF4) {
			extra = 3;
			cp = c & 0x07;
		} else {
			return false; // continuation byte alone, or 0xC0/0xC1/0xF5+ (never valid)
		}
		if (i + extra >= n)
			return false; // truncated sequence: the last continuation byte is missing
		for (std::size_t k = 1; k <= extra; ++k) {
			const auto cc = static_cast<unsigned char>(s[i + k]);
			if ((cc & 0xC0) != 0x80)
				return false;
			cp = (cp << 6) | (cc & 0x3F);
		}
		// Reject overlong forms, UTF-16 surrogates and values beyond U+10FFFF.
		if ((extra == 2 && cp < 0x800) || (extra == 3 && cp < 0x10000) || (cp >= 0xD800 && cp <= 0xDFFF) ||
		    cp > 0x10FFFF)
			return false;
		i += extra + 1;
	}
	return true;
}

// Strict decimal parser: digits only, no sign, no spaces, within [min, max].
bool parseNumber(std::string_view s, int min, int max, int &out)
{
	if (s.empty())
		return false;
	int value = 0;
	const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
	if (ec != std::errc() || ptr != s.data() + s.size())
		return false;
	if (value < min || value > max)
		return false;
	out = value;
	return true;
}

LoadResult fail(LoadError error, int line, std::string detail)
{
	LoadResult r;
	r.error = error;
	r.line = line;
	r.detail = std::move(detail);
	return r;
}

} // namespace

std::uint32_t BibleModule::makeKey(int book, int chapter, int verse)
{
	return static_cast<std::uint32_t>(book) * 1'000'000u + static_cast<std::uint32_t>(chapter) * 1'000u +
	       static_cast<std::uint32_t>(verse);
}

void BibleModule::clear()
{
	info_ = ModuleInfo{};
	entries_.clear();
	entries_.shrink_to_fit();
	text_.clear();
	text_.shrink_to_fit();
	lastChapter_.fill(0);
}

LoadResult BibleModule::loadFromFile(const std::filesystem::path &path)
{
	std::ifstream in(path, std::ios::binary);
	if (!in.is_open()) {
		clear();
		return fail(LoadError::FileNotFound, 0, "cannot open " + path.u8string());
	}
	return loadFromStream(in);
}

LoadResult BibleModule::loadFromStream(std::istream &in)
{
	clear();
	LoadResult result = parse(in);
	if (!result.ok())
		clear(); // never keep a half-loaded Bible
	return result;
}

LoadResult BibleModule::parse(std::istream &in)
{
	std::string line;
	int lineNo = 0;
	bool sawMagic = false;
	bool inData = false;
	std::uint32_t previousKey = 0;

	while (std::getline(in, line)) {
		++lineNo;
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (lineNo == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0)
			line.erase(0, 3); // UTF-8 byte order mark added by some Windows editors
		if (line.empty())
			continue;
		if (!isValidUtf8(line))
			return fail(LoadError::BadEncoding, lineNo, "invalid UTF-8");

		if (line.front() == '#') {
			if (inData)
				return fail(LoadError::CorruptRecord, lineNo, "header line after the first verse");
			const auto tab = line.find('\t');
			if (tab == std::string::npos)
				return fail(LoadError::CorruptRecord, lineNo, "header line without a tab");
			const std::string_view key(line.data() + 1, tab - 1);
			const std::string value = line.substr(tab + 1);
			if (!sawMagic) {
				if (key != "vyra-module" || value != "1")
					return fail(LoadError::NotAModule, lineNo,
						    "unsupported or missing #vyra-module version");
				sawMagic = true;
			} else if (key == "code") {
				info_.code = value;
			} else if (key == "name") {
				info_.name = value;
			} else if (key == "language") {
				info_.language = value;
			} else if (key == "license") {
				info_.license = value;
			} else if (key == "source") {
				info_.source = value;
			} // unknown header keys are ignored on purpose: they allow future additions
			continue;
		}

		if (!sawMagic)
			return fail(LoadError::NotAModule, lineNo, "the first line is not '#vyra-module'");

		if (!inData) {
			inData = true;
			for (const auto *field :
			     {&info_.code, &info_.name, &info_.language, &info_.license, &info_.source}) {
				if (field->empty())
					return fail(
						LoadError::MissingHeader, lineNo,
						"a mandatory header (code, name, language, license, source) is missing or empty");
			}
		}

		// <book> TAB <chapter> TAB <verse> TAB <text>
		const auto t1 = line.find('\t');
		const auto t2 = t1 == std::string::npos ? t1 : line.find('\t', t1 + 1);
		const auto t3 = t2 == std::string::npos ? t2 : line.find('\t', t2 + 1);
		if (t3 == std::string::npos)
			return fail(LoadError::CorruptRecord, lineNo, "expected 4 tab-separated fields");

		int book = 0, chapter = 0, verse = 0;
		if (!parseNumber(std::string_view(line).substr(0, t1), 1, kBookCount, book) ||
		    !parseNumber(std::string_view(line).substr(t1 + 1, t2 - t1 - 1), 1, kMaxChapter, chapter) ||
		    !parseNumber(std::string_view(line).substr(t2 + 1, t3 - t2 - 1), 1, kMaxVerse, verse))
			return fail(LoadError::CorruptRecord, lineNo, "book, chapter or verse number out of range");

		const std::string_view text = std::string_view(line).substr(t3 + 1);
		if (text.empty())
			return fail(LoadError::CorruptRecord, lineNo, "empty verse text");
		if (text.find('\t') != std::string_view::npos)
			return fail(LoadError::CorruptRecord, lineNo, "tab inside the verse text");

		const std::uint32_t key = makeKey(book, chapter, verse);
		if (key <= previousKey)
			return fail(LoadError::CorruptRecord, lineNo, "records are not in strictly increasing order");
		previousKey = key;

		if (text_.size() + text.size() > std::numeric_limits<std::uint32_t>::max())
			return fail(LoadError::CorruptRecord, lineNo, "module too large");

		entries_.push_back(
			{key, static_cast<std::uint32_t>(text_.size()), static_cast<std::uint32_t>(text.size())});
		text_.append(text);
		lastChapter_[static_cast<std::size_t>(book)] =
			std::max(lastChapter_[static_cast<std::size_t>(book)], chapter);
	}

	if (!sawMagic)
		return fail(LoadError::NotAModule, 0, "empty file");
	if (!inData)
		return fail(LoadError::Empty, 0, "the module contains no verse");
	return LoadResult{};
}

std::optional<std::string_view> BibleModule::verse(const Reference &ref) const
{
	if (ref.book < 1 || ref.book > kBookCount || ref.chapter < 1 || ref.chapter > kMaxChapter || ref.verse < 1 ||
	    ref.verse > kMaxVerse)
		return std::nullopt;

	const std::uint32_t key = makeKey(ref.book, ref.chapter, ref.verse);
	const auto it = std::lower_bound(entries_.begin(), entries_.end(), key,
					 [](const Entry &e, std::uint32_t k) { return e.key < k; });
	if (it == entries_.end() || it->key != key)
		return std::nullopt;
	return std::string_view(text_).substr(it->offset, it->length);
}

int BibleModule::chapterCount(int book) const
{
	if (book < 1 || book > kBookCount)
		return 0;
	return lastChapter_[static_cast<std::size_t>(book)];
}

int BibleModule::verseCount(int book, int chapter) const
{
	if (book < 1 || book > kBookCount || chapter < 1 || chapter > kMaxChapter)
		return 0;
	// First entry at or after chapter+1, then step back to the last verse of this chapter.
	const std::uint32_t next = makeKey(book, chapter + 1, 0);
	auto it = std::lower_bound(entries_.begin(), entries_.end(), next,
				   [](const Entry &e, std::uint32_t k) { return e.key < k; });
	if (it == entries_.begin())
		return 0;
	--it;
	if (it->key / 1'000u != static_cast<std::uint32_t>(book) * 1'000u + static_cast<std::uint32_t>(chapter))
		return 0;
	return static_cast<int>(it->key % 1'000u);
}

} // namespace vyra::bible
