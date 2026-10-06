/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "src/overlay/http_routes.hpp"

namespace vyra::overlay {

Route routeRequest(std::string_view received, bool &complete)
{
	std::size_t end = received.find("\r\n\r\n");
	if (end == std::string_view::npos)
		end = received.find("\n\n");
	complete = end != std::string_view::npos;
	if (!complete) {
		// Not finished: too much without an end is an attack or a mistake, not a slow client.
		if (received.size() > kMaxHeadBytes) {
			complete = true;
			return Route::TooLarge;
		}
		return Route::BadRequest;
	}
	if (end > kMaxHeadBytes)
		return Route::TooLarge;

	// Request line: METHOD SP TARGET SP VERSION
	std::size_t lineEnd = received.find('\n');
	std::string_view line = received.substr(0, lineEnd);
	if (!line.empty() && line.back() == '\r')
		line.remove_suffix(1);

	const std::size_t sp1 = line.find(' ');
	if (sp1 == std::string_view::npos)
		return Route::BadRequest;
	const std::size_t sp2 = line.find(' ', sp1 + 1);
	if (sp2 == std::string_view::npos)
		return Route::BadRequest;
	const std::string_view method = line.substr(0, sp1);
	std::string_view target = line.substr(sp1 + 1, sp2 - sp1 - 1);
	const std::string_view version = line.substr(sp2 + 1);
	if (version.substr(0, 5) != "HTTP/" || target.empty() || target[0] != '/')
		return Route::BadRequest;
	if (method != "GET")
		return Route::MethodNotAllowed;

	const std::size_t query = target.find_first_of("?#");
	if (query != std::string_view::npos)
		target = target.substr(0, query);

	if (target == "/" || target == "/index.html")
		return Route::Page;
	if (target == "/overlay.css")
		return Route::Style;
	if (target == "/overlay.js")
		return Route::Script;
	if (target == "/state")
		return Route::State;
	if (target == "/events")
		return Route::Events;
	return Route::NotFound;
}

std::string okHead(std::string_view contentType, std::size_t length)
{
	std::string head = "HTTP/1.1 200 OK\r\nContent-Type: ";
	head += contentType;
	head += "\r\nContent-Length: " + std::to_string(length);
	head += "\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n";
	return head;
}

std::string eventStreamHead()
{
	return "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream; charset=utf-8\r\n"
	       "Cache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: keep-alive\r\n\r\n";
}

std::string errorResponse(Route route)
{
	const char *status = "404 Not Found";
	switch (route) {
	case Route::BadRequest:
		status = "400 Bad Request";
		break;
	case Route::MethodNotAllowed:
		status = "405 Method Not Allowed";
		break;
	case Route::TooLarge:
		status = "431 Request Header Fields Too Large";
		break;
	default:
		break;
	}
	return std::string("HTTP/1.1 ") + status + "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
}

std::string sseMessage(std::string_view json)
{
	std::string out = "data: ";
	out += json;
	out += "\n\n";
	return out;
}

std::string sseKeepAlive()
{
	return ": keep-alive\n\n";
}

} // namespace vyra::overlay
