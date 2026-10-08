/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ui/picker_panel.hpp"

#include <algorithm>

#include <obs-module.h>

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include "src/bible/bible_module.hpp"
#include "src/search/book_catalog.hpp"

namespace vyra::ui {

namespace {

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

/** A cell of a grid. A double-click is reported on its own (the first click has already been sent). */
class Cell : public QToolButton {
	Q_OBJECT

public:
	explicit Cell(const QString &label, QWidget *parent = nullptr) : QToolButton(parent)
	{
		setText(label);
		setProperty("pickCell", true); // selector of the style sheet
		setFocusPolicy(Qt::NoFocus); // the keyboard stays in the search bar
		setAutoRaise(false); // the cell is fully drawn by the style sheet
	}

	/** Keys held when the button was pressed (what the click means depends on them). */
	Qt::KeyboardModifiers modifiers() const { return modifiers_; }

signals:
	void doubleClicked();

protected:
	void mousePressEvent(QMouseEvent *event) override
	{
		modifiers_ = event->modifiers();
		QToolButton::mousePressEvent(event);
	}
	void mouseDoubleClickEvent(QMouseEvent *event) override
	{
		if (event->button() == Qt::LeftButton)
			emit doubleClicked();
		QToolButton::mouseDoubleClickEvent(event);
	}

private:
	Qt::KeyboardModifiers modifiers_;
};

void setFlag(QWidget *w, const char *name, bool value)
{
	if (w->property(name).toBool() == value)
		return;
	w->setProperty(name, value);
	w->style()->unpolish(w);
	w->style()->polish(w);
}

constexpr int kGap = 3;

} // namespace

// ---------------------------------------------------------------------------------------------------

FlowGrid::FlowGrid(QWidget *parent) : QWidget(parent) {}

void FlowGrid::setCells(const std::vector<QToolButton *> &cells, const QSize &cellSize)
{
	// The old cells leave at once (never found again by their name) but are deleted later: the click that
	// is replacing them may still be running inside one of them.
	for (QToolButton *old : cells_) {
		old->hide();
		old->setParent(nullptr);
		old->deleteLater();
	}
	cells_ = cells;
	cellSize_ = cellSize;
	for (QToolButton *cell : cells_) {
		cell->setParent(this);
		cell->resize(cellSize_);
		cell->show();
	}
	relayout();
}

void FlowGrid::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	relayout();
}

void FlowGrid::relayout()
{
	const int columns = std::max(1, (width() + kGap) / (cellSize_.width() + kGap));
	int index = 0;
	for (QToolButton *cell : cells_) {
		const int row = index / columns;
		const int col = index % columns;
		cell->setGeometry(col * (cellSize_.width() + kGap), row * (cellSize_.height() + kGap), cellSize_.width(),
				  cellSize_.height());
		++index;
	}
	const int rows = (static_cast<int>(cells_.size()) + columns - 1) / columns;
	const int height = std::max(0, rows * (cellSize_.height() + kGap) - kGap);
	if (minimumHeight() != height)
		setMinimumHeight(height);
}

// ---------------------------------------------------------------------------------------------------

PickerPanel::PickerPanel(const bible::BibleModule *bible, QWidget *parent) : QWidget(parent), bible_(bible)
{
	setObjectName(QStringLiteral("pickerPanel"));
	auto *root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(4);

	auto *tabRow = new QHBoxLayout();
	tabRow->setSpacing(4);
	tabs_ = new QButtonGroup(this);
	tabs_->setExclusive(true);
	const char *const labels[] = {"Picker.Books", "Picker.Chapters", "Picker.Verses"};
	for (int i = 0; i < 3; ++i) {
		auto *tab = new QPushButton(text(labels[i]), this);
		tab->setObjectName(QStringLiteral("pickerTab%1").arg(i));
		tab->setCheckable(true);
		tab->setFlat(true);
		tab->setFocusPolicy(Qt::NoFocus);
		tabs_->addButton(tab, i);
		tabRow->addWidget(tab);
	}
	tabRow->addStretch(1);
	root->addLayout(tabRow);

	pages_ = new QStackedWidget(this);
	root->addWidget(pages_, 1);
	for (FlowGrid **grid : {&booksGrid_, &chaptersGrid_, &versesGrid_}) {
		auto *scroll = new QScrollArea(pages_);
		scroll->setWidgetResizable(true);
		scroll->setFrameShape(QFrame::NoFrame);
		scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		*grid = new FlowGrid(scroll);
		scroll->setWidget(*grid);
		pages_->addWidget(scroll);
	}
	connect(tabs_, &QButtonGroup::idClicked, this, [this](int id) { showPage(static_cast<Page>(id)); });

	rebuildBooks();
	showPage(Books);
}

void PickerPanel::showPage(Page page)
{
	pages_->setCurrentIndex(page);
	if (QAbstractButton *tab = tabs_->button(page))
		tab->setChecked(true);
	// Chapters and Verses mean nothing until a book / a chapter has been chosen.
	tabs_->button(Chapters)->setEnabled(chaptersBook_ != 0);
	tabs_->button(Verses)->setEnabled(versesChapter_ != 0);
}

void PickerPanel::rebuildBooks()
{
	std::vector<QToolButton *> cells;
	for (int book = 1; book <= 66; ++book) {
		auto *cell = new Cell(QString::fromUtf8(search::shortBookName(book).data(),
							static_cast<qsizetype>(search::shortBookName(book).size())));
		cell->setObjectName(QStringLiteral("book-%1").arg(book));
		cell->setToolTip(QString::fromUtf8(search::frenchBookName(book).data(),
						   static_cast<qsizetype>(search::frenchBookName(book).size())));
		cell->setEnabled(bible_ && bible_->chapterCount(book) > 0); // a book missing from this Bible
		connect(cell, &QToolButton::clicked, this, [this, book] {
			emit bookChosen(book);
		});
		cells.push_back(cell);
	}
	booksGrid_->setCells(cells, QSize(40, 26));
}

void PickerPanel::showChapters(int book)
{
	if (!bible_ || book < 1 || book > 66)
		return;
	chaptersBook_ = book;
	std::vector<QToolButton *> cells;
	for (int chapter = 1; chapter <= bible_->chapterCount(book); ++chapter) {
		auto *cell = new Cell(QString::number(chapter));
		cell->setObjectName(QStringLiteral("chapter-%1").arg(chapter));
		connect(cell, &QToolButton::clicked, this, [this, book, chapter] { emit chapterChosen(book, chapter); });
		cells.push_back(cell);
	}
	chaptersGrid_->setCells(cells, QSize(34, 26));
	showPage(Chapters);
	markCells();
}

void PickerPanel::showVerses(int book, int chapter)
{
	if (!bible_ || book < 1 || book > 66 || chapter < 1 || chapter > bible_->chapterCount(book))
		return;
	if (chaptersBook_ != book)
		showChapters(book); // keep the Chapters tab consistent with the verses shown
	versesBook_ = book;
	versesChapter_ = chapter;
	std::vector<QToolButton *> cells;
	for (int verse = 1; verse <= bible_->verseCount(book, chapter); ++verse) {
		auto *cell = new Cell(QString::number(verse));
		cell->setObjectName(QStringLiteral("verse-%1").arg(verse));
		connect(cell, &QToolButton::clicked, this, [this, cell, book, chapter, verse] {
			const Qt::KeyboardModifiers mods = cell->modifiers();
			emit verseChosen(book, chapter, verse, mods & Qt::ShiftModifier, mods & Qt::ControlModifier);
		});
		connect(cell, &Cell::doubleClicked, this, [this, book, chapter, verse] {
			emit verseChosen(book, chapter, verse, false, true);
		});
		cells.push_back(cell);
	}
	versesGrid_->setCells(cells, QSize(34, 26));
	showPage(Verses);
	markCells();
}

void PickerPanel::setPosition(const std::optional<search::Passage> &preview, const std::optional<search::Passage> &live,
			 bool follow)
{
	preview_ = preview;
	live_ = live;
	if (follow && preview_) {
		if (versesBook_ == preview_->book && versesChapter_ == preview_->endChapter)
			showPage(Verses); // already there: do not rebuild the cells under the mouse
		else
			showVerses(preview_->book, preview_->endChapter);
	}
	markCells();
}

// Colors the cells: blue = in the preview, red = on the air.
void PickerPanel::markCells()
{
	const auto covers = [](const std::optional<search::Passage> &p, int chapter, int verse) {
		if (!p)
			return false;
		if (chapter < p->startChapter || chapter > p->endChapter)
			return false;
		if (chapter == p->startChapter && verse < p->startVerse)
			return false;
		if (chapter == p->endChapter && verse > p->endVerse)
			return false;
		return true;
	};

	for (QToolButton *cell : booksGrid_->cells()) {
		const int book = cell->objectName().mid(5).toInt();
		setFlag(cell, "preview", preview_ && preview_->book == book);
		setFlag(cell, "live", live_ && live_->book == book);
	}
	for (QToolButton *cell : chaptersGrid_->cells()) {
		const int chapter = cell->objectName().mid(8).toInt();
		const bool inPreview = preview_ && preview_->book == chaptersBook_ && chapter >= preview_->startChapter &&
				       chapter <= preview_->endChapter;
		const bool onAir = live_ && live_->book == chaptersBook_ && chapter >= live_->startChapter &&
				   chapter <= live_->endChapter;
		setFlag(cell, "preview", inPreview);
		setFlag(cell, "live", onAir);
	}
	for (QToolButton *cell : versesGrid_->cells()) {
		const int verse = cell->objectName().mid(6).toInt();
		setFlag(cell, "preview", preview_ && preview_->book == versesBook_ && covers(preview_, versesChapter_, verse));
		setFlag(cell, "live", live_ && live_->book == versesBook_ && covers(live_, versesChapter_, verse));
	}
}

} // namespace vyra::ui

#include "picker_panel.moc"
