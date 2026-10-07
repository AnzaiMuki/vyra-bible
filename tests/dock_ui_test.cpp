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
#include <QToolButton>
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

 { // quick selection by typing: the number alone follows the reading
  vyra::ui::VyraDock d(&bible); d.resize(420,420); d.show(); app.processEvents();
  auto *edit=d.findChild<QLineEdit*>();
  auto previewText=[&]{ return contents(d)[0]->text(); };
  auto liveText=[&]{ return contents(d)[1]->text(); };
  QTest::keyClicks(edit,"Jn 3:16"); QTest::keyClick(edit,Qt::Key_Return);
  CHECK(edit->text()=="Jean 3:16" && edit->selectedText()=="Jean 3:16");   // selected: the next number replaces it
  QTest::keyClicks(edit,"17"); CHECK(status(d).startsWith("Jean 3:17 (1 versets)"));
  QTest::keyClick(edit,Qt::Key_Return);
  CHECK(previewText().contains("Jean 3:17") && edit->text()=="Jean 3:17" && edit->selectedText()=="Jean 3:17");
  QTest::keyClicks(edit,"19-21"); QTest::keyClick(edit,Qt::Key_Return);
  CHECK(previewText().contains("Jean 3:19\xE2\x80\x93" "21"));
  QTest::keyClicks(edit,"4:1"); QTest::keyClick(edit,Qt::Key_Return);
  CHECK(previewText().contains("Jean 4:1") && !previewText().contains("Jean 4:19"));
  QTest::keyClicks(edit,"5"); QTest::keyClick(edit,Qt::Key_Return,Qt::ControlModifier);   // straight on the air
  CHECK(liveText().contains("Jean 4:5") && edit->text()=="Jean 4:5");
  // an impossible number changes nothing and says why
  QTest::keyClicks(edit,"99"); CHECK(status(d).contains("n'a que 54 versets"));
  QTest::keyClick(edit,Qt::Key_Return); QTest::keyClick(edit,Qt::Key_Return,Qt::ControlModifier);
  CHECK(previewText().contains("Jean 4:5") && liveText().contains("Jean 4:5"));
  // the picker followed: chapter 4 of John, verse 5 is the preview and the live one
  auto*v5=d.findChild<QToolButton*>("verse-5"); CHECK(v5 && v5->property("preview").toBool() && v5->property("live").toBool());
  CHECK(d.findChild<QToolButton*>("verse-54") && !d.findChild<QToolButton*>("verse-55"));
 }
 { // quick selection by mouse: Books -> Chapters -> Verses
  vyra::ui::VyraDock d(&bible); d.resize(420,420); d.show(); app.processEvents();
  auto *edit=d.findChild<QLineEdit*>();
  int heard=0; bool live=false; d.setProgramListener([&](const vyra::stage::StageController&c){++heard; live=c.programLive();});
  auto cell=[&](const char*n){ auto*b=d.findChild<QToolButton*>(n); CHECK(b!=nullptr); return b; };
  CHECK(!d.findChild<QToolButton*>("chapter-1"));                     // nothing to pick before a book
  QTest::mouseClick(cell("book-43"),Qt::LeftButton);
  CHECK(edit->text()=="Jean " && status(d).contains("tapez le chapitre"));
  CHECK(cell("chapter-21") && !d.findChild<QToolButton*>("chapter-22"));
  QTest::mouseClick(cell("chapter-3"),Qt::LeftButton);
  CHECK(edit->text()=="Jean 3:");
  CHECK(cell("verse-36") && !d.findChild<QToolButton*>("verse-37"));
  CHECK(contents(d)[0]->isHidden() || !contents(d)[0]->text().contains("Jean"));   // choosing a chapter previews nothing yet
  QTest::mouseClick(cell("verse-16"),Qt::LeftButton);
  CHECK(contents(d)[0]->text().contains("Jean 3:16") && edit->text()=="Jean 3:16");
  CHECK(cell("verse-16")->property("preview").toBool() && !cell("verse-17")->property("preview").toBool());
  CHECK(heard==1);                                                    // clicking previews only: the live screen is untouched
  QTest::mouseClick(cell("verse-18"),Qt::LeftButton,Qt::ShiftModifier);
  CHECK(contents(d)[0]->text().contains("Jean 3:16\xE2\x80\x93" "18"));
  for(int v=16;v<=18;++v) CHECK(cell(QString("verse-%1").arg(v).toUtf8().constData())->property("preview").toBool());
  CHECK(!cell("verse-19")->property("preview").toBool());
  QTest::mouseClick(cell("verse-20"),Qt::LeftButton,Qt::ControlModifier);       // Ctrl+click: on the air
  CHECK(heard==2 && live && contents(d)[1]->text().contains("Jean 3:20"));
  CHECK(cell("verse-20")->property("live").toBool() && cell("verse-20")->property("preview").toBool());
  QTest::mouseDClick(cell("verse-25"),Qt::LeftButton);                          // double-click: on the air
  CHECK(contents(d)[1]->text().contains("Jean 3:25"));
  CHECK(!cell("verse-20")->property("live").toBool() && cell("verse-25")->property("live").toBool());
  // a shift+click with no anchor in this chapter is a plain click
  QTest::mouseClick(cell("chapter-3"),Qt::LeftButton);
  QTest::mouseClick(cell("book-19"),Qt::LeftButton); QTest::mouseClick(cell("chapter-23"),Qt::LeftButton);
  QTest::mouseClick(cell("verse-4"),Qt::LeftButton,Qt::ShiftModifier);
  CHECK(contents(d)[0]->text().contains("Psaumes 23:4") && !contents(d)[0]->text().contains("Psaumes 23:4\xE2"));
  // typing after a picker click follows the picked chapter
  QTest::keyClicks(edit,"5"); QTest::keyClick(edit,Qt::Key_Return);
  CHECK(contents(d)[0]->text().contains("Psaumes 23:5"));
  // the picker moves with typed passages too
  QTest::keyClicks(edit,"Ap 22:21"); QTest::keyClick(edit,Qt::Key_Return);
  CHECK(cell("verse-21")->property("preview").toBool() && !d.findChild<QToolButton*>("verse-22"));
  if(out) d.grab().save(QString("%1/m05b_picker.png").arg(out));
 }
 { vyra::ui::VyraDock d(nullptr); d.show(); app.processEvents(); auto*e=d.findChild<QLineEdit*>();
   CHECK(!e->isEnabled()); for(int i=0;i<4;++i) CHECK(!btn(d,i)->isEnabled()); }
 return vyra::testing::finish();}
