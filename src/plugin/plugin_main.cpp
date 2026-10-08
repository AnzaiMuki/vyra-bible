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

#include <QMetaObject>
#include <QPointer>
#include <QString>
#include <QWidget>

#include <filesystem>
#include <memory>

#include "src/bible/bible_module.hpp"
#include "src/obs/hotkeys.hpp"
#include "src/obs/obs_source_setup.hpp"
#include "src/overlay/overlay_frame.hpp"
#include "src/overlay/overlay_server.hpp"
#include "ui/vyra_dock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

namespace {

constexpr const char *kDockId = "vyra-bible-dock";

bool g_dockRegistered = false;

// The Bible shown by the plugin. Later milestones hand it to the search engine.
std::unique_ptr<vyra::bible::BibleModule> g_bible;

// The local web server behind the Browser Source. Owned here; the dock only holds a QPointer-guarded callback.
std::unique_ptr<vyra::overlay::OverlayServer> g_overlay;
unsigned long long g_overlayRevision = 0;

constexpr quint16 kOverlayFirstPort = 17420;
constexpr quint16 kOverlayLastPort = 17429;

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

/**
 * Starts the overlay server. Returns the line to show in the dock.
 * Never throws and never stops OBS: without a server the dock still works, it just cannot feed OBS.
 */
QString startOverlay(bool &ok)
{
	char *rawDir = obs_module_file("overlay");
	if (!rawDir) {
		obs_log(LOG_ERROR, "overlay folder not found in the plugin data folder");
		ok = false;
		return text("Error.OverlayMissing");
	}
	const QString dir = QString::fromUtf8(rawDir);
	bfree(rawDir);

	g_overlay = std::make_unique<vyra::overlay::OverlayServer>(dir);
	QString error;
	if (!g_overlay->start(kOverlayFirstPort, kOverlayLastPort, &error)) {
		obs_log(LOG_ERROR, "overlay server cannot listen on ports %d-%d: %s", kOverlayFirstPort,
			kOverlayLastPort, error.toUtf8().constData());
		g_overlay.reset();
		ok = false;
		return text("Error.OverlayPort").arg(kOverlayFirstPort).arg(kOverlayLastPort);
	}
	const QString url = QStringLiteral("http://127.0.0.1:%1/").arg(g_overlay->port());
	obs_log(LOG_INFO, "overlay server listening on %s", url.toUtf8().constData());
	ok = true;
	return text("Status.OverlayRunning").arg(url);
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

	bool overlayOk = false;
	const QString overlayStatus = startOverlay(overlayOk);

	auto *dock = new vyra::ui::VyraDock(g_bible.get(), mainWindow);
	dock->setStatus(bibleStatus + QLatin1Char('\n') + overlayStatus, !bibleOk || !overlayOk);

	if (g_bible) {
		// History and favorites live in the OBS config folder of the plugin, next to the other plugin settings.
		char *rawLibrary = obs_module_config_path("library.txt");
		if (rawLibrary) {
			dock->setLibraryFile(std::filesystem::u8path(rawLibrary));
			bfree(rawLibrary);
		}
	}

	if (g_overlay) {
		QPointer<vyra::overlay::OverlayServer> portSource(g_overlay.get());
		dock->setAddSourceHandler([portSource]() -> QString {
			if (!portSource)
				return text("Error.OverlayPort").arg(kOverlayFirstPort).arg(kOverlayLastPort);
			const std::string url = QStringLiteral("http://127.0.0.1:%1/").arg(portSource->port()).toStdString();
			using vyra::obs::SetupResult;
			switch (vyra::obs::ensureOverlaySource(url)) {
			case SetupResult::Created:
				return text("Status.SourceCreated");
			case SetupResult::Updated:
				return text("Status.SourceUpdated");
			case SetupResult::AddedToScene:
				return text("Status.SourceAdded");
			case SetupResult::AlreadyReady:
				return text("Status.SourceReady");
			case SetupResult::NoScene:
				return text("Error.NoScene");
			case SetupResult::NoBrowserSource:
				break;
			}
			return text("Error.NoBrowserSource");
		});
		QPointer<vyra::overlay::OverlayServer> server(g_overlay.get());
		dock->setProgramListener([server](const vyra::stage::StageController &stage) {
			if (server) // the server may be gone while OBS is still closing the dock
				server->publish(vyra::overlay::makeFrame(stage, ++g_overlayRevision));
		});
	}

	// Global hotkeys. OBS calls them from its own thread: hand the action to the Qt thread and return.
	{
		QPointer<vyra::ui::VyraDock> target(dock);
		vyra::obs::registerHotkeys([target](vyra::stage::OperatorAction action) {
			if (!target)
				return;
			QMetaObject::invokeMethod(
				target.data(), [target, action] { if (target) target->perform(action); },
				Qt::QueuedConnection);
		});
	}

	// On success OBS takes ownership of the widget (it is wrapped in a QDockWidget).
	if (!obs_frontend_add_dock_by_id(kDockId, obs_module_text("Dock.Title"), dock)) {
		obs_log(LOG_ERROR, "OBS refused to register the dock '%s'", kDockId);
		vyra::obs::unregisterHotkeys();
		delete dock;
		g_overlay.reset();
		return false;
	}

	g_dockRegistered = true;
	obs_log(LOG_INFO, "dock registered");
	return true;
}

void obs_module_unload(void)
{
	vyra::obs::unregisterHotkeys();
	if (g_dockRegistered) {
		obs_frontend_remove_dock(kDockId);
		g_dockRegistered = false;
	}
	g_overlay.reset();
	g_bible.reset();
	obs_log(LOG_INFO, "unloaded");
}
