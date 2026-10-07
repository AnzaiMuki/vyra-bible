/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "src/search/passage_query.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>

#include "src/search/book_catalog.hpp"

namespace vyra::search {

namespace {

using bible::BibleModule;
using bible::Reference;

constexpr std::size_t kMaxQueryBytes = 256; // anything longer is not a reference
constexpr int kHuge = 999999;               // stands for "a number too large to be valid"

enum class TokenKind { Word, Number, Punct };

struct Token {
	TokenKind kind;
	std::string text; // Word: letters; Number: digits; Punct: one of ": . , ; -"
};

// Splits normalized text into words, numbers and punctuation. Spaces only separate tokens.
std::vector<Token> tokenize(const std::string &normalized)
{
	std::vector<Token> tokens;
	std::size_t i = 0;
	while (i < normalized.size()) {
		const char c = normalized[i];
		if (c == ' ') {
			++i;
		} else if (c >= 'a' && c <= 'z') {
			std::size_t j = i;
			while (j < normalized.size() && normalized[j] >= 'a' && normalized[j] <= 'z')
				++j;
			tokens.push_back({TokenKind::Word, normalized.substr(i, j - i)});
			i = j;
		} else if (c >= '0' && c <= '9') {
			std::size_t j = i;
			while (j < normalized.size() && normalized[j] >= '0' && normalized[j] <= '9')
				++j;
			tokens.push_back({TokenKind::Number, normalized.substr(i, j - i)});
			i = j;
		} else {
			tokens.push_back({TokenKind::Punct, std::string(1, c)});
			++i;
		}
	}
	return tokens;
}

int numberValue(const std::string &digits)
{
	if (digits.size() > 6)
		return kHuge;
	int v = 0;
	for (const char c : digits)
		v = v * 10 + (c - '0');
	return v;
}

bool isOneOf(const std::string &word, std::initializer_list<const char *> set)
{
	return std::any_of(set.begin(), set.end(), [&](const char *s) { return word == s; });
}

bool isChapterFiller(const std::string &w)
{
	return isOneOf(w, {"chapitre", "chapitres", "chap", "chapter", "chapters", "chp", "ch"});
}

bool isVerseWord(const std::string &w)
{
	return isOneOf(w, {"v", "vv", "vs", "ver", "vers", "verset", "versets", "verse", "verses"});
}

bool isRangeWord(const std::string &w)
{
	return isOneOf(w, {"a", "au", "to"}); // "à" and "au" in French, "to" in English
}

enum class Sep { None, Verse, Range, Adjacent };

struct NumberItem {
	int value;
	Sep before; // what separated it from the previous number
};

QueryResult make(QueryStatus status, int book = 0)
{
	QueryResult r;
	r.status = status;
	r.book = book;
	return r;
}

// Reads the book name at the start of the tokens and returns its key (normalized, no spaces,
// with the book number first: "1co", "jean"). @p next receives the index of the first token after
// the name. Returns false when there is no name.
bool extractBookKey(const std::vector<Token> &tokens, std::string &key, std::size_t &next)
{
	std::size_t i = 0;
	std::string numberPrefix;
	next = 0;
	if (tokens.empty())
		return false;

	// "1 Co", "2 Jean": a digit 1..3 followed by a word.
	if (tokens[0].kind == TokenKind::Number && tokens[0].text.size() == 1 && tokens[0].text[0] >= '1' &&
	    tokens[0].text[0] <= '3' && tokens.size() > 1 && tokens[1].kind == TokenKind::Word) {
		numberPrefix = tokens[0].text;
		i = 1;
		// "1er Jean", "1re Pierre": the ordinal ending is ignored when a name follows.
		if (tokens.size() > i + 1 && tokens[i].kind == TokenKind::Word &&
		    tokens[i + 1].kind == TokenKind::Word &&
		    isOneOf(tokens[i].text, {"er", "re", "ere", "e", "eme", "ieme", "st", "nd", "rd"}))
			++i;
		// "I Co", "II Jean", "III Jn": Roman numerals.
	} else if (tokens[0].kind == TokenKind::Word && tokens.size() > 1 && tokens[1].kind == TokenKind::Word &&
		   isOneOf(tokens[0].text, {"i", "ii", "iii"})) {
		numberPrefix = tokens[0].text == "i" ? "1" : tokens[0].text == "ii" ? "2" : "3";
		i = 1;
	}

	std::vector<std::string> words;
	while (i < tokens.size() && (tokens[i].kind == TokenKind::Word || tokens[i].text == ".")) {
		if (tokens[i].kind == TokenKind::Word)
			words.push_back(tokens[i].text);
		++i; // a '.' after an abbreviation ("Jn. 3") is skipped
	}
	next = i;

	// "Jean chapitre 3": a trailing filler word is not part of the name, unless it is the only word ("1 Ch").
	while (words.size() > 1 && isChapterFiller(words.back()))
		words.pop_back();

	if (words.empty())
		return false;

	key = numberPrefix;
	for (const auto &w : words)
		key += w;
	return true;
}

// Finds the book named at the start of the tokens: exact name or abbreviation first, then a unique prefix.
QueryResult parseBook(const std::vector<Token> &tokens, std::size_t &next)
{
	std::string key;
	if (!extractBookKey(tokens, key, next))
		return make(QueryStatus::Syntax);

	if (const int exact = findBookExact(key))
		return make(QueryStatus::Ok, exact);

	QueryResult r;
	r.candidates = findBooksByPrefix(key);
	if (r.candidates.empty()) {
		r.status = QueryStatus::UnknownBook;
	} else if (r.candidates.size() == 1) {
		r.status = QueryStatus::Ok;
		r.book = r.candidates.front();
		r.candidates.clear();
	} else {
		r.status = QueryStatus::AmbiguousBook;
	}
	return r;
}

QueryResult fail(QueryStatus status, int book, int limit = 0)
{
	QueryResult r = make(status, book);
	r.limit = limit;
	return r;
}

// Checks one (chapter, verse) of a book. Returns Ok or the error result.
QueryResult checkVerse(const BibleModule &module, int book, int chapter, int verse)
{
	const int chapters = module.chapterCount(book);
	if (chapter < 1 || chapter > chapters)
		return fail(QueryStatus::ChapterOutOfRange, book, chapters);
	const int verses = module.verseCount(book, chapter);
	if (verse < 1 || verse > verses)
		return fail(QueryStatus::VerseOutOfRange, book, verses);
	return make(QueryStatus::Ok, book);
}

// From the first token after the book name to the end: chapter, verse and ranges, checked against the module.
QueryResult resolveNumbers(const std::vector<Token> &tokens, std::size_t next, int book, const BibleModule &module)
{
	const int chapters = module.chapterCount(book);
	if (chapters == 0)
		return make(QueryStatus::BookNotInModule, book);

	// ---- numbers: chapter, verse, ranges ----
	std::vector<NumberItem> items;
	Sep pending = Sep::None;
	bool havePending = false;

	auto addSeparator = [&](Sep sep) -> bool {
		if (items.empty() || havePending)
			return false; // a separator with no number before it, or two separators in a row
		pending = sep;
		havePending = true;
		return true;
	};

	for (std::size_t i = next; i < tokens.size(); ++i) {
		const Token &t = tokens[i];
		if (t.kind == TokenKind::Number) {
			items.push_back({numberValue(t.text),
					 items.empty() ? Sep::None : (havePending ? pending : Sep::Adjacent)});
			havePending = false;
		} else if (t.kind == TokenKind::Punct) {
			if (t.text == ";")
				return fail(QueryStatus::Unsupported, book);
			if (!addSeparator(t.text == "-" ? Sep::Range : Sep::Verse))
				return fail(QueryStatus::Syntax, book);
		} else if (isChapterFiller(t.text)) {
			continue; // "Jean chapitre 3"
		} else if (isVerseWord(t.text)) {
			if (!addSeparator(Sep::Verse))
				return fail(QueryStatus::Syntax, book);
		} else if (isRangeWord(t.text)) {
			if (!addSeparator(Sep::Range))
				return fail(QueryStatus::Syntax, book);
		} else {
			return fail(QueryStatus::Syntax, book);
		}
	}

	if (items.empty())
		return make(havePending ? QueryStatus::Syntax : QueryStatus::NeedChapter, book);
	if (havePending)
		return make(QueryStatus::Incomplete, book);

	auto isVerseSep = [](Sep s) {
		return s == Sep::Verse || s == Sep::Adjacent;
	};

	Passage p;
	p.book = book;
	const bool single = chapters == 1; // Jude, Philémon, 2 Jean, 3 Jean, Abdias: "Jude 5" is verse 5

	// Shape of the query -> (start chapter, start verse, end chapter, end verse). 0 as a verse means "end of chapter".
	int sc = 0, sv = 0, ec = 0, ev = 0;
	switch (items.size()) {
	case 1:
		if (single) {
			sc = ec = 1;
			sv = ev = items[0].value;
		} else {
			sc = ec = items[0].value;
			sv = 1;
			ev = 0;
		}
		break;
	case 2:
		if (isVerseSep(items[1].before)) { // N1:N2
			sc = ec = items[0].value;
			sv = ev = items[1].value;
		} else if (single) { // Jude 5-7
			sc = ec = 1;
			sv = items[0].value;
			ev = items[1].value;
		} else { // Jean 3-4
			sc = items[0].value;
			ec = items[1].value;
			sv = 1;
			ev = 0;
		}
		break;
	case 3: // N1:N2-N3
		if (!isVerseSep(items[1].before) || items[2].before != Sep::Range)
			return fail(QueryStatus::Syntax, book);
		sc = ec = items[0].value;
		sv = items[1].value;
		ev = items[2].value;
		break;
	case 4: // N1:N2-N3:N4
		if (!isVerseSep(items[1].before) || items[2].before != Sep::Range || !isVerseSep(items[3].before))
			return fail(QueryStatus::Syntax, book);
		sc = items[0].value;
		sv = items[1].value;
		ec = items[2].value;
		ev = items[3].value;
		break;
	default:
		return fail(QueryStatus::Syntax, book);
	}

	// Validate the start, then the end ("end of chapter" is always valid once the chapter is).
	if (QueryResult r = checkVerse(module, book, sc, sv); !r.ok())
		return r;
	if (ev == 0) {
		if (ec < 1 || ec > chapters)
			return fail(QueryStatus::ChapterOutOfRange, book, chapters);
		ev = module.verseCount(book, ec);
	} else if (QueryResult r = checkVerse(module, book, ec, ev); !r.ok()) {
		return r;
	}
	if (ec < sc || (ec == sc && ev < sv))
		return fail(QueryStatus::ReversedRange, book);

	p.startChapter = sc;
	p.startVerse = sv;
	p.endChapter = ec;
	p.endVerse = ev;

	QueryResult ok = make(QueryStatus::Ok, book);
	ok.passage = p;
	return ok;
}


} // namespace

namespace {

// "17", "17-19", ":17", "v17" and "3:16", "3 16", "3:16-18" with nothing but numbers and separators:
// the book is the one of the context.
bool isRelative(const std::vector<Token> &tokens)
{
	bool number = false;
	for (const Token &t : tokens) {
		if (t.kind == TokenKind::Number)
			number = true;
		else if (t.kind == TokenKind::Punct)
			continue;
		else if (!(isChapterFiller(t.text) || isVerseWord(t.text) || isRangeWord(t.text)))
			return false;
	}
	return number;
}

} // namespace

QueryResult resolveQuery(std::string_view query, const BibleModule &module, const Passage *context)
{
	if (query.size() > kMaxQueryBytes)
		return make(QueryStatus::Syntax);

	std::vector<Token> tokens = tokenize(normalizeText(query));
	if (tokens.empty())
		return make(QueryStatus::Empty);

	// Shorthand relative to the passage being read: "17" is verse 17 of the same chapter.
	if (context && context->book >= 1 && isRelative(tokens)) {
		// A leading verse separator (":17", "v17") only says "a verse".
		while (!tokens.empty() && ((tokens.front().kind == TokenKind::Punct && tokens.front().text != "-" &&
					    tokens.front().text != ";") ||
					   (tokens.front().kind == TokenKind::Word && isVerseWord(tokens.front().text))))
			tokens.erase(tokens.begin());
		if (tokens.empty())
			return make(QueryStatus::Incomplete, context->book);

		int numbers = 0;
		bool verseSeparator = false;
		bool range = false;
		for (const Token &t : tokens) {
			if (t.kind == TokenKind::Number)
				++numbers;
			else if ((t.kind == TokenKind::Punct && t.text != "-" && t.text != ";") ||
				 (t.kind == TokenKind::Word && isVerseWord(t.text)))
				verseSeparator = true;
			else if (t.text == "-" || (t.kind == TokenKind::Word && isRangeWord(t.text)))
				range = true;
		}
		// One number, or two joined by a range ("17-19"): verses of the chapter being read.
		if (numbers == 1 || (numbers == 2 && range && !verseSeparator)) {
			Token chapter;
			chapter.kind = TokenKind::Number;
			chapter.text = std::to_string(context->endChapter);
			Token colon;
			colon.kind = TokenKind::Punct;
			colon.text = ":";
			tokens.insert(tokens.begin(), {chapter, colon});
		}
		return resolveNumbers(tokens, 0, context->book, module);
	}

	std::size_t next = 0;
	QueryResult bookResult = parseBook(tokens, next);
	if (bookResult.status != QueryStatus::Ok)
		return bookResult;
	return resolveNumbers(tokens, next, bookResult.book, module);
}

std::vector<Reference> passageVerses(const Passage &p, const BibleModule &module)
{
	std::vector<Reference> verses;
	if (p.book < 1)
		return verses;
	for (int c = p.startChapter; c <= p.endChapter; ++c) {
		const int first = c == p.startChapter ? p.startVerse : 1;
		const int last = c == p.endChapter ? p.endVerse : module.verseCount(p.book, c);
		for (int v = first; v <= last; ++v) {
			if (module.verse({p.book, c, v}))
				verses.push_back({p.book, c, v});
		}
	}
	return verses;
}

std::string formatPassage(const Passage &p, const BibleModule &module)
{
	static const char *const kDash = "\xE2\x80\x93"; // en dash
	std::string out(frenchBookName(p.book));
	if (out.empty())
		return out;
	out += ' ';

	const bool single = module.chapterCount(p.book) == 1;
	const auto num = [](int n) {
		return std::to_string(n);
	};

	if (single) { // "Jude 5", "Jude 5–7"
		out += num(p.startVerse);
		if (p.endVerse != p.startVerse)
			out += kDash + num(p.endVerse);
		return out;
	}
	if (p.startChapter == p.endChapter && p.startVerse == p.endVerse)
		return out + num(p.startChapter) + ':' + num(p.startVerse);

	const bool wholeStart = p.startVerse == 1;
	const bool wholeEnd = p.endVerse == module.verseCount(p.book, p.endChapter);
	if (wholeStart && wholeEnd) { // "Jean 3", "Jean 3–4"
		out += num(p.startChapter);
		if (p.endChapter != p.startChapter)
			out += kDash + num(p.endChapter);
		return out;
	}
	if (p.startChapter == p.endChapter)
		return out + num(p.startChapter) + ':' + num(p.startVerse) + kDash + num(p.endVerse);
	return out + num(p.startChapter) + ':' + num(p.startVerse) + kDash + num(p.endChapter) + ':' + num(p.endVerse);
}

std::vector<int> suggestBooks(std::string_view typed, std::size_t maxCount)
{
	if (typed.size() > kMaxQueryBytes)
		return {};
	const std::vector<Token> tokens = tokenize(normalizeText(typed));
	std::string key;
	std::size_t next = 0;
	if (!extractBookKey(tokens, key, next)) {
		// Only a book number so far ("1", "2", "3"): all the books of that series.
		if (tokens.size() == 1 && tokens[0].kind == TokenKind::Number && tokens[0].text.size() == 1 &&
		    tokens[0].text[0] >= '1' && tokens[0].text[0] <= '3')
			key = tokens[0].text;
		else
			return {};
	}
	std::vector<int> books = findBooksByPrefix(key);
	if (books.size() > maxCount)
		books.resize(maxCount);
	return books;
}

} // namespace vyra::search
