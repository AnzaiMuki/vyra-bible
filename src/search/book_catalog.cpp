/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "src/search/book_catalog.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

namespace vyra::search {

namespace {

struct BookNames {
	const char *french;  // printed name in the Segond
	const char *english; // printed name in the KJV
	// Extra spellings, separated by '|': abbreviations and variants (French first, then English).
	// The two names above are always added. Spellings are free-form: they are normalized when loaded.
	const char *aliases;
};

// Index = book number - 1 (Genesis = 1 ... Revelation = 66).
// French abbreviations are the ones the Segond itself uses in its cross references
// (checked by the tests against the whole source: "No" = Nombres, "Ép" = Éphésiens, ...).
constexpr std::array<BookNames, 66> kBooks = {{
	{"Genèse", "Genesis", "Ge|Gn|Gen"},
	{"Exode", "Exodus", "Ex|Exo|Exod"},
	{"Lévitique", "Leviticus", "Lé|Lv|Lev"},
	{"Nombres", "Numbers", "No|Nb|Nom|Num"},
	{"Deutéronome", "Deuteronomy", "De|Dt|Deut"},
	{"Josué", "Joshua", "Jos|Josh"},
	{"Juges", "Judges", "Jg|Jug|Judg"},
	{"Ruth", "Ruth", "Ru|Rt"},
	{"1 Samuel", "1 Samuel", "1 S|1 Sa|1 Sam"},
	{"2 Samuel", "2 Samuel", "2 S|2 Sa|2 Sam"},
	{"1 Rois", "1 Kings", "1 R|1 Ro|1 Ki|1 Kgs"},
	{"2 Rois", "2 Kings", "2 R|2 Ro|2 Ki|2 Kgs"},
	{"1 Chroniques", "1 Chronicles", "1 Ch|1 Chr"},
	{"2 Chroniques", "2 Chronicles", "2 Ch|2 Chr"},
	{"Esdras", "Ezra", "Esd|Ezr"},
	{"Néhémie", "Nehemiah", "Né|Neh"},
	{"Esther", "Esther", "Est|Esth"},
	{"Job", "Job", "Jb"},
	{"Psaumes", "Psalms", "Ps|Psaume|Psa|Psalm|Pss"},
	{"Proverbes", "Proverbs", "Pr|Pro|Prov"},
	{"Ecclésiaste", "Ecclesiastes", "Ec|Ecc|Eccl|Qohélet"},
	{"Cantique des Cantiques", "Song of Solomon", "Ca|Cant|Cantique|Song of Songs|Song|SOS|Canticles"},
	{"Ésaïe", "Isaiah", "És|Es|Esaie|Isaïe|Is|Isa"},
	{"Jérémie", "Jeremiah", "Jé|Jr|Jer"},
	{"Lamentations", "Lamentations", "La|Lam"},
	{"Ézéchiel", "Ezekiel", "Éz|Ez|Ezé|Ezek|Eze"},
	{"Daniel", "Daniel", "Da|Dn|Dan"},
	{"Osée", "Hosea", "Os|Hos"},
	{"Joël", "Joel", "Joë|Jl"},
	{"Amos", "Amos", "Am"},
	{"Abdias", "Obadiah", "Ab|Abd|Obad|Oba"},
	{"Jonas", "Jonah", "Jon"},
	{"Michée", "Micah", "Mi|Mic"},
	{"Nahum", "Nahum", "Na|Nah"},
	{"Habacuc", "Habakkuk", "Ha|Hab|Habakuk"},
	{"Sophonie", "Zephaniah", "So|Sph|Zeph|Zep"},
	{"Aggée", "Haggai", "Ag|Agg|Hag"},
	{"Zacharie", "Zechariah", "Za|Zac|Zech|Zec"},
	{"Malachie", "Malachi", "Mal"},
	{"Matthieu", "Matthew", "Mt|Mat|Matt"},
	{"Marc", "Mark", "Mc|Mr|Mar|Mrk|Mk"},
	{"Luc", "Luke", "Lu|Lc|Luk|Lk"},
	{"Jean", "John", "Jn|Jhn"},
	{"Actes", "Acts", "Ac|Act|Actes des Apôtres"},
	{"Romains", "Romans", "Ro|Rm|Rom"},
	{"1 Corinthiens", "1 Corinthians", "1 Co|1 Cor"},
	{"2 Corinthiens", "2 Corinthians", "2 Co|2 Cor"},
	{"Galates", "Galatians", "Ga|Gal"},
	{"Éphésiens", "Ephesians", "Ép|Ep|Eph|Éph"},
	{"Philippiens", "Philippians", "Ph|Php|Phil|Phi"},
	{"Colossiens", "Colossians", "Col"},
	{"1 Thessaloniciens", "1 Thessalonians", "1 Th|1 Thes|1 Thess"},
	{"2 Thessaloniciens", "2 Thessalonians", "2 Th|2 Thes|2 Thess"},
	{"1 Timothée", "1 Timothy", "1 Ti|1 Tim"},
	{"2 Timothée", "2 Timothy", "2 Ti|2 Tim"},
	{"Tite", "Titus", "Tit"},
	{"Philémon", "Philemon", "Phm|Philem|Phlm"},
	{"Hébreux", "Hebrews", "Hé|He|Heb"},
	{"Jacques", "James", "Ja|Jc|Jas|Jam"},
	{"1 Pierre", "1 Peter", "1 Pi|1 Pe|1 Pet|1 Pt"},
	{"2 Pierre", "2 Peter", "2 Pi|2 Pe|2 Pet|2 Pt"},
	{"1 Jean", "1 John", "1 Jn"},
	{"2 Jean", "2 John", "2 Jn"},
	{"3 Jean", "3 John", "3 Jn"},
	{"Jude", "Jude", "Jud|Jd"},
	{"Apocalypse", "Revelation", "Ap|Apo|Apoc|Rev|Revelations|Révélation"},
}};

struct Alias {
	std::string key; // normalized, no spaces
	int book;
};

// Decodes one UTF-8 character at @p i. Returns the code point (or 0xFFFFFFFF if invalid) and advances @p i.
std::uint32_t nextCodePoint(std::string_view s, std::size_t &i)
{
	const auto c = static_cast<unsigned char>(s[i]);
	if (c < 0x80) {
		++i;
		return c;
	}
	std::size_t extra = 0;
	std::uint32_t cp = 0;
	if (c >= 0xC2 && c <= 0xDF) {
		extra = 1;
		cp = c & 0x1F;
	} else if (c >= 0xE0 && c <= 0xEF) {
		extra = 2;
		cp = c & 0x0F;
	} else if (c >= 0xF0 && c <= 0xF4) {
		extra = 3;
		cp = c & 0x07;
	} else {
		++i;
		return 0xFFFFFFFFu;
	}
	if (i + extra >= s.size()) { // truncated sequence
		++i;
		return 0xFFFFFFFFu;
	}
	for (std::size_t k = 1; k <= extra; ++k) {
		const auto cc = static_cast<unsigned char>(s[i + k]);
		if ((cc & 0xC0) != 0x80) {
			++i;
			return 0xFFFFFFFFu;
		}
		cp = (cp << 6) | (cc & 0x3F);
	}
	i += extra + 1;
	return cp;
}

// Accent folding for the Latin-1 supplement (U+00C0..U+00FF) and the ligatures we may meet.
// Returns the replacement ASCII text, or nullptr if the code point is not a letter we know.
const char *foldLetter(std::uint32_t cp)
{
	if (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7)
		cp += 0x20; // upper case -> lower case in the Latin-1 block
	switch (cp) {
	case 0xE0:
	case 0xE1:
	case 0xE2:
	case 0xE3:
	case 0xE4:
	case 0xE5:
		return "a";
	case 0xE6:
		return "ae";
	case 0xE7:
		return "c";
	case 0xE8:
	case 0xE9:
	case 0xEA:
	case 0xEB:
		return "e";
	case 0xEC:
	case 0xED:
	case 0xEE:
	case 0xEF:
		return "i";
	case 0xF1:
		return "n";
	case 0xF2:
	case 0xF3:
	case 0xF4:
	case 0xF5:
	case 0xF6:
		return "o";
	case 0xF9:
	case 0xFA:
	case 0xFB:
	case 0xFC:
		return "u";
	case 0xFD:
	case 0xFF:
		return "y";
	case 0x152:
	case 0x153:
		return "oe"; // Œ œ
	default:
		return nullptr;
	}
}

std::vector<Alias> buildTable(std::vector<std::string> &conflicts)
{
	std::vector<Alias> table;
	auto add = [&](std::string_view spelling, int book) {
		std::string key = normalizeText(spelling);
		key.erase(std::remove_if(key.begin(), key.end(), [](char c) { return c == ' ' || c == '.'; }),
			  key.end());
		if (key.empty())
			return;
		table.push_back({std::move(key), book});
	};

	for (std::size_t i = 0; i < kBooks.size(); ++i) {
		const int book = static_cast<int>(i) + 1;
		add(kBooks[i].french, book);
		add(kBooks[i].english, book);
		std::string_view rest = kBooks[i].aliases;
		while (!rest.empty()) {
			const auto bar = rest.find('|');
			add(rest.substr(0, bar), book);
			if (bar == std::string_view::npos)
				break;
			rest.remove_prefix(bar + 1);
		}
	}

	std::sort(table.begin(), table.end(),
		  [](const Alias &a, const Alias &b) { return a.key != b.key ? a.key < b.key : a.book < b.book; });
	// Same spelling for the same book is harmless (e.g. "Job"); for two books it is a conflict.
	std::vector<Alias> unique;
	for (const auto &a : table) {
		if (!unique.empty() && unique.back().key == a.key) {
			if (unique.back().book != a.book)
				conflicts.push_back(a.key + ": book " + std::to_string(unique.back().book) +
						    " / book " + std::to_string(a.book));
			continue; // the lowest book number wins, and the conflict is reported
		}
		unique.push_back(a);
	}
	return unique;
}

struct Catalog {
	std::vector<std::string> conflicts;
	std::vector<Alias> table; // sorted by key, one entry per key
	Catalog() : table(buildTable(conflicts)) {}
};

const Catalog &catalog()
{
	static const Catalog instance; // built once, on first use (thread-safe since C++11)
	return instance;
}

} // namespace

std::string normalizeText(std::string_view utf8)
{
	std::string out;
	out.reserve(utf8.size());
	std::size_t i = 0;
	while (i < utf8.size()) {
		const std::uint32_t cp = nextCodePoint(utf8, i);
		if (cp < 0x80) {
			const char c = static_cast<char>(cp);
			if (c >= 'A' && c <= 'Z')
				out.push_back(static_cast<char>(c - 'A' + 'a'));
			else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
				out.push_back(c);
			else if (c == ':' || c == '.' || c == ',' || c == ';' || c == '-')
				out.push_back(c);
			else if (c == '\'')
				continue; // apostrophe inside a word ("l'Éternel"): dropped, not a separator
			else
				out.push_back(' ');
			continue;
		}
		// Unicode dashes and minus: U+2010..U+2015, U+2212
		if ((cp >= 0x2010 && cp <= 0x2015) || cp == 0x2212) {
			out.push_back('-');
			continue;
		}
		if (cp == 0x2019 || cp == 0x2018) // typographic apostrophes
			continue;
		if (const char *folded = foldLetter(cp)) {
			out.append(folded);
			continue;
		}
		out.push_back(' '); // anything else (no-break space, symbols, invalid bytes)
	}
	return out;
}

std::string_view frenchBookName(int book)
{
	if (book < 1 || book > static_cast<int>(kBooks.size()))
		return {};
	return kBooks[static_cast<std::size_t>(book) - 1].french;
}

std::string_view englishBookName(int book)
{
	if (book < 1 || book > static_cast<int>(kBooks.size()))
		return {};
	return kBooks[static_cast<std::size_t>(book) - 1].english;
}

int findBookExact(std::string_view key)
{
	const auto &table = catalog().table;
	const auto it = std::lower_bound(table.begin(), table.end(), key, [](const Alias &a, std::string_view k) {
		return std::string_view(a.key) < k;
	});
	if (it == table.end() || it->key != key)
		return 0;
	return it->book;
}

std::vector<int> findBooksByPrefix(std::string_view key)
{
	std::vector<int> books;
	if (key.empty())
		return books;
	const auto &table = catalog().table;
	auto it = std::lower_bound(table.begin(), table.end(), key,
				   [](const Alias &a, std::string_view k) { return std::string_view(a.key) < k; });
	for (; it != table.end() && std::string_view(it->key).substr(0, key.size()) == key; ++it)
		books.push_back(it->book);
	std::sort(books.begin(), books.end());
	books.erase(std::unique(books.begin(), books.end()), books.end());
	return books;
}

const std::vector<std::string> &catalogConflicts()
{
	return catalog().conflicts;
}

std::size_t catalogAliasCount()
{
	return catalog().table.size();
}

} // namespace vyra::search
