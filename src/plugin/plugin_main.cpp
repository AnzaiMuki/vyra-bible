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

#include <QString>
#include <QWidget>

#include <filesystem>
#include <memory>

#include "src/bible/bible_module.hpp"
#include "ui/vyra_dock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

namespace {

constexpr const char *kDockId = "vyra-bible-dock";

bool g_dockRegistered = false;

// The Bible shown by the plugin. Later milestones hand it to the search engine.
std::unique_ptr<vyra::bible::BibleModule> g_bible;

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

/**
 * Loads the bundled Segond 1910 and returns the line to show in the dock.
 * Never throws and never stops OBS: a missing or corrupt Bible only disables the plugin's content.
 */
QString loadBundledBible(bool &ok)
{
	using vyra::bible::LoadError;

	g_bible = std::make_unique<vyra::bible::BibleModule>();

	char *rawPath = obs_module_file("bibles/lsg1910.tsv");
	if (!rawPath) {
		obs_log(LOG_ERROR, "Bible file 'bibles/lsg1910.tsv' not found in the plugin data folder");
		g_bible.reset();
		ok = false;
		return text("Error.BibleMissing");
	}
	const std::filesystem::path path = std::filesystem::u8path(rawPath); // UTF-8 path, also on Windows
	bfree(rawPath);

	const vyra::bible::LoadResult result = g_bible->loadFromFile(path);
	if (!result.ok()) {
		obs_log(LOG_ERROR, "cannot load the Bible '%s': error %d, line %d, %s", path.u8string().c_str(),
			static_cast<int>(result.error), result.line, result.detail.c_str());
		g_bible.reset();
		ok = false;
		switch (result.error) {
		case LoadError::FileNotFound:
			return text("Error.BibleMissing");
		case LoadError::BadEncoding:
			return text("Error.BibleEncoding");
		default:
			return text("Error.BibleInvalid");
		}
	}

	const auto &info = g_bible->info();
	obs_log(LOG_INFO, "Bible loaded: %s (%s), %zu verses", info.name.c_str(), info.code.c_str(),
		g_bible->verseCount());
	ok = true;
	return text("Status.BibleLoaded")
		.arg(QString::fromStdString(info.name))
		.arg(static_cast<qulonglong>(g_bible->verseCount()));
}

} // namespace

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "loading (version %s)", PLUGIN_VERSION);

	auto *mainWindow = static_cast<QWidget *>(obs_frontend_get_main_window());
	if (!mainWindow) {
		obs_log(LOG_ERROR, "OBS main window is not available, the VYRA Bible dock cannot be created");
		return false;
	}

	bool bibleOk = false;
	const QString bibleStatus = loadBundledBible(bibleOk);

	auto *dock = new vyra::ui::VyraDock(mainWindow);
	dock->setStatus(bibleStatus, !bibleOk);

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
	g_bible.reset();
	obs_log(LOG_INFO, "unloaded");
}
