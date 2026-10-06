/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

// Drives the real dock with simulated key presses and clicks (Qt "offscreen" platform, OBS replaced by
// two stubs). It checks what the operator sees: status line, PREVIEW, PROGRAM, enabled buttons.
// Built only when Qt6 Widgets and Test are found. Run with QT_QPA_PLATFORM=offscreen.
// Set OUTDIR to also get a screenshot (m04_live.png).

#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QTextStream>
#include <QtTest/QtTest>
#include <cstdio>
#include <string>
#include "src/bible/bible_module.hpp"
#include "src/stage/stage_controller.hpp"
#include "ui/vyra_dock.hpp"
#include "tests/check.hpp"
static QMap<std::string, std::string> g;
extern "C" const char *obs_module_text(const char *k){auto it=g.find(k);return it==g.end()?k:it.value().c_str();}
extern "C" void obs_log(int, const char *, ...) {}
static QString status(QWidget&d){return d.findChild<QLabel*>("dockStatus")->text();}
static QList<QLabel*> contents(QWidget&d){return d.findChildren<QLabel*>("stageContent");}
static QPushButton* btn(QWidget&d,int i){return d.findChildren<QPushButton*>().at(i);} // 0 prev,1 next,2 hide,3 onair
int main(int c,char**v){QApplication app(c,v);
 QFile f(VYRA_LOCALE_FR);f.open(QIODevice::ReadOnly|QIODevice::Text);QTextStream in(&f);in.setEncoding(QStringConverter::Utf8);
 while(!in.atEnd()){QString l=in.readLine();int e=l.indexOf('=');if(e<0)continue;QString val=l.mid(e+1).trimmed();if(val.startsWith('"'))val=val.mid(1,val.size()-2);g[l.left(e).toStdString()]=val.toStdString();}
 vyra::bible::BibleModule bible; CHECK(bible.loadFromFile(VYRA_LSG_PATH).ok());
 const char* out=getenv("OUTDIR");
 {
  vyra::ui::VyraDock d(&bible); d.resize(420,380); d.show(); app.processEvents();
  auto *edit=d.findChild<QLineEdit*>();
  CHECK(edit->isEnabled());
  CHECK(!btn(d,0)->isEnabled()&&!btn(d,1)->isEnabled()&&!btn(d,2)->isEnabled()&&!btn(d,3)->isEnabled());
  // the program listener hears ON AIR and Hide, and nothing else
  int heard=0; bool lastLive=false;
  d.setProgramListener([&](const vyra::stage::StageController& c){ ++heard; lastLive=c.programLive(); });
  CHECK(heard==1 && !lastLive);                      // told the initial state at once
  // typing feedback
  QTest::keyClicks(edit,"Jn"); CHECK(status(d).contains("chapitre")); 
  QTest::keyClicks(edit," 3:16"); CHECK(status(d).contains("Jean 3:16 (1 versets)")); 
  QTest::keyClick(edit,Qt::Key_Return);
  CHECK(status(d).startsWith("Preview : Jean 3:16"));
  CHECK(contents(d)[0]->isVisible()&&contents(d)[0]->text().contains("Jean 3:16"));
  CHECK(!contents(d)[1]->isVisible());               // nothing on program yet
  CHECK(btn(d,3)->isEnabled()&&btn(d,0)->isEnabled());
  CHECK(heard==1);                                   // preview is not the program: nobody is told
  // Ctrl+Enter -> air
  QTest::keyClick(edit,Qt::Key_Return,Qt::ControlModifier);
  CHECK(heard==2 && lastLive);
  CHECK(status(d).startsWith("EN DIRECT : Jean 3:16"));
  CHECK(contents(d)[1]->isVisible()&&contents(d)[1]->text().contains("Jean 3:16"));
  CHECK(btn(d,2)->isEnabled());
  // Down: preview moves, program does not
  QTest::keyClick(edit,Qt::Key_Down);
  CHECK(heard==2);                                   // moving the preview never tells the listener
  CHECK(edit->text()=="Jean 3:17"); CHECK(contents(d)[0]->text().contains("Jean 3:17")); CHECK(contents(d)[1]->text().contains("Jean 3:16"));
  QTest::keyClick(edit,Qt::Key_Up); QTest::keyClick(edit,Qt::Key_Up);
  CHECK(edit->text()=="Jean 3:15"); CHECK(contents(d)[1]->text().contains("Jean 3:16"));
  // typing an invalid passage then Enter/Ctrl+Enter changes nothing
  edit->clear(); QTest::keyClicks(edit,"Jn 3:99"); CHECK(status(d).contains("n'a que 36 versets"));
  CHECK(d.findChild<QLabel*>("dockStatus")->property("error").toBool());
  QTest::keyClick(edit,Qt::Key_Return); QTest::keyClick(edit,Qt::Key_Return,Qt::ControlModifier);
  CHECK(contents(d)[0]->text().contains("Jean 3:15")); CHECK(contents(d)[1]->text().contains("Jean 3:16"));
  // ambiguous
  edit->clear(); QTest::keyClicks(edit,"Ju 1"); CHECK(status(d).contains("ambigu")); 
  // Esc hides; program caption keeps the passage ready
  QTest::keyClick(edit,Qt::Key_Escape);
  CHECK(heard==3 && !lastLive);
  CHECK(status(d).startsWith("Masqué : Jean 3:16")); CHECK(!contents(d)[1]->isVisible()); CHECK(!btn(d,2)->isEnabled());
  // Esc again: nothing live, nothing happens
  QString s=status(d); QTest::keyClick(edit,Qt::Key_Escape); CHECK(status(d)==s); CHECK(heard==3);
  // Ctrl+Enter with the bar emptied airs the preview
  edit->clear(); QTest::keyClick(edit,Qt::Key_Return,Qt::ControlModifier);
  CHECK(status(d).startsWith("EN DIRECT : Jean 3:15")); CHECK(heard==4 && lastLive);
  // buttons
  QTest::mouseClick(btn(d,2),Qt::LeftButton); CHECK(!contents(d)[1]->isVisible());
  QTest::mouseClick(btn(d,1),Qt::LeftButton); CHECK(contents(d)[0]->text().contains("Jean 3:16"));
  QTest::mouseClick(btn(d,3),Qt::LeftButton); CHECK(contents(d)[1]->text().contains("Jean 3:16"));
  // range + html escaping + long chapter does not crash
  edit->clear(); QTest::keyClicks(edit,"Ps 119"); QTest::keyClick(edit,Qt::Key_Return,Qt::ControlModifier);
  CHECK(status(d).startsWith("EN DIRECT : Psaumes 119"));
  edit->clear(); QTest::keyClicks(edit,"Jn 3:16-18"); QTest::keyClick(edit,Qt::Key_Return);
  d.resize(420,420); app.processEvents(); if(out) d.grab().save(QString("%1/m04_live.png").arg(out));
 }
 { vyra::ui::VyraDock d(nullptr); d.show(); app.processEvents(); auto*e=d.findChild<QLineEdit*>();
   CHECK(!e->isEnabled()); for(int i=0;i<4;++i) CHECK(!btn(d,i)->isEnabled()); }
 return vyra::testing::finish();}
