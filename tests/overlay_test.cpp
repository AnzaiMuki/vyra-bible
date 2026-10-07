/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

// Tests of what is sent to the overlay page: the frame, its JSON, and the HTTP vocabulary.
// VYRA_LSG_PATH = data/bibles/lsg1910.tsv

#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "src/bible/bible_module.hpp"
#include "src/overlay/http_routes.hpp"
#include "src/overlay/overlay_frame.hpp"
#include "src/search/passage_query.hpp"
#include "src/stage/stage_controller.hpp"
#include "tests/check.hpp"

using namespace vyra::overlay;
using namespace vyra::bible;
using namespace vyra::search;
using namespace vyra::stage;

namespace {

BibleModule g_bible;

// ---- A strict JSON reader, written independently of the writer, only to check the writer. ----
struct Json {
	enum Kind { Null, Bool, Num, Str, Arr, Obj } kind = Null;
	bool b = false;
	double n = 0;
	std::string s;
	std::vector<Json> a;
	std::map<std::string, Json> o;
};

struct Reader {
	const std::string &t;
	std::size_t i = 0;
	bool ok = true;
	explicit Reader(const std::string &text) : t(text) {}

	void ws()
	{
		while (i < t.size() && (t[i] == ' ' || t[i] == '\n' || t[i] == '\t' || t[i] == '\r'))
			++i;
	}
	bool eat(char c)
	{
		ws();
		if (i < t.size() && t[i] == c) {
			++i;
			return true;
		}
		return ok = false;
	}
	static void utf8(std::string &out, unsigned cp)
	{
		if (cp < 0x80)
			out += static_cast<char>(cp);
		else if (cp < 0x800) {
			out += static_cast<char>(0xC0 | (cp >> 6));
			out += static_cast<char>(0x80 | (cp & 0x3F));
		} else {
			out += static_cast<char>(0xE0 | (cp >> 12));
			out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
			out += static_cast<char>(0x80 | (cp & 0x3F));
		}
	}
	std::string str()
	{
		std::string out;
		if (!eat('"'))
			return out;
		while (i < t.size() && t[i] != '"') {
			const unsigned char c = static_cast<unsigned char>(t[i]);
			if (c < 0x20) { // raw control characters are illegal in JSON
				ok = false;
				return out;
			}
			if (c != '\\') {
				out += t[i++];
				continue;
			}
			++i;
			if (i >= t.size()) {
				ok = false;
				return out;
			}
			const char e = t[i++];
			switch (e) {
			case '"': out += '"'; break;
			case '\\': out += '\\'; break;
			case '/': out += '/'; break;
			case 'n': out += '\n'; break;
			case 'r': out += '\r'; break;
			case 't': out += '\t'; break;
			case 'b': out += '\b'; break;
			case 'f': out += '\f'; break;
			case 'u': {
				if (i + 4 > t.size()) {
					ok = false;
					return out;
				}
				utf8(out, static_cast<unsigned>(std::strtoul(t.substr(i, 4).c_str(), nullptr, 16)));
				i += 4;
				break;
			}
			default: ok = false; return out;
			}
		}
		if (i >= t.size())
			ok = false;
		else
			++i;
		return out;
	}
	Json value()
	{
		Json j;
		ws();
		if (i >= t.size()) {
			ok = false;
			return j;
		}
		if (t[i] == '"') {
			j.kind = Json::Str;
			j.s = str();
		} else if (t[i] == '{') {
			j.kind = Json::Obj;
			++i;
			ws();
			if (i < t.size() && t[i] == '}') {
				++i;
				return j;
			}
			do {
				ws();
				const std::string key = str();
				eat(':');
				j.o[key] = value();
				ws();
			} while (ok && i < t.size() && t[i] == ',' && ++i);
			eat('}');
		} else if (t[i] == '[') {
			j.kind = Json::Arr;
			++i;
			ws();
			if (i < t.size() && t[i] == ']') {
				++i;
				return j;
			}
			do {
				j.a.push_back(value());
				ws();
			} while (ok && i < t.size() && t[i] == ',' && ++i);
			eat(']');
		} else if (t.compare(i, 4, "true") == 0) {
			j.kind = Json::Bool, j.b = true, i += 4;
		} else if (t.compare(i, 5, "false") == 0) {
			j.kind = Json::Bool, i += 5;
		} else {
			char *end = nullptr;
			j.n = std::strtod(t.c_str() + i, &end);
			if (end == t.c_str() + i)
				ok = false;
			j.kind = Json::Num;
			i = static_cast<std::size_t>(end - t.c_str());
		}
		return j;
	}
};

bool parse(const std::string &text, Json &out)
{
	Reader r(text);
	out = r.value();
	r.ws();
	return r.ok && r.i == text.size();
}

Frame frameWith(const std::string &reference, const std::string &verseText)
{
	Frame f;
	f.revision = 7;
	f.visible = true;
	f.reference = reference;
	f.verses.push_back({1, 2, verseText, false});
	return f;
}

void testJsonShape()
{
	Frame f = frameWith("Jean 3:16", "Car Dieu");
	CHECK(toJson(f) == "{\"rev\":7,\"visible\":true,\"theme\":\"lower\",\"page\":1,\"pages\":1,\"reference\":\"Jean 3:16\",\"verses\":[{\"c\":1,\"v\":2,\"t\":\"Car Dieu\"}]}");

	Frame hidden;
	hidden.revision = 8;
	CHECK(toJson(hidden) == "{\"rev\":8,\"visible\":false,\"theme\":\"lower\",\"page\":1,\"pages\":1,\"reference\":\"\",\"verses\":[]}");

	// A hidden frame never carries text, even if the slide still holds some.
	Frame stale = frameWith("Jean 3:16", "Car Dieu");
	stale.visible = false;
	CHECK(toJson(stale).find("Car Dieu") == std::string::npos);
}

void testJsonEscaping()
{
	const std::string nasty = std::string("guillemet \" antislash \\ lt < gt > amp & nl \n cr \r tab \t nul-ish \x01\x1f ") +
				  "e\xCC\x81 \xC3\xA9 \xE2\x80\xA8 \xE2\x80\xA9 \xE2\x80\x99 \xF0\x9F\x95\x8A";
	const std::string json = toJson(frameWith("Réf <b>", nasty));

	CHECK(json.find('\n') == std::string::npos); // one line: required by the SSE framing
	CHECK(json.find('\r') == std::string::npos);
	CHECK(json.find('<') == std::string::npos && json.find('>') == std::string::npos &&
	      json.find('&') == std::string::npos);
	CHECK(json.find("\xE2\x80\xA8") == std::string::npos); // U+2028 would break JS string literals
	CHECK(json.find("\xE2\x80\xA9") == std::string::npos);

	Json j;
	CHECK(parse(json, j));
	CHECK(j.kind == Json::Obj);
	CHECK(j.o["reference"].s == "Réf <b>");
	// U+2028/2029 come back as themselves (escaped on the wire, identical after parsing).
	CHECK(j.o["verses"].a.size() == 1);
	CHECK(j.o["verses"].a[0].o["t"].s == nasty);
}

void testWholeBibleRoundTrip()
{
	// Every chapter, every page, every theme: each frame goes through the JSON and is read back; putting the
	// pieces together gives back each verse of the Bible exactly.
	StageController s(g_bible);
	std::size_t verses = 0, frames = 0;
	bool allGood = true;
	for (const Theme theme : {Theme::LowerThird, Theme::FullScreen, Theme::Minimal}) {
		s.setTheme(theme);
		verses = 0;
		for (int book = 1; book <= 66; ++book) {
			for (int chapter = 1; chapter <= g_bible.chapterCount(book); ++chapter) {
				CHECK(s.showInPreview({book, chapter, 1, chapter, g_bible.verseCount(book, chapter)}));
				CHECK(s.takeOnAir());
				std::map<int, std::string> rebuilt; // verse -> text put together from the pieces read from JSON
				int expectedPage = 1;
				do {
					const Frame f = makeFrame(s, 1);
					Json j;
					++frames;
					if (!parse(toJson(f), j) || j.o["page"].n != expectedPage ||
					    j.o["pages"].n != static_cast<double>(s.programPages().size()) ||
					    j.o["theme"].s != themeName(theme) || j.o["verses"].a.empty()) {
						allGood = false;
						break;
					}
					for (const Json &jv : j.o["verses"].a) {
						const int v = static_cast<int>(jv.o.at("v").n);
						const bool cont = jv.o.count("k") && jv.o.at("k").b;
						if (!cont && rebuilt.count(v))
							allGood = false; // a verse started twice
						rebuilt[v] += jv.o.at("t").s;
					}
					++expectedPage;
				} while (s.nextPage());
				if (rebuilt.size() != static_cast<std::size_t>(g_bible.verseCount(book, chapter)))
					allGood = false;
				for (const auto &[v, text] : rebuilt)
					if (text != std::string(*g_bible.verse({book, chapter, v})))
						allGood = false;
				verses += rebuilt.size();
			}
		}
		CHECK(verses == g_bible.verseCount());
	}
	CHECK(allGood);
	CHECK(frames > 3u * 1189u);
}

void testFrameFollowsTheProgramOnly()
{
	StageController s(g_bible);
	CHECK(!makeFrame(s, 1).visible);                 // nothing yet

	Json j;
	s.showInPreview(resolveQuery("Jn 3:16", g_bible).passage);
	CHECK(!makeFrame(s, 2).visible);                 // preview alone shows nothing on the air
	s.takeOnAir();
	Frame f = makeFrame(s, 3);
	CHECK(f.visible && f.revision == 3 && f.reference == "Jean 3:16");

	s.showInPreview(resolveQuery("Ps 23", g_bible).passage);
	CHECK(makeFrame(s, 4).reference == "Jean 3:16"); // preview moved, frame did not

	s.hideProgram();
	const Frame h = makeFrame(s, 5);
	CHECK(!h.visible && h.verses.empty());
	CHECK(parse(toJson(h), j) && !j.o["visible"].b);
}

void testRoutes()
{
	const auto route = [](const std::string &req, bool *completeOut = nullptr) {
		bool complete = false;
		const Route r = routeRequest(req, complete);
		if (completeOut)
			*completeOut = complete;
		return r;
	};
	const std::string tail = " HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";

	CHECK(route("GET /" + tail) == Route::Page);
	CHECK(route("GET /index.html" + tail) == Route::Page);
	CHECK(route("GET /overlay.css" + tail) == Route::Style);
	CHECK(route("GET /overlay.js" + tail) == Route::Script);
	CHECK(route("GET /state" + tail) == Route::State);
	CHECK(route("GET /events" + tail) == Route::Events);
	CHECK(route("GET /events?x=1" + tail) == Route::Events);
	CHECK(route("GET /?theme=a#b" + tail) == Route::Page);
	CHECK(route("GET /nope" + tail) == Route::NotFound);
	CHECK(route("GET /../../etc/passwd" + tail) == Route::NotFound);
	CHECK(route("GET //state" + tail) == Route::NotFound);
	CHECK(route("GET /state/" + tail) == Route::NotFound);
	CHECK(route("GET /%2e%2e/x" + tail) == Route::NotFound);
	CHECK(route("GET /overlay.js\\..\\x" + tail) == Route::NotFound);

	CHECK(route("POST /state" + tail) == Route::MethodNotAllowed);
	CHECK(route("DELETE /" + tail) == Route::MethodNotAllowed);
	CHECK(route("get /" + tail) == Route::MethodNotAllowed); // methods are case-sensitive
	CHECK(route("GET" + std::string("\r\n\r\n")) == Route::BadRequest);
	CHECK(route("GET /\r\n\r\n") == Route::BadRequest);
	CHECK(route("GET x HTTP/1.1\r\n\r\n") == Route::BadRequest);
	CHECK(route("GET / FTP/1.1\r\n\r\n") == Route::BadRequest);
	CHECK(route("\r\n\r\n") == Route::BadRequest);

	// bare LF line endings are accepted
	CHECK(route("GET /state HTTP/1.1\nHost: x\n\n") == Route::State);

	// an unfinished head asks for more bytes
	bool complete = true;
	route("GET /sta", &complete);
	CHECK(!complete);
	route("GET /state HTTP/1.1\r\nHost: x\r\n", &complete);
	CHECK(!complete);

	// too much without an end: refused, not waited for
	const std::string flood(kMaxHeadBytes + 10, 'a');
	CHECK(route("GET /" + flood, &complete) == Route::TooLarge && complete);
	CHECK(route("GET /state HTTP/1.1\r\nX: " + flood + "\r\n\r\n") == Route::TooLarge);

	// garbage never crashes
	unsigned seed = 12345;
	for (int n = 0; n < 20000; ++n) {
		std::string g;
		const int len = static_cast<int>(seed % 40);
		for (int k = 0; k < len; ++k) {
			seed = seed * 1103515245u + 12345u;
			static const char alphabet[] = "GET /?#.%\\\r\n HTTP1.0:abz\0\xff";
			g += alphabet[(seed >> 16) % (sizeof alphabet - 1)];
		}
		bool c = false;
		(void)routeRequest(g, c);
	}
	CHECK(true);
}

void testResponses()
{
	const std::string ok = okHead("text/html; charset=utf-8", 42);
	CHECK(ok.rfind("HTTP/1.1 200 OK\r\n", 0) == 0);
	CHECK(ok.find("Content-Length: 42\r\n") != std::string::npos);
	CHECK(ok.find("Cache-Control: no-store") != std::string::npos); // a changed overlay is never served stale
	CHECK(ok.size() >= 4 && ok.compare(ok.size() - 4, 4, "\r\n\r\n") == 0);

	const std::string sse = eventStreamHead();
	CHECK(sse.find("text/event-stream") != std::string::npos);
	CHECK(sse.find("Content-Length") == std::string::npos);

	CHECK(errorResponse(Route::NotFound).rfind("HTTP/1.1 404", 0) == 0);
	CHECK(errorResponse(Route::BadRequest).rfind("HTTP/1.1 400", 0) == 0);
	CHECK(errorResponse(Route::MethodNotAllowed).rfind("HTTP/1.1 405", 0) == 0);
	CHECK(errorResponse(Route::TooLarge).rfind("HTTP/1.1 431", 0) == 0);

	CHECK(sseMessage("{\"a\":1}") == "data: {\"a\":1}\n\n");
	CHECK(sseKeepAlive()[0] == ':'); // a comment: clients ignore it
}

} // namespace

int main()
{
	const auto loaded = g_bible.loadFromFile(VYRA_LSG_PATH);
	if (!loaded.ok()) {
		std::printf("FAIL cannot load %s: %s\n", VYRA_LSG_PATH, loaded.detail.c_str());
		return 1;
	}
	testJsonShape();
	testJsonEscaping();
	testWholeBibleRoundTrip();
	testFrameFollowsTheProgramOnly();
	testRoutes();
	testResponses();
	return vyra::testing::finish();
}
