/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

/*
 * The tiny HTTP vocabulary of the overlay server, without any socket: read a request head,
 * decide what it asks for, write response heads. Independent of OBS and Qt, so it is tested alone.
 */

#include <string>
#include <string_view>

namespace vyra::overlay {

enum class Route {
	Page,     // GET /            -> index.html
	Style,    // GET /overlay.css
	Script,   // GET /overlay.js
	State,    // GET /state       -> the current frame as JSON
	Events,   // GET /events      -> Server-Sent Events stream
	NotFound, // anything else
	BadRequest,
	MethodNotAllowed,
	TooLarge, // head longer than kMaxHeadBytes
};

constexpr std::size_t kMaxHeadBytes = 8192;

/**
 * Decides the route from the bytes received so far.
 * @param complete  set to false while the end of the head ("\r\n\r\n" or "\n\n") has not arrived yet:
 *                  the caller must wait for more bytes (the returned route is then meaningless).
 * Only GET and HEAD-less GET are served; a query string is ignored; "//" and ".." never match a route.
 */
Route routeRequest(std::string_view received, bool &complete);

/** Head of a 200 response with the given content type and length. Never cached. */
std::string okHead(std::string_view contentType, std::size_t length);
/** Head of the Server-Sent Events response (no length, connection kept open). */
std::string eventStreamHead();
/** Complete short response for an error route (404, 400, 405, 431). */
std::string errorResponse(Route route);
/** One SSE message carrying @p json (a single line, as toJson produces). */
std::string sseMessage(std::string_view json);
/** SSE comment line, sent from time to time so that dead connections are noticed. */
std::string sseKeepAlive();

} // namespace vyra::overlay
