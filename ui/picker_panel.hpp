/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QWidget>

#include <optional>
#include <vector>

#include "src/search/passage_query.hpp"

namespace vyra::bible {
class BibleModule;
}

class QAbstractButton;
class QButtonGroup;
class QScrollArea;
class QStackedWidget;
class QToolButton;

namespace vyra::ui {

/** Lays out its buttons in rows, as many per row as the width allows. Scrolls inside a QScrollArea. */
class FlowGrid : public QWidget {
	Q_OBJECT

public:
	explicit FlowGrid(QWidget *parent = nullptr);
	/** Replaces the cells (the old ones are deleted). The grid takes ownership. */
	void setCells(const std::vector<QToolButton *> &cells, const QSize &cellSize);
	const std::vector<QToolButton *> &cells() const { return cells_; }

protected:
	void resizeEvent(QResizeEvent *event) override;

private:
	void relayout();

	std::vector<QToolButton *> cells_;
	QSize cellSize_{32, 26};
};

/**
 * Pick a passage with the mouse: Books -> Chapters -> Verses, like a Bible app.
 * It decides nothing: it only says what was clicked (signals) and shows what the dock tells it.
 *
 * Clicks:  book -> chapters of that book;  chapter -> verses of that chapter;
 *          verse -> verseChosen (the dock previews it; Shift = range from the last verse, Ctrl or
 *          double-click = on the air as well).
 */
class PickerPanel : public QWidget {
	Q_OBJECT

public:
	explicit PickerPanel(const bible::BibleModule *bible, QWidget *parent = nullptr);

	/**
	 * Shows which verses are in the PREVIEW and which are on the air (red). When @p follow is true, the
	 * panel also moves to the verses of the preview's chapter (use it when the preview itself changed).
	 */
	void setPosition(const std::optional<search::Passage> &preview, const std::optional<search::Passage> &live,
			 bool follow);

	/** Shows the chapters of @p book / the verses of @p chapter (the dock calls these after a click). */
	void showChapters(int book);
	void showVerses(int book, int chapter);

signals:
	void bookChosen(int book);
	void chapterChosen(int book, int chapter);
	/** @p extend: Shift was held. @p onAir: Ctrl was held, or the verse was double-clicked. */
	void verseChosen(int book, int chapter, int verse, bool extend, bool onAir);

private:
	enum Page { Books = 0, Chapters = 1, Verses = 2 };
	void showPage(Page page);
	void rebuildBooks();
	void markCells();

	const bible::BibleModule *bible_;
	QStackedWidget *pages_ = nullptr;
	QButtonGroup *tabs_ = nullptr;
	FlowGrid *booksGrid_ = nullptr;
	FlowGrid *chaptersGrid_ = nullptr;
	FlowGrid *versesGrid_ = nullptr;
	int chaptersBook_ = 0; // book whose chapters are shown
	int versesBook_ = 0;   // chapter whose verses are shown
	int versesChapter_ = 0;
	std::optional<search::Passage> preview_;
	std::optional<search::Passage> live_;
};

} // namespace vyra::ui
