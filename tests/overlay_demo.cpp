/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

// Test helper (Linux/macOS): the real overlay server plus the real Preview/Program rules, driven from
// standard input, so that a browser can be tested against it (tests/overlay_e2e_test.py).
//   commands, one per line:  preview <reference> | air | hide | next | previous | quit
// It prints "READY <port>" once listening and "OK" after each command.
// VYRA_LSG_PATH, VYRA_OVERLAY_DIR as in the other tests.

#include <QCoreApplication>
#include <QSocketNotifier>

#include <cstdio>
#include <iostream>
#include <string>

#include "src/bible/bible_module.hpp"
#include "src/overlay/overlay_frame.hpp"
#include "src/overlay/overlay_server.hpp"
#include "src/search/passage_query.hpp"
#include "src/stage/stage_controller.hpp"

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	vyra::bible::BibleModule bible;
	if (!bible.loadFromFile(VYRA_LSG_PATH).ok())
		return 2;
	vyra::stage::StageController stage(bible);
	vyra::overlay::OverlayServer server(VYRA_OVERLAY_DIR);
	if (!server.start(18500, 18599))
		return 3;
	unsigned long long revision = 0;
	server.publish(vyra::overlay::makeFrame(stage, ++revision));
	std::printf("READY %u\n", static_cast<unsigned>(server.port()));
	std::fflush(stdout);

	QSocketNotifier notifier(0, QSocketNotifier::Read);
	QObject::connect(&notifier, &QSocketNotifier::activated, [&] {
		std::string line;
		if (!std::getline(std::cin, line)) {
			app.quit();
			return;
		}
		if (line == "quit") {
			app.quit();
			return;
		}
		bool changesProgram = false;
		if (line.rfind("preview ", 0) == 0) {
			const auto r = vyra::search::resolveQuery(line.substr(8), bible);
			if (r.ok())
				stage.showInPreview(r.passage);
		} else if (line == "air") {
			changesProgram = stage.takeOnAir();
		} else if (line == "hide") {
			changesProgram = stage.hideProgram();
		} else if (line == "next") {
			stage.previewNext();
		} else if (line == "previous") {
			stage.previewPrevious();
		}
		if (changesProgram) // exactly like the dock: only the Program is ever published
			server.publish(vyra::overlay::makeFrame(stage, ++revision));
		std::printf("OK\n");
		std::fflush(stdout);
	});
	return app.exec();
}
