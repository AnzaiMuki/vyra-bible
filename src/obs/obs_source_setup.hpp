/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

/*
 * Creates (or updates) the Browser Source that shows the overlay, so that the operator does not
 * have to type the address by hand. The only file of the plugin, with plugin_main, that calls libobs
 * to create sources; everything else stays independent of OBS.
 */

#include <string>

namespace vyra::obs {

enum class SetupResult {
	Created,         // a new source was created and added to the current scene
	Updated,         // the source existed: its address was corrected
	AddedToScene,    // the source existed elsewhere and was added to the current scene
	AlreadyReady,    // nothing to do
	NoBrowserSource, // this OBS has no browser source (the OBS "browser" component is missing)
	NoScene,         // no current scene
};

constexpr const char *kSourceName = "VYRA Bible";

/** Name of the browser source type in OBS. */
constexpr const char *kBrowserSourceId = "browser_source";

/**
 * Makes sure a Browser Source named "VYRA Bible" exists, points at @p url (1920x1080) and is in the
 * current scene. Never removes or renames anything. Must be called from the OBS UI thread.
 */
SetupResult ensureOverlaySource(const std::string &url);

} // namespace vyra::obs
