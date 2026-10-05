/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

/*
 * Plugin entry point. Its only job is to register VYRA Bible with OBS:
 * today the operator dock, later the hotkeys and the local server.
 * No business logic lives here.
 */

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include <QWidget>

#include "ui/vyra_dock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

namespace {

constexpr const char *kDockId = "vyra-bible-dock";

bool g_dockRegistered = false;

} // namespace

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "loading (version %s)", PLUGIN_VERSION);

	auto *mainWindow = static_cast<QWidget *>(obs_frontend_get_main_window());
	if (!mainWindow) {
		obs_log(LOG_ERROR, "OBS main window is not available, the VYRA Bible dock cannot be created");
		return false;
	}

	auto *dock = new vyra::ui::VyraDock(mainWindow);

	// On success OBS takes ownership of the widget (it is wrapped in a QDockWidget).
	if (!obs_frontend_add_dock_by_id(kDockId, obs_module_text("Dock.Title"), dock)) {
		obs_log(LOG_ERROR, "OBS refused to register the dock '%s'", kDockId);
		delete dock;
		return false;
	}

	g_dockRegistered = true;
	obs_log(LOG_INFO, "dock registered");
	return true;
}

void obs_module_unload(void)
{
	if (g_dockRegistered) {
		obs_frontend_remove_dock(kDockId);
		g_dockRegistered = false;
	}
	obs_log(LOG_INFO, "unloaded");
}
