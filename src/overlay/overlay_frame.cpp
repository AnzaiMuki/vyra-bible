/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "src/overlay/overlay_frame.hpp"

#include <cstdio>

namespace vyra::overlay {

namespace {

void appendString(std::string &out, const std::string &s)
{
	out += '"';
	for (std::size_t i = 0; i < s.size(); ++i) {
		const unsigned char c = static_cast<unsigned char>(s[i]);
		switch (c) {
		case '"':
			out += "\\\"";
			break;
		case '\\':
			out += "\\\\";
			break;
		case '\n':
			out += "\\n";
			break;
		case '\r':
			out += "\\r";
			break;
		case '\t':
			out += "\\t";
			break;
		case '<':
		case '>':
		case '&': {
			char buf[8];
			std::snprintf(buf, sizeof buf, "\\u%04x", c);
			out += buf;
			break;
		}
		default:
			if (c < 0x20) {
				char buf[8];
				std::snprintf(buf, sizeof buf, "\\u%04x", c);
				out += buf;
			} else if (c == 0xE2 && i + 2 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0x80 &&
				   (static_cast<unsigned char>(s[i + 2]) == 0xA8 ||
				    static_cast<unsigned char>(s[i + 2]) == 0xA9)) {
				out += static_cast<unsigned char>(s[i + 2]) == 0xA8 ? "\\u2028" : "\\u2029";
				i += 2;
			} else {
				out += static_cast<char>(c);
			}
		}
	}
	out += '"';
}

} // namespace

Frame makeFrame(const stage::StageController &controller, unsigned long long revision)
{
	Frame frame;
	frame.revision = revision;
	frame.theme = stage::themeName(controller.theme());
	if (controller.programLive() && !controller.programPages().empty()) {
		frame.visible = true;
		frame.reference = search::formatPassage(*controller.program(), controller.module());
		frame.pages = static_cast<int>(controller.programPages().size());
		frame.page = static_cast<int>(controller.programPage()) + 1;
		frame.verses = controller.programPages()[controller.programPage()].verses;
	}
	return frame;
}

std::string toJson(const Frame &frame)
{
	std::string out = "{\"rev\":" + std::to_string(frame.revision);
	out += frame.visible ? ",\"visible\":true" : ",\"visible\":false";
	out += ",\"theme\":";
	appendString(out, frame.theme);
	out += ",\"page\":" + std::to_string(frame.visible ? frame.page : 1);
	out += ",\"pages\":" + std::to_string(frame.visible ? frame.pages : 1);
	out += ",\"reference\":";
	appendString(out, frame.visible ? frame.reference : std::string());
	out += ",\"verses\":[";
	if (frame.visible) {
		bool first = true;
		for (const stage::PageVerse &v : frame.verses) {
			if (!first)
				out += ',';
			first = false;
			out += "{\"c\":" + std::to_string(v.chapter) + ",\"v\":" + std::to_string(v.verse) + ",\"t\":";
			appendString(out, v.text);
			if (v.continuation)
				out += ",\"k\":true";
			out += '}';
		}
	}
	out += "]}";
	return out;
}

} // namespace vyra::overlay
