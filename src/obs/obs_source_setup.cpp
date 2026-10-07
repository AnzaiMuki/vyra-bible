/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "src/obs/obs_source_setup.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <cstring>

namespace vyra::obs {

namespace {

constexpr int kWidth = 1920;
constexpr int kHeight = 1080;

/** True when the existing source already shows @p url at our size. */
bool hasSettings(obs_source_t *source, const std::string &url)
{
	obs_data_t *settings = obs_source_get_settings(source);
	const char *current = obs_data_get_string(settings, "url");
	const bool same = current && url == current && obs_data_get_int(settings, "width") == kWidth &&
			  obs_data_get_int(settings, "height") == kHeight;
	obs_data_release(settings);
	return same;
}

void fill(obs_data_t *settings, const std::string &url)
{
	obs_data_set_string(settings, "url", url.c_str());
	obs_data_set_int(settings, "width", kWidth);
	obs_data_set_int(settings, "height", kHeight);
	obs_data_set_bool(settings, "is_local_file", false);
	// The page is transparent and handles its own fades: OBS must neither pause it nor reload it by itself.
	obs_data_set_bool(settings, "shutdown", false);
	obs_data_set_bool(settings, "restart_when_active", false);
}

} // namespace

SetupResult ensureOverlaySource(const std::string &url)
{
	if (obs_get_source_output_flags(kBrowserSourceId) == 0)
		return SetupResult::NoBrowserSource;

	obs_source_t *sceneSource = obs_frontend_get_current_scene();
	if (!sceneSource)
		return SetupResult::NoScene;
	obs_scene_t *scene = obs_scene_from_source(sceneSource); // owned by sceneSource, not to be released

	SetupResult result = SetupResult::AlreadyReady;
	obs_source_t *source = obs_get_source_by_name(kSourceName);

	if (source && std::strcmp(obs_source_get_id(source), kBrowserSourceId) != 0) {
		// Something else already has our name: never touch it.
		obs_source_release(source);
		obs_source_release(sceneSource);
		return SetupResult::NoBrowserSource;
	}

	if (!source) {
		obs_data_t *settings = obs_data_create();
		fill(settings, url);
		source = obs_source_create(kBrowserSourceId, kSourceName, settings, nullptr);
		obs_data_release(settings);
		if (!source) {
			obs_source_release(sceneSource);
			return SetupResult::NoBrowserSource;
		}
		obs_scene_add(scene, source);
		result = SetupResult::Created;
	} else {
		if (!hasSettings(source, url)) {
			obs_data_t *settings = obs_data_create();
			fill(settings, url);
			obs_source_update(source, settings);
			obs_data_release(settings);
			result = SetupResult::Updated;
		}
		if (!obs_scene_find_source(scene, kSourceName)) {
			obs_scene_add(scene, source);
			if (result == SetupResult::AlreadyReady)
				result = SetupResult::AddedToScene;
		}
	}

	obs_source_release(source);
	obs_source_release(sceneSource);
	return result;
}

} // namespace vyra::obs
