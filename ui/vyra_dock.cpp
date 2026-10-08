/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ui/vyra_dock.hpp"

#include <obs-module.h>
#include <plugin-support.h>

#include <algorithm>

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMetaObject>
#include <QTabBar>
#include <QTabWidget>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include "src/bible/bible_module.hpp"
#include "src/search/book_catalog.hpp"
#include "src/stage/stage_controller.hpp"
#include "ui/brand.hpp"
#include "ui/picker_panel.hpp"
#include "ui/stage_panel.hpp"

namespace vyra::ui {

namespace {

// The look of VYRA Studio (same palette, pill controls, rounded cards) at the density of an OBS dock.
// The whole dock is styled, so it reads the same on every OBS theme; nothing here leaks outside the dock.
//   background #0e1318, card #151c23, screen #06090c, border #293036, control #212a31,
//   accent blue #2e9bff, ok green #33c77b, dim text #5e6b78 / #8795a3, ON AIR red #d92d20.
constexpr const char *kStyleSheet = R"(
#vyraDock {
	background-color: #0e1318;
}
QLabel {
	color: #c9d3dd;
}
#brandTitle {
	color: #f3f6f9;
	font-size: 13px;
	font-weight: 700;
}
#brandSub {
	color: #5e6b78;
	font-size: 9px;
	font-weight: 600;
	letter-spacing: 1.5px;
}
QLineEdit {
	background-color: #0b1015;
	border: 1px solid #293036;
	border-radius: 8px;
	padding: 7px 10px;
	color: #f3f6f9;
	font-size: 13px;
	selection-background-color: #2e9bff;
	selection-color: #ffffff;
}
QLineEdit:focus {
	border: 1px solid #2e9bff;
}
QLineEdit:disabled {
	color: #4a5663;
}
QPushButton {
	background-color: #212a31;
	color: #d5dde5;
	border: none;
	border-radius: 7px;
	padding: 6px 9px;
	font-weight: 600;
}
QPushButton:hover {
	background-color: #2a353e;
}
QPushButton:pressed {
	background-color: #1a2229;
}
QPushButton:disabled {
	background-color: #161d23;
	color: #4a5663;
}
#onAirButton {
	font-weight: 800;
	letter-spacing: 1px;
	min-height: 20px;
	padding: 6px 12px;
}
#onAirButton:enabled {
	background-color: #d92d20;
	color: #ffffff;
}
#onAirButton:enabled:hover {
	background-color: #e5382b;
}
#favoriteButton {
	font-size: 15px;
	padding: 3px 10px;
}
#favoriteButton[favorite="true"] {
	color: #e0a800;
}
#addSourceButton {
	background-color: transparent;
	color: #2e9bff;
	padding: 2px 4px;
	font-weight: 600;
}
#addSourceButton:hover {
	color: #6cb8ff;
	background-color: transparent;
}
QFrame[segmentedControl="true"] {
	background-color: #212a31;
	border: 1px solid #212a31;
	border-radius: 15px;
}
QFrame[segmentedControl="true"] QPushButton {
	background-color: transparent;
	color: #8795a3;
	border: 1px solid transparent;
	border-radius: 12px;
	padding: 4px 9px;
}
QFrame[segmentedControl="true"] QPushButton:hover:!checked {
	color: #d5dde5;
}
QFrame[segmentedControl="true"] QPushButton:checked {
	background-color: #2e9bff;
	border: 1px solid #2e9bff;
	color: #ffffff;
}
QFrame[segmentedControl="true"] QPushButton:disabled {
	color: #4a5663;
}
#stageCard {
	background-color: #151c23;
	border: 1px solid #293036;
	border-radius: 10px;
}
#stageTitle {
	background-color: #11171c;
	color: #8795a3;
	border-radius: 6px;
	padding: 2px 8px;
	font-size: 10px;
	font-weight: 700;
	letter-spacing: 1.5px;
}
#stageTitle[live="true"] {
	background-color: #3a1512;
	color: #ff6b5e;
}
#stagePreviewScreen, #stageProgramScreen {
	background-color: #06090c;
	border: 1px solid #1c242b;
	border-radius: 8px;
}
#stagePreviewScreen[live="true"], #stageProgramScreen[live="true"] {
	border: 2px solid #d92d20;
}
#stageContent {
	color: #f2f4f7;
	font-size: 12px;
}
#stageCaption {
	color: #5e6b78;
	font-size: 11px;
}
#pageLabel {
	color: #8795a3;
	font-family: "Cascadia Mono", Consolas, "Courier New", monospace;
	font-size: 11px;
}
QTabWidget::pane {
	border: none;
}
QTabBar::tab {
	background-color: transparent;
	color: #8795a3;
	padding: 5px 9px;
	margin-right: 2px;
	border-radius: 11px;
	font-weight: 600;
}
QTabBar::tab:hover:!selected {
	color: #d5dde5;
}
QTabBar::tab:selected {
	background-color: #212a31;
	color: #ffffff;
}
QListWidget {
	background-color: #0b1015;
	border: 1px solid #293036;
	border-radius: 8px;
	color: #d5dde5;
	padding: 3px;
	outline: none;
}
QListWidget::item {
	padding: 5px 8px;
	border-radius: 5px;
}
QListWidget::item:hover {
	background-color: #1a222a;
}
QListWidget::item:selected {
	background-color: #12304f;
	color: #ffffff;
}
QScrollBar:vertical {
	background: transparent;
	width: 8px;
	margin: 0;
}
QScrollBar::handle:vertical {
	background: #2a343c;
	border-radius: 4px;
	min-height: 24px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
	height: 0;
}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
	background: transparent;
}
QScrollArea, QScrollArea > QWidget > QWidget {
	background: transparent;
	border: none;
}
QToolTip {
	background-color: #151c23;
	color: #d5dde5;
	border: 1px solid #293036;
	padding: 4px 6px;
}
#pickerPanel QPushButton {
	background-color: transparent;
	color: #8795a3;
	border: none;
	border-bottom: 2px solid transparent;
	border-radius: 0;
	padding: 3px 8px;
}
#pickerPanel QPushButton:hover:!checked {
	color: #d5dde5;
}
#pickerPanel QPushButton:checked {
	color: #ffffff;
	border-bottom: 2px solid #2e9bff;
}
#pickerPanel QPushButton:disabled {
	color: #3a4550;
}
#pickerPanel QToolButton {
	background-color: #1a222a;
	color: #c9d3dd;
	border: 1px solid #293036;
	border-radius: 5px;
	font-size: 11px;
}
#pickerPanel QToolButton:hover {
	border-color: #2e9bff;
}
#pickerPanel QToolButton:disabled {
	color: #3a4550;
}
#pickerPanel QToolButton[preview="true"] {
	background-color: #12304f;
	border-color: #2e9bff;
	color: #ffffff;
}
#pickerPanel QToolButton[live="true"] {
	background-color: #d92d20;
	border-color: #f04438;
	color: #ffffff;
}
#footer {
	background-color: #080b0e;
	border-radius: 8px;
}
#dockStatus {
	color: #8795a3;
	font-size: 11px;
}
#dockStatus[error="true"] {
	color: #ff6b5e;
}
#statusDot {
	background-color: #33c77b;
	border-radius: 4px;
}
#statusDot[error="true"] {
	background-color: #f04438;
}
)";

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

QString fromStd(const std::string &s)
{
	return QString::fromStdString(s);
}

/** The line shown under the search bar for what is typed. @p isError is true for the red ones. */
QString describeQuery(const search::QueryResult &r, const bible::BibleModule &bible, bool &isError)
{
	using search::QueryStatus;
	isError = !r.ok() && !r.inProgress();
	const QString book = fromStd(std::string(search::frenchBookName(r.book)));

	switch (r.status) {
	case QueryStatus::Ok: {
		const size_t count = search::passageVerses(r.passage, bible).size();
		return text("Status.Ready").arg(fromStd(search::formatPassage(r.passage, bible))).arg(count);
	}
	case QueryStatus::Empty:
		return text("Status.Type");
	case QueryStatus::NeedChapter:
		return text("Status.NeedChapter").arg(book);
	case QueryStatus::Incomplete:
		return text("Status.Incomplete").arg(book);
	case QueryStatus::Syntax:
		return text("Error.Syntax");
	case QueryStatus::Unsupported:
		return text("Error.Unsupported");
	case QueryStatus::UnknownBook:
		return text("Error.UnknownBook");
	case QueryStatus::AmbiguousBook: {
		QStringList names;
		for (int candidate : r.candidates)
			names << fromStd(std::string(search::frenchBookName(candidate)));
		return text("Error.AmbiguousBook").arg(names.join(QStringLiteral(", ")));
	}
	case QueryStatus::BookNotInModule:
		return text("Error.BookNotInModule").arg(book);
	case QueryStatus::ChapterOutOfRange:
		return text("Error.ChapterOutOfRange").arg(book).arg(r.limit);
	case QueryStatus::VerseOutOfRange:
		return text("Error.VerseOutOfRange").arg(book).arg(r.limit);
	case QueryStatus::ReversedRange:
		return text("Error.ReversedRange");
	}
	return {};
}

/** The passage stored in a list item (see refreshLibrary). */
search::Passage passageOf(const QListWidgetItem *item)
{
	const QVariantList v = item->data(Qt::UserRole).toList();
	if (v.size() != 5)
		return {};
	return {v[0].toInt(), v[1].toInt(), v[2].toInt(), v[3].toInt(), v[4].toInt()};
}

/** A slide as Qt rich text. Everything that comes from the Bible is escaped. */
QString slideHtml(const stage::Slide &slide)
{
	QString html = QStringLiteral("<p><b>%1</b></p><p>").arg(fromStd(slide.reference).toHtmlEscaped());
	const bool numbered = slide.verses.size() > 1;
	for (const stage::SlideVerse &v : slide.verses) {
		if (numbered)
			html += QStringLiteral("<sup>%1</sup>&nbsp;").arg(v.verse);
		html += fromStd(v.text).toHtmlEscaped() + QLatin1Char(' ');
	}
	return html + QStringLiteral("</p>");
}

} // namespace

void VyraDock::setStatus(const QString &text, bool isError)
{
	statusLabel_->setText(text);
	statusLabel_->setProperty("error", isError);
	// A changed dynamic property is not picked up by the style sheet until the widget is re-polished.
	statusLabel_->style()->unpolish(statusLabel_);
	statusLabel_->style()->polish(statusLabel_);
	statusDot_->setProperty("error", isError);
	statusDot_->style()->unpolish(statusDot_);
	statusDot_->style()->polish(statusDot_);
}

VyraDock::VyraDock(const bible::BibleModule *bible, QWidget *parent) : QWidget(parent), bible_(bible)
{
	setObjectName(QStringLiteral("vyraDock"));
	setStyleSheet(QString::fromUtf8(kStyleSheet));
	if (bible_)
		stage_ = std::make_unique<stage::StageController>(*bible_);

	auto *root = new QVBoxLayout(this);
	root->setContentsMargins(8, 8, 8, 8);
	root->setSpacing(8);

	// Header: the VYRA mark and the name, as in VYRA Studio
	auto *header = new QHBoxLayout();
	header->setSpacing(8);
	header->addWidget(new BrandMark(30, this));
	auto *brandText = new QVBoxLayout();
	brandText->setSpacing(0);
	auto *brandTitle = new QLabel(text("Brand.Title"), this);
	brandTitle->setObjectName(QStringLiteral("brandTitle"));
	auto *brandSub = new QLabel(text("Brand.Sub"), this);
	brandSub->setObjectName(QStringLiteral("brandSub"));
	brandText->addWidget(brandTitle);
	brandText->addWidget(brandSub);
	header->addLayout(brandText);
	header->addStretch(1);
	root->addLayout(header);

	searchEdit_ = new QLineEdit(this);
	searchEdit_->setPlaceholderText(text("Dock.Search.Placeholder"));
	searchEdit_->setClearButtonEnabled(true);
	searchEdit_->setToolTip(text("Tooltip.Search"));
	searchEdit_->installEventFilter(this);
	favoriteButton_ = new QPushButton(text("Button.Favorite"), this);
	favoriteButton_->setObjectName(QStringLiteral("favoriteButton"));
	favoriteButton_->setToolTip(text("Tooltip.Favorite"));
	favoriteButton_->setFocusPolicy(Qt::NoFocus);
	auto *searchRow = new QHBoxLayout();
	searchRow->setSpacing(6);
	searchRow->addWidget(searchEdit_, 1);
	searchRow->addWidget(favoriteButton_);
	root->addLayout(searchRow);

	// Control-room monitors
	auto *stages = new QHBoxLayout();
	stages->setSpacing(8);
	preview_ = new StagePanel(text("Stage.Preview"), text("Stage.Preview.Empty"),
				  QStringLiteral("stagePreviewScreen"), this);
	program_ = new StagePanel(text("Stage.Program"), text("Stage.Program.Empty"),
				  QStringLiteral("stageProgramScreen"), this);
	stages->addWidget(preview_, 1);
	stages->addWidget(program_, 1);
	root->addLayout(stages);

	auto *buttons = new QHBoxLayout();
	buttons->setSpacing(6);
	previousButton_ = new QPushButton(text("Button.Previous"), this);
	nextButton_ = new QPushButton(text("Button.Next"), this);
	hideButton_ = new QPushButton(text("Button.Hide"), this);
	onAirButton_ = new QPushButton(text("Button.OnAir"), this);
	onAirButton_->setObjectName(QStringLiteral("onAirButton"));
	previousButton_->setToolTip(text("Tooltip.Previous"));
	nextButton_->setToolTip(text("Tooltip.Next"));
	hideButton_->setToolTip(text("Tooltip.Hide"));
	onAirButton_->setToolTip(text("Tooltip.OnAir"));

	// The keyboard flow (type, Enter, Ctrl+Enter) must never be interrupted by a click moving the focus.
	for (QPushButton *button : {previousButton_, nextButton_, hideButton_, onAirButton_})
		button->setFocusPolicy(Qt::NoFocus);

	buttons->addWidget(previousButton_);
	buttons->addWidget(nextButton_);
	buttons->addWidget(hideButton_);
	buttons->addStretch(1);
	buttons->addWidget(onAirButton_);
	root->addLayout(buttons);

	// Display: theme and pages of the PROGRAM
	auto *display = new QHBoxLayout();
	display->setSpacing(6);
	themeBox_ = new SegmentedControl({text("Theme.Lower"), text("Theme.Full"), text("Theme.Minimal")}, this);
	themeBox_->setObjectName(QStringLiteral("themeBox"));
	themeBox_->setToolTip(text("Tooltip.Theme"));
	pagePreviousButton_ = new QPushButton(text("Button.PagePrevious"), this);
	pageNextButton_ = new QPushButton(text("Button.PageNext"), this);
	pagePreviousButton_->setToolTip(text("Tooltip.PagePrevious"));
	pageNextButton_->setToolTip(text("Tooltip.PageNext"));
	pagePreviousButton_->setFocusPolicy(Qt::NoFocus);
	pageNextButton_->setFocusPolicy(Qt::NoFocus);
	pagePreviousButton_->setObjectName(QStringLiteral("pagePreviousButton"));
	pageNextButton_->setObjectName(QStringLiteral("pageNextButton"));
	pageLabel_ = new QLabel(this);
	pageLabel_->setObjectName(QStringLiteral("pageLabel"));
	display->addWidget(themeBox_);
	display->addStretch(1);
	root->addLayout(display);

	// Page bar: only there while the passage on the air has several pages (keeps the dock narrow otherwise).
	pageBar_ = new QWidget(this);
	auto *pageLayout = new QHBoxLayout(pageBar_);
	pageLayout->setContentsMargins(0, 0, 0, 0);
	pageLayout->setSpacing(6);
	pageLayout->addWidget(pageLabel_);
	pageLayout->addStretch(1);
	pageLayout->addWidget(pagePreviousButton_);
	pageLayout->addWidget(pageNextButton_);
	pageBar_->hide();
	root->addWidget(pageBar_);

	picker_ = new PickerPanel(bible_, this);
	picker_->setMinimumHeight(110);

	// Quick access: the picker, what already went on the air, and the passages kept as favorites.
	tabs_ = new QTabWidget(this);
	tabs_->setObjectName(QStringLiteral("quickTabs"));
	tabs_->tabBar()->setFocusPolicy(Qt::NoFocus);
	tabs_->setFocusPolicy(Qt::NoFocus);
	auto makeList = [this](const char *name) {
		auto *list = new QListWidget(this);
		list->setObjectName(QString::fromUtf8(name));
		list->setFocusPolicy(Qt::NoFocus); // clicking must not take the keyboard away from the search bar
		return list;
	};
	historyList_ = makeList("historyList");
	favoritesList_ = makeList("favoritesList");
	clearHistoryButton_ = new QPushButton(text("Button.ClearHistory"), this);
	clearHistoryButton_->setFocusPolicy(Qt::NoFocus);
	auto *historyPage = new QWidget(this);
	auto *historyLayout = new QVBoxLayout(historyPage);
	historyLayout->setContentsMargins(0, 4, 0, 0);
	historyLayout->addWidget(historyList_, 1);
	historyLayout->addWidget(clearHistoryButton_, 0, Qt::AlignRight);
	tabs_->addTab(picker_, text("Tab.Picker"));
	tabs_->addTab(historyPage, text("Tab.History"));
	tabs_->addTab(favoritesList_, text("Tab.Favorites"));
	tabs_->setMinimumHeight(130);
	root->addWidget(tabs_, 1);

	addSourceButton_ = new QPushButton(text("Button.AddSource"), this);
	addSourceButton_->setObjectName(QStringLiteral("addSourceButton"));
	addSourceButton_->setFlat(true);
	addSourceButton_->setFocusPolicy(Qt::NoFocus);
	addSourceButton_->setToolTip(text("Tooltip.AddSource"));
	addSourceButton_->hide(); // shown once a handler is set
	root->addWidget(addSourceButton_, 0, Qt::AlignLeft);
	connect(addSourceButton_, &QPushButton::clicked, this, [this] {
		if (addSourceHandler_)
			setStatus(addSourceHandler_(), false);
	});

	// Footer: a status dot (green = fine, red = a problem) and the status text, as in VYRA Studio
	auto *footer = new QFrame(this);
	footer->setObjectName(QStringLiteral("footer"));
	auto *footerLayout = new QHBoxLayout(footer);
	footerLayout->setContentsMargins(10, 7, 10, 7);
	footerLayout->setSpacing(8);
	statusDot_ = new QLabel(footer);
	statusDot_->setObjectName(QStringLiteral("statusDot"));
	statusDot_->setFixedSize(8, 8);
	statusLabel_ = new QLabel(footer);
	statusLabel_->setObjectName(QStringLiteral("dockStatus"));
	statusLabel_->setWordWrap(true);
	footerLayout->addWidget(statusDot_, 0, Qt::AlignTop | Qt::AlignHCenter);
	footerLayout->addWidget(statusLabel_, 1);
	root->addWidget(footer);

	connect(picker_, &PickerPanel::bookChosen, this, [this](int book) {
		if (!stage_)
			return;
		const std::string name(search::frenchBookName(book));
		picker_->showChapters(book);
		setFieldText(fromStd(name) + QLatin1Char(' '), false, true);
	});
	connect(picker_, &PickerPanel::chapterChosen, this, [this](int book, int chapter) {
		if (!stage_)
			return;
		picker_->showVerses(book, chapter);
		setFieldText(fromStd(std::string(search::frenchBookName(book))) + QLatin1Char(' ') +
				     QString::number(chapter) + QLatin1Char(':'),
			     false, true);
	});
	connect(picker_, &PickerPanel::verseChosen, this,
		[this](int book, int chapter, int verse, bool extend, bool onAir) {
			pickVerse(book, chapter, verse, extend, onAir);
		});
	connect(searchEdit_, &QLineEdit::textChanged, this, [this] { onQueryChanged(); });
	connect(previousButton_, &QPushButton::clicked, this, [this] { navigate(false); });
	connect(nextButton_, &QPushButton::clicked, this, [this] { navigate(true); });
	connect(hideButton_, &QPushButton::clicked, this, [this] { hideProgram(); });
	connect(onAirButton_, &QPushButton::clicked, this, [this] { airFromField(); });
	connect(favoriteButton_, &QPushButton::clicked, this, [this] { toggleFavorite(); });
	connect(clearHistoryButton_, &QPushButton::clicked, this, [this] {
		library_.clearHistory();
		saveLibrary();
		refreshLibrary();
	});
	for (QListWidget *list : {historyList_, favoritesList_}) {
		// Queued on purpose: using an item rebuilds the lists, and a list must never be emptied from inside
		// its own click signal (the view would still touch the item it just lost).
		connect(list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
			const search::Passage passage = passageOf(item);
			QMetaObject::invokeMethod(
				this, [this, passage] { useLibraryItem(passage, false); }, Qt::QueuedConnection);
		});
		connect(list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
			const search::Passage passage = passageOf(item);
			QMetaObject::invokeMethod(
				this, [this, passage] { useLibraryItem(passage, true); }, Qt::QueuedConnection);
		});
	}
	connect(pagePreviousButton_, &QPushButton::clicked, this, [this] { changePage(false); });
	connect(pageNextButton_, &QPushButton::clicked, this, [this] { changePage(true); });
	connect(themeBox_, &SegmentedControl::activated, this, [this](int i) { changeTheme(i); });

	searchEdit_->setEnabled(stage_ != nullptr);
	refresh();
	refreshLibrary();
	obs_log(LOG_DEBUG, "dock widgets created");
}

VyraDock::~VyraDock() = default;

void VyraDock::setAddSourceHandler(AddSourceHandler handler)
{
	addSourceHandler_ = std::move(handler);
	addSourceButton_->setVisible(static_cast<bool>(addSourceHandler_));
}

void VyraDock::setProgramListener(ProgramListener listener)
{
	programListener_ = std::move(listener);
	notifyProgram();
}

void VyraDock::notifyProgram()
{
	if (programListener_ && stage_)
		programListener_(*stage_);
}

void VyraDock::onQueryChanged()
{
	if (syncingField_ || !bible_)
		return;
	query_ = search::resolveQuery(searchEdit_->text().toUtf8().toStdString(), *bible_, context());
	bool isError = false;
	const QString message = describeQuery(query_, *bible_, isError);
	setStatus(message, isError);
}

void VyraDock::previewFromField()
{
	if (!stage_ || !query_.ok())
		return;
	if (stage_->showInPreview(query_.passage)) {
		setFieldToPreview();
		setStatus(text("Status.Previewed").arg(fromStd(search::formatPassage(*stage_->preview(), *bible_))),
			  false);
		refresh(true);
	}
}

void VyraDock::airFromField()
{
	if (!stage_)
		return;
	if (!searchEdit_->text().trimmed().isEmpty()) {
		// Something is typed: it must be valid, otherwise nothing goes on the air (the error is on screen).
		if (!query_.ok())
			return;
		stage_->showInPreview(query_.passage);
	}
	if (stage_->takeOnAir()) {
		setStatus(text("Status.OnAir").arg(fromStd(search::formatPassage(*stage_->program(), *bible_))), false);
		setFieldToPreview();
		refresh(true);
		recordOnAir();
		notifyProgram();
	}
}

void VyraDock::navigate(bool forward)
{
	if (!stage_)
		return;
	if (forward ? stage_->previewNext() : stage_->previewPrevious()) {
		setFieldToPreview();
		setStatus(text("Status.Previewed").arg(fromStd(search::formatPassage(*stage_->preview(), *bible_))),
			  false);
		refresh(true);
	}
}

void VyraDock::setLibraryFile(const std::filesystem::path &path)
{
	libraryPath_ = path;
	if (!bible_)
		return;
	const library::LoadReport report = library_.loadFromFile(path, *bible_);
	refreshLibrary();
	refresh();
	if (report.skippedLines > 0)
		setStatus(text("Status.LibrarySkipped").arg(static_cast<qulonglong>(report.skippedLines)), true);
}

void VyraDock::saveLibrary()
{
	if (libraryPath_.empty())
		return;
	if (!library_.saveToFile(libraryPath_))
		setStatus(text("Error.LibrarySave"), true);
}

void VyraDock::recordOnAir()
{
	if (!stage_ || !stage_->program())
		return;
	library_.addToHistory(*stage_->program());
	saveLibrary();
	refreshLibrary();
}

void VyraDock::toggleFavorite()
{
	if (!stage_ || !stage_->preview())
		return;
	const search::Passage passage = *stage_->preview();
	const bool wasFavorite = library_.isFavorite(passage);
	const bool nowFavorite = library_.toggleFavorite(passage);
	if (!wasFavorite && !nowFavorite) {
		setStatus(
			text("Error.FavoritesFull").arg(static_cast<qulonglong>(library::PassageLibrary::kMaxFavorites)),
			true);
		return;
	}
	saveLibrary();
	refreshLibrary();
	refresh();
}

void VyraDock::refreshLibrary()
{
	if (!bible_)
		return;
	auto fill = [this](QListWidget *list, const std::vector<search::Passage> &passages) {
		list->clear();
		for (const search::Passage &p : passages) {
			auto *item = new QListWidgetItem(fromStd(search::formatPassage(p, *bible_)));
			item->setData(Qt::UserRole,
				      QVariantList{p.book, p.startChapter, p.startVerse, p.endChapter, p.endVerse});
			list->addItem(item);
		}
	};
	fill(historyList_, library_.history());
	fill(favoritesList_, library_.favorites());
	clearHistoryButton_->setEnabled(!library_.history().empty());
}

/** A click on history or favorites: the passage goes to the PREVIEW; a double-click puts it on the air. */
void VyraDock::useLibraryItem(const search::Passage &passage, bool onAir)
{
	if (!stage_ || !library::isValidPassage(passage, *bible_) || !stage_->showInPreview(passage))
		return;
	setFieldToPreview();
	if (onAir && stage_->takeOnAir()) {
		setStatus(text("Status.OnAir").arg(fromStd(search::formatPassage(*stage_->program(), *bible_))), false);
		refresh(true);
		recordOnAir();
		notifyProgram();
		return;
	}
	setStatus(text("Status.Previewed").arg(fromStd(search::formatPassage(*stage_->preview(), *bible_))), false);
	refresh(true);
}

void VyraDock::perform(stage::OperatorAction action)
{
	using stage::OperatorAction;
	switch (action) {
	case OperatorAction::OnAir:
		airFromField();
		break;
	case OperatorAction::Hide:
		hideProgram();
		break;
	case OperatorAction::PreviewNext:
		navigate(true);
		break;
	case OperatorAction::PreviewPrevious:
		navigate(false);
		break;
	case OperatorAction::PageNext:
		changePage(true);
		break;
	case OperatorAction::PagePrevious:
		changePage(false);
		break;
	}
}

void VyraDock::changePage(bool forward)
{
	if (!stage_ || !(forward ? stage_->nextPage() : stage_->previousPage()))
		return;
	refresh();
	notifyProgram();
}

void VyraDock::changeTheme(int index)
{
	if (!stage_)
		return;
	static const stage::Theme themes[] = {stage::Theme::LowerThird, stage::Theme::FullScreen,
					      stage::Theme::Minimal};
	if (index < 0 || index > 2)
		return;
	stage_->setTheme(themes[index]);
	refresh();
	notifyProgram();
}

void VyraDock::hideProgram()
{
	if (stage_ && stage_->hideProgram()) {
		setStatus(text("Status.Hidden").arg(fromStd(search::formatPassage(*stage_->program(), *bible_))),
			  false);
		refresh();
		notifyProgram();
	}
}

const search::Passage *VyraDock::context() const
{
	return stage_ && stage_->preview() ? &*stage_->preview() : nullptr;
}

/** Sets the search bar without reacting to it as if the operator had typed. */
void VyraDock::setFieldText(const QString &value, bool selectAll, bool report)
{
	syncingField_ = true;
	searchEdit_->setText(value);
	if (selectAll)
		searchEdit_->selectAll();
	syncingField_ = false;
	if (bible_)
		query_ = search::resolveQuery(value.toUtf8().toStdString(), *bible_, context());
	if (report && bible_) {
		bool isError = false;
		const QString message = describeQuery(query_, *bible_, isError);
		setStatus(message, isError);
	}
}

/**
 * Keeps the search bar equal to the preview, so that Ctrl+Enter always airs what the operator sees.
 * The text is selected: the next number typed replaces it ("Jean 3:17" selected, "18" typed, Jean 3:18).
 */
void VyraDock::setFieldToPreview()
{
	if (!stage_ || !stage_->preview())
		return;
	setFieldText(fromStd(search::formatPassage(*stage_->preview(), *bible_)), true);
}

void VyraDock::pickVerse(int book, int chapter, int verse, bool extend, bool onAir)
{
	if (!stage_)
		return;
	search::Passage passage{book, chapter, verse, chapter, verse};
	if (extend && anchor_ && anchor_->book == book && anchor_->chapter == chapter) {
		passage.startVerse = std::min(anchor_->verse, verse);
		passage.endVerse = std::max(anchor_->verse, verse);
	} else if (!extend) {
		anchor_ = bible::Reference{book, chapter, verse};
	}
	if (!stage_->showInPreview(passage))
		return;
	setFieldToPreview();
	if (onAir && stage_->takeOnAir()) {
		setStatus(text("Status.OnAir").arg(fromStd(search::formatPassage(*stage_->program(), *bible_))), false);
		refresh(true);
		recordOnAir();
		notifyProgram();
		return;
	}
	setStatus(text("Status.Previewed").arg(fromStd(search::formatPassage(*stage_->preview(), *bible_))), false);
	refresh(true);
}

void VyraDock::refresh(bool follow)
{
	const bool have = stage_ != nullptr;

	if (have && stage_->preview())
		preview_->setContent(slideHtml(stage_->slide(*stage_->preview())));
	else
		preview_->setCaption(text("Stage.Preview.Empty"));

	if (have && stage_->programLive()) {
		program_->setContent(slideHtml(stage_->slide(*stage_->program())));
		program_->setLive(true);
	} else if (have && stage_->program()) {
		program_->setCaption(
			text("Stage.Program.Hidden").arg(fromStd(search::formatPassage(*stage_->program(), *bible_))));
		program_->setLive(false);
	} else {
		program_->setCaption(text("Stage.Program.Empty"));
		program_->setLive(false);
	}

	if (have) {
		std::optional<search::Passage> live;
		if (stage_->programLive())
			live = stage_->program();
		picker_->setPosition(stage_->preview(), live, follow);
	}

	previousButton_->setEnabled(have && stage_->hasPrevious());
	nextButton_->setEnabled(have && stage_->hasNext());
	hideButton_->setEnabled(have && stage_->programLive());
	onAirButton_->setEnabled(have && stage_->preview().has_value());
	themeBox_->setEnabled(have);
	if (have)
		themeBox_->setCurrentIndex(static_cast<int>(stage_->theme()));
	const bool previewed = have && stage_->preview().has_value();
	const bool fav = previewed && library_.isFavorite(*stage_->preview());
	favoriteButton_->setEnabled(previewed);
	favoriteButton_->setText(text(fav ? "Button.Favorite.On" : "Button.Favorite"));
	favoriteButton_->setProperty("favorite", fav);

	const std::size_t pages = have ? stage_->programPages().size() : 0;
	const std::size_t page = have ? stage_->programPage() : 0;
	const bool live = have && stage_->programLive();
	pageLabel_->setText(live && pages > 1 ? text("Status.Page").arg(page + 1).arg(pages) : QString());
	pagePreviousButton_->setEnabled(live && page > 0);
	pageNextButton_->setEnabled(live && page + 1 < pages);
	pageBar_->setVisible(pages > 1);
}

bool VyraDock::eventFilter(QObject *watched, QEvent *event)
{
	if (watched != searchEdit_ || event->type() != QEvent::KeyPress || !stage_)
		return QWidget::eventFilter(watched, event);

	const auto *key = static_cast<QKeyEvent *>(event);
	const bool ctrl = key->modifiers() & Qt::ControlModifier;
	const bool plain = (key->modifiers() & ~Qt::KeypadModifier) == Qt::NoModifier;

	switch (key->key()) {
	case Qt::Key_Return:
	case Qt::Key_Enter:
		if (ctrl)
			airFromField();
		else
			previewFromField();
		return true;
	case Qt::Key_Up:
	case Qt::Key_Down:
		if (plain && stage_->preview()) {
			navigate(key->key() == Qt::Key_Down);
			return true;
		}
		break;
	case Qt::Key_D:
		if (ctrl && stage_->preview()) {
			toggleFavorite();
			return true;
		}
		break;
	case Qt::Key_PageUp:
	case Qt::Key_PageDown:
		if (plain && stage_->programLive()) {
			changePage(key->key() == Qt::Key_PageDown);
			return true;
		}
		break;
	case Qt::Key_Escape:
		if (stage_->programLive()) {
			hideProgram();
			return true;
		}
		break;
	default:
		break;
	}
	return QWidget::eventFilter(watched, event);
}

} // namespace vyra::ui
