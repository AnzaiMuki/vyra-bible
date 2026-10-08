/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

// Timing of what the operator feels in the dock (not a pass/fail test: timings depend on the machine).
// Build in Release, run with QT_QPA_PLATFORM=offscreen. Prints milliseconds per action.

#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QtTest/QtTest>
#include <chrono>
#include <cstdio>
#include <map>
#include <string>
#include "src/bible/bible_module.hpp"
#include "ui/vyra_dock.hpp"

static std::map<std::string, std::string> g;
extern "C" const char *obs_module_text(const char *k) { auto i = g.find(k); return i == g.end() ? k : i->second.c_str(); }
extern "C" void obs_log(int, const char *, ...) {}

using Clock = std::chrono::steady_clock;
static double since(Clock::time_point t) { return std::chrono::duration<double, std::milli>(Clock::now() - t).count(); }

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	vyra::bible::BibleModule bible;
	if (!bible.loadFromFile(VYRA_LSG_PATH).ok())
		return 1;
	auto t = Clock::now();
	vyra::ui::VyraDock d(&bible);
	d.resize(420, 700);
	d.show();
	app.processEvents();
	std::printf("dock creation + first show: %.1f ms\n", since(t));

	auto *edit = d.findChild<QLineEdit *>();
	auto run = [&](const char *label, int n, auto &&fn) {
		t = Clock::now();
		for (int i = 0; i < n; ++i) {
			fn(i);
			app.processEvents();
		}
		std::printf("%-46s %7.2f ms each (n=%d)\n", label, since(t) / n, n);
	};

	run("type 'Ps 119:1' then Enter (preview)", 20, [&](int) {
		edit->selectAll();
		QTest::keyClicks(edit, "Ps 119:1");
		QTest::keyClick(edit, Qt::Key_Return);
	});
	run("Down arrow: next verse (Ps 119, 176 cells)", 100, [&](int) { QTest::keyClick(edit, Qt::Key_Down); });
	run("Ctrl+Enter: on air a verse", 100, [&](int) {
		QTest::keyClick(edit, Qt::Key_Down);
		QTest::keyClick(edit, Qt::Key_Return, Qt::ControlModifier);
	});
	run("type '17' + Enter (relative shorthand)", 50, [&](int) {
		edit->selectAll();
		QTest::keyClicks(edit, "17");
		QTest::keyClick(edit, Qt::Key_Return);
	});
	run("Ps 119 whole chapter, then on air", 20, [&](int) {
		edit->selectAll();
		QTest::keyClicks(edit, "Ps 119");
		QTest::keyClick(edit, Qt::Key_Return, Qt::ControlModifier);
	});
	run("PageDown (next page of the program)", 50, [&](int) { QTest::keyClick(edit, Qt::Key_PageDown); });
	run("jump across books (Gn 1:1, Ap 22:21)", 40, [&](int i) {
		edit->selectAll();
		QTest::keyClicks(edit, i % 2 ? "Ap 22:21" : "Gn 1:1");
		QTest::keyClick(edit, Qt::Key_Return);
	});
	return 0;
}
