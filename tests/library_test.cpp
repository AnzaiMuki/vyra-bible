/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

// Tests of history and favorites. VYRA_LSG_PATH = data/bibles/lsg1910.tsv

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "src/bible/bible_module.hpp"
#include "src/library/passage_library.hpp"
#include "src/search/passage_query.hpp"
#include "tests/check.hpp"

using namespace vyra;
using namespace vyra::library;

namespace {

bible::BibleModule g_bible;

search::Passage P(const char *query)
{
	const auto r = search::resolveQuery(query, g_bible);
	CHECK(r.ok());
	return r.passage;
}

std::string slurp(const std::filesystem::path &p)
{
	std::ifstream in(p, std::ios::binary);
	std::ostringstream s;
	s << in.rdbuf();
	return s.str();
}

} // namespace

int main()
{
	CHECK(g_bible.loadFromFile(VYRA_LSG_PATH).ok());

	{ // history: most recent first, no duplicates, capped
		PassageLibrary lib;
		lib.addToHistory(P("Jn 3:16"));
		lib.addToHistory(P("Ps 23"));
		lib.addToHistory(P("Rm 8:28"));
		CHECK(lib.history().size() == 3 && lib.history()[0] == P("Rm 8:28") && lib.history()[2] == P("Jn 3:16"));
		lib.addToHistory(P("Jn 3:16")); // again: moves to the top, no second copy
		CHECK(lib.history().size() == 3 && lib.history()[0] == P("Jn 3:16") && lib.history()[1] == P("Rm 8:28"));
		CHECK(!(P("Jn 3:16") == P("Jn 3:17")) && !(P("Jn 3:16") == P("Jn 3:16-17")));
		for (int v = 1; v <= 31; ++v)
			lib.addToHistory({1, 1, v, 1, v}); // 31 more: 34 total, under the cap
		CHECK(lib.history().size() == 34);
		for (int c = 1; c <= 30; ++c)
			lib.addToHistory({2, c, 1, c, 1});
		CHECK(lib.history().size() == PassageLibrary::kMaxHistory);
		CHECK(lib.history()[0] == search::Passage({2, 30, 1, 30, 1}));
		CHECK(!(lib.history().back() == P("Jn 3:16"))); // the oldest ones fell off
		lib.clearHistory();
		CHECK(lib.history().empty());
	}

	{ // favorites: toggle, order, cap
		PassageLibrary lib;
		CHECK(lib.toggleFavorite(P("Ps 23")) && lib.isFavorite(P("Ps 23")));
		CHECK(lib.toggleFavorite(P("Jn 3:16")));
		CHECK(lib.favorites().size() == 2 && lib.favorites()[0] == P("Ps 23")); // order of adding
		CHECK(!lib.toggleFavorite(P("Ps 23")) && !lib.isFavorite(P("Ps 23")) && lib.favorites().size() == 1);
		CHECK(!lib.isFavorite(P("Jn 3:17")));
		PassageLibrary full;
		for (int c = 1; c <= 150; ++c)
			for (int v = 1; v <= 2; ++v)
				if (!full.atFavoritesCap())
					CHECK(full.toggleFavorite({19, c, v, c, v}));
		CHECK(full.favorites().size() == PassageLibrary::kMaxFavorites && full.atFavoritesCap());
		CHECK(!full.toggleFavorite(P("Jn 3:16")) && full.favorites().size() == PassageLibrary::kMaxFavorites);
		CHECK(!full.isFavorite(P("Jn 3:16")));
	}

	{ // validity against the Bible
		CHECK(isValidPassage(P("Jn 3:16-18"), g_bible) && isValidPassage(P("Jn 3"), g_bible));
		CHECK(!isValidPassage({43, 3, 16, 3, 99}, g_bible));
		CHECK(!isValidPassage({43, 3, 18, 3, 16}, g_bible)); // reversed
		CHECK(!isValidPassage({43, 4, 1, 3, 16}, g_bible));
		CHECK(!isValidPassage({0, 1, 1, 1, 1}, g_bible) && !isValidPassage({67, 1, 1, 1, 1}, g_bible));
		CHECK(!isValidPassage({43, 0, 1, 0, 1}, g_bible) && !isValidPassage({43, 3, 0, 3, 0}, g_bible));
	}

	{ // text round trip keeps order and content
		PassageLibrary a;
		a.addToHistory(P("Jn 3:16"));
		a.addToHistory(P("Ps 23"));
		a.addToHistory(P("Jn 3:16-4:2"));
		a.toggleFavorite(P("Rm 8:28"));
		a.toggleFavorite(P("Ap 22:21"));
		PassageLibrary b;
		const LoadReport r = b.parse(a.serialize(), g_bible);
		CHECK(r.fileFound && r.skippedLines == 0);
		CHECK(b.history().size() == 3 && b.favorites().size() == 2);
		for (std::size_t i = 0; i < 3; ++i)
			CHECK(b.history()[i] == a.history()[i]);
		for (std::size_t i = 0; i < 2; ++i)
			CHECK(b.favorites()[i] == a.favorites()[i]);
		CHECK(b.serialize() == a.serialize());
	}

	{ // damaged and foreign content: skipped, counted, never fatal
		PassageLibrary lib;
		const std::string text = std::string("VYRA-LIBRARY 1\r\n") + "H 43 3 16 3 16\r\n" // CRLF is tolerated
					 "H 43 3 16 3 99\n"   // verse that does not exist
					 "H 43 3 18 3 16\n"   // reversed
					 "X 43 3 16 3 16\n"   // unknown tag
					 "F 43 3 16\n"        // too short
					 "F 43 3 16 3 16 7\n" // too long
					 "F abc\n"
					 "\n"
					 "F 43 3 16 3 16\n"
					 "F 43 3 16 3 16\n";  // duplicate: kept once
		const LoadReport r = lib.parse(text, g_bible);
		CHECK(r.skippedLines == 6);
		CHECK(lib.history().size() == 1 && lib.favorites().size() == 1);
		PassageLibrary other;
		other.addToHistory(P("Jn 3:16"));
		const LoadReport bad = other.parse("not a library file\nH 43 3 16 3 16\n", g_bible);
		CHECK(bad.skippedLines == 1 && other.history().empty() && other.favorites().empty());
		const LoadReport empty = other.parse("", g_bible);
		CHECK(empty.skippedLines == 0 && other.history().empty());  // an empty file is an empty library
		// a Bible with fewer verses (another translation) drops what it does not have
		bible::BibleModule tiny;
		std::istringstream tinyIn("#vyra-module\t1\n#code\tT\n#name\tTiny\n#language\tfr\n#license\tPD\n#source\tx\n43\t3\t16\tCar Dieu\n");
		CHECK(tiny.loadFromStream(tinyIn).ok());
		PassageLibrary t;
		const LoadReport tr = t.parse("VYRA-LIBRARY 1\nH 43 3 16 3 16\nH 43 3 17 3 17\n", tiny);
		CHECK(tr.skippedLines == 1 && t.history().size() == 1);
	}

	{ // files: absent file = empty library; save/load; atomic replace; unwritable folder reports failure
		namespace fs = std::filesystem;
		const fs::path dir = fs::temp_directory_path() / "vyra_library_test";
		fs::remove_all(dir);
		PassageLibrary lib;
		const LoadReport none = lib.loadFromFile(dir / "nope.txt", g_bible);
		CHECK(!none.fileFound && lib.history().empty());

		lib.addToHistory(P("Jn 3:16"));
		lib.toggleFavorite(P("Ps 23"));
		CHECK(lib.saveToFile(dir / "sub" / "library.txt")); // creates the folder
		CHECK(!fs::exists(dir / "sub" / "library.txt.tmp"));
		lib.addToHistory(P("Rm 8:28"));
		CHECK(lib.saveToFile(dir / "sub" / "library.txt")); // replaces
		PassageLibrary again;
		const LoadReport r = again.loadFromFile(dir / "sub" / "library.txt", g_bible);
		CHECK(r.fileFound && r.skippedLines == 0 && again.history().size() == 2 && again.favorites().size() == 1);
		CHECK(slurp(dir / "sub" / "library.txt") == lib.serialize());
		// a path that cannot be a file (its parent is a regular file)
		CHECK(!lib.saveToFile(dir / "sub" / "library.txt" / "x.txt"));
		fs::remove_all(dir);
	}

	return testing::finish();
}
