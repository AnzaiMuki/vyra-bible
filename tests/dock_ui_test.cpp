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
#include <QComboBox>
#include <QListWidget>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QToolButton>
#include <QTextStream>
#include <QtTest/QtTest>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include "src/bible/bible_module.hpp"
#include "src/search/passage_query.hpp"
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
 { // themes and pages: long passage on the air is paged, short one is not; only the program moves
  vyra::ui::VyraDock d(&bible); d.show(); app.processEvents();
  int heard=0; std::string theme; std::size_t page=99,pages=0;
  d.setProgramListener([&](const vyra::stage::StageController&s){ ++heard; theme=vyra::stage::themeName(s.theme()); page=s.programPage(); pages=s.programPages().size(); });
  auto*edit=d.findChild<QLineEdit*>(); auto*box=d.findChild<QComboBox*>("themeBox"); auto*lab=d.findChild<QLabel*>("pageLabel");
  QPushButton*pp=nullptr,*pn=nullptr;
  for(auto*b:d.findChildren<QPushButton*>()){ if(b->text().contains("Page")&&b->text().startsWith(QString::fromUtf8("\xE2\x97\x80"))) pp=b; else if(b->text().startsWith("Page")) pn=b; }
  CHECK(box && lab && pp && pn && box->count()==3);
  CHECK(theme=="lower" && !pn->isVisible());
  QTest::keyClicks(edit,"Jn 3:16"); QTest::keyClick(edit,Qt::Key_Return); QTest::keyClick(edit,Qt::Key_Return,Qt::ControlModifier);
  CHECK(pages==1 && lab->text().isEmpty() && !pn->isVisible());
  edit->selectAll(); QTest::keyClicks(edit,"Ps 119:1-40"); QTest::keyClick(edit,Qt::Key_Return);
  CHECK(pages==1);                                                    // preview only: the program is untouched
  QTest::keyClick(edit,Qt::Key_Return,Qt::ControlModifier); app.processEvents();
  CHECK(pages>1 && page==0 && pn->isVisible() && lab->text()==QString("Page 1/%1").arg(pages));
  CHECK(!pp->isEnabled() && pn->isEnabled());
  const int before=heard; const auto previewText=contents(d)[0]->text();
  QTest::keyClick(edit,Qt::Key_PageDown); CHECK(page==1 && heard==before+1 && lab->text().startsWith("Page 2/"));
  CHECK(contents(d)[0]->text()==previewText);                        // pages never touch the preview
  QTest::mouseClick(pn,Qt::LeftButton); CHECK(page==2);
  QTest::mouseClick(pp,Qt::LeftButton); CHECK(page==1);
  QTest::keyClick(edit,Qt::Key_PageUp); CHECK(page==0);
  const int h2=heard; QTest::keyClick(edit,Qt::Key_PageUp); CHECK(page==0 && heard==h2); // at the first page: nothing sent
  for(std::size_t i=1;i<pages;++i) QTest::keyClick(edit,Qt::Key_PageDown);
  CHECK(page==pages-1 && !pn->isEnabled()); const int h3=heard; QTest::keyClick(edit,Qt::Key_PageDown); CHECK(heard==h3);
  // a theme change is heard, re-cuts the pages (page 1) and is kept for the next passages
  box->setCurrentIndex(1); QMetaObject::invokeMethod(box,"activated",Q_ARG(int,1)); app.processEvents();
  CHECK(theme=="full" && page==0 && heard==h3+1);
  QTest::keyClick(edit,Qt::Key_Escape);                              // hidden: page keys do nothing
  const int h4=heard; QTest::keyClick(edit,Qt::Key_PageDown); CHECK(heard==h4 && !pn->isEnabled());
  if(out) d.grab().save(QString("%1/m07_dock.png").arg(out));
 }
 { // hotkey actions behave exactly like the buttons; only OnAir changes the program verse
  using A=vyra::stage::OperatorAction;
  vyra::ui::VyraDock d(&bible); d.show(); app.processEvents();
  int heard=0; bool live=false; std::string ref;
  d.setProgramListener([&](const vyra::stage::StageController&s){ ++heard; live=s.programLive(); ref=s.program()?vyra::search::formatPassage(*s.program(),bible):""; });
  const int h0=heard;
  d.perform(A::OnAir); d.perform(A::Hide); d.perform(A::PageNext); d.perform(A::PagePrevious); d.perform(A::PreviewNext);
  CHECK(heard==h0 && !live);                                           // nothing in preview yet: nothing happens
  auto*edit=d.findChild<QLineEdit*>();
  QTest::keyClicks(edit,"Jn 3:16"); QTest::keyClick(edit,Qt::Key_Return);
  d.perform(A::PreviewNext); CHECK(contents(d)[0]->text().contains("Jean 3:17") && heard==h0);   // preview moves, program does not
  d.perform(A::PreviewPrevious); d.perform(A::PreviewPrevious); CHECK(contents(d)[0]->text().contains("Jean 3:15") && heard==h0);
  d.perform(A::OnAir); CHECK(live && ref.find("3:15")!=std::string::npos && heard==h0+1);
  d.perform(A::PreviewNext); CHECK(ref.find("3:15")!=std::string::npos && heard==h0+1);          // stepping never touches the air
  d.perform(A::Hide); CHECK(!live && heard==h0+2);
  d.perform(A::Hide); CHECK(heard==h0+2);                                                       // already hidden
 }
 { // history and favorites: recorded on ON AIR only, usable by click, kept on disk
  namespace fs=std::filesystem;
  const fs::path dir=fs::temp_directory_path()/"vyra_dock_lib_test"; fs::remove_all(dir);
  const fs::path file=dir/"library.txt";
  auto items=[](QListWidget*l){ QStringList r; for(int i=0;i<l->count();++i) r<<l->item(i)->text(); return r; };
  {
   vyra::ui::VyraDock d(&bible); d.setLibraryFile(file); d.show(); app.processEvents();
   int heard=0; bool live=false; std::string ref;
   d.setProgramListener([&](const vyra::stage::StageController&s){ ++heard; live=s.programLive(); ref=s.program()?vyra::search::formatPassage(*s.program(),bible):""; });
   auto*edit=d.findChild<QLineEdit*>(); auto*hist=d.findChild<QListWidget*>("historyList"); auto*fav=d.findChild<QListWidget*>("favoritesList");
   auto*favBtn=d.findChild<QPushButton*>("favoriteButton"); auto*tabs=d.findChild<QTabWidget*>("quickTabs");
   CHECK(hist && fav && favBtn && tabs && tabs->count()==3 && hist->count()==0 && !favBtn->isEnabled());
   QTest::keyClicks(edit,"Jn 3:16"); QTest::keyClick(edit,Qt::Key_Return);
   CHECK(hist->count()==0 && favBtn->isEnabled());                          // previewing is not airing: no history
   QTest::keyClick(edit,Qt::Key_Return,Qt::ControlModifier);
   CHECK(items(hist)==QStringList{"Jean 3:16"});
   QTest::keyClicks(edit,"Ps 23"); QTest::keyClick(edit,Qt::Key_Return,Qt::ControlModifier);
   QTest::keyClicks(edit,"Rm 8:28"); QTest::keyClick(edit,Qt::Key_Return);   // previewed only
   CHECK(items(hist)==QStringList({"Psaumes 23","Jean 3:16"}));
   // favorites: button and Ctrl+D act on the PREVIEW
   CHECK(favBtn->text().startsWith(QString::fromUtf8("\xE2\x98\x86")));
   QTest::mouseClick(favBtn,Qt::LeftButton); CHECK(items(fav)==QStringList{"Romains 8:28"} && favBtn->text().startsWith(QString::fromUtf8("\xE2\x98\x85")));
   CHECK(favBtn->property("favorite").toBool());
   QTest::keyClick(edit,Qt::Key_D,Qt::ControlModifier); CHECK(fav->count()==0 && !favBtn->property("favorite").toBool());
   QTest::keyClick(edit,Qt::Key_D,Qt::ControlModifier); CHECK(fav->count()==1);
   CHECK(heard>=2 && ref=="Psaumes 23");                                    // favorites never touch the air
   const int h=heard;
   // click = preview, double-click = on air (and goes to the top of the history); the operator opens the tab first
   tabs->setCurrentIndex(1); app.processEvents();
   QTest::mouseClick(hist->viewport(),Qt::LeftButton,Qt::NoModifier,hist->visualItemRect(hist->item(1)).center()); app.processEvents();
   CHECK(contents(d)[0]->text().contains("Jean 3:16") && heard==h && ref=="Psaumes 23");
   QTest::mouseDClick(hist->viewport(),Qt::LeftButton,Qt::NoModifier,hist->visualItemRect(hist->item(1)).center()); app.processEvents();
   CHECK(heard==h+1 && ref=="Jean 3:16" && live && items(hist)==QStringList({"Jean 3:16","Psaumes 23"}));
   tabs->setCurrentIndex(2); QTest::qWait(QApplication::doubleClickInterval()+50); // else Qt may read the click as part of the previous double-click
   QTest::mouseClick(fav->viewport(),Qt::LeftButton,Qt::NoModifier,fav->visualItemRect(fav->item(0)).center()); app.processEvents();
   CHECK(contents(d)[0]->text().contains("Romains 8:28") && ref=="Jean 3:16");   // one click: preview only
   QTest::mouseDClick(fav->viewport(),Qt::LeftButton,Qt::NoModifier,fav->visualItemRect(fav->item(0)).center()); app.processEvents();
   CHECK(ref=="Romains 8:28" && items(hist)[0]=="Romains 8:28" && edit->text()=="Romains 8:28");
   if(out) { tabs->setCurrentIndex(1); app.processEvents(); d.grab().save(QString("%1/m09_history.png").arg(out)); }
  }
  { // a new dock (OBS restarted) finds everything again
   vyra::ui::VyraDock d(&bible); d.setLibraryFile(file); d.show(); app.processEvents();
   auto*hist=d.findChild<QListWidget*>("historyList"); auto*fav=d.findChild<QListWidget*>("favoritesList");
   CHECK(items(hist)==QStringList({"Romains 8:28","Jean 3:16","Psaumes 23"}) && items(fav)==QStringList{"Romains 8:28"});
   for(auto*b:d.findChildren<QPushButton*>()) if(b->text()=="Effacer l'historique"){ QTest::mouseClick(b,Qt::LeftButton); CHECK(hist->count()==0 && !b->isEnabled()); }
   CHECK(fav->count()==1);                                                  // clearing the history keeps the favorites
  }
  { vyra::ui::VyraDock d(&bible); d.setLibraryFile(file); d.show(); app.processEvents();
    CHECK(d.findChild<QListWidget*>("historyList")->count()==0 && d.findChild<QListWidget*>("favoritesList")->count()==1); }
  { // a damaged file: what is readable is kept, the rest is reported in red, nothing crashes
    { std::ofstream o(file,std::ios::binary|std::ios::trunc); o<<"VYRA-LIBRARY 1\nF 43 3 16 3 16\nF garbage\nH 43 3 16 3 99\n"; }
    vyra::ui::VyraDock d(&bible); d.setLibraryFile(file); d.show(); app.processEvents();
    CHECK(d.findChild<QListWidget*>("favoritesList")->count()==1 && d.findChild<QListWidget*>("historyList")->count()==0);
    CHECK(status(d).contains("2") && d.findChild<QLabel*>("dockStatus")->property("error").toBool());
  }
  { // a library path that cannot be written: the dock keeps working and says so
    vyra::ui::VyraDock d(&bible); d.setLibraryFile(file/"impossible"/"x.txt"); d.show(); app.processEvents();
    auto*edit=d.findChild<QLineEdit*>(); QTest::keyClicks(edit,"Jn 3:16"); QTest::keyClick(edit,Qt::Key_Return,Qt::ControlModifier);
    CHECK(d.findChild<QListWidget*>("historyList")->count()==1 && status(d).contains("Impossible d'enregistrer"));
  }
  { // favorites cap
    vyra::ui::VyraDock d(&bible); d.show(); app.processEvents();
    auto*edit=d.findChild<QLineEdit*>(); auto*favBtn=d.findChild<QPushButton*>("favoriteButton");
    for(int c=1;c<=100;++c) for(int v=1;v<=2;++v){ edit->selectAll(); QTest::keyClicks(edit,QString("Ps %1:%2").arg(c).arg(v)); QTest::keyClick(edit,Qt::Key_Return); QTest::mouseClick(favBtn,Qt::LeftButton); }
    CHECK(d.findChild<QListWidget*>("favoritesList")->count()==200);
    edit->selectAll(); QTest::keyClicks(edit,"Jn 3:16"); QTest::keyClick(edit,Qt::Key_Return); QTest::mouseClick(favBtn,Qt::LeftButton);
    CHECK(d.findChild<QListWidget*>("favoritesList")->count()==200 && status(d).contains("200"));
  }
  fs::remove_all(dir);
 }
 { // the "add the source to OBS" button exists only when something can do it, and shows what happened
  vyra::ui::VyraDock d(&bible); d.show(); app.processEvents();
  auto*add=d.findChild<QPushButton*>("addSourceButton"); CHECK(add && !add->isVisible());
  int calls=0; d.setAddSourceHandler([&]{ ++calls; return QString("Source creee"); });
  app.processEvents(); CHECK(add->isVisible());
  QTest::mouseClick(add,Qt::LeftButton); CHECK(calls==1 && status(d)=="Source creee");
 }
 { vyra::ui::VyraDock d(nullptr); d.show(); app.processEvents(); auto*e=d.findChild<QLineEdit*>();
   CHECK(!e->isEnabled()); for(int i=0;i<4;++i) CHECK(!btn(d,i)->isEnabled()); }
 return vyra::testing::finish();}
