/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QFrame>
#include <QStringList>
#include <QWidget>

class QAbstractButton;
class QButtonGroup;

namespace vyra::ui {

/**
 * The VYRA Concept mark (rounded dark square, white ring, tan bar and grey dot), drawn with QPainter so it
 * stays sharp at any size and needs no image file in the plugin.
 */
class BrandMark : public QWidget {
	Q_OBJECT

public:
	explicit BrandMark(int size, QWidget *parent = nullptr);

protected:
	void paintEvent(QPaintEvent *event) override;
};

/**
 * Pill-shaped segmented control, as in VYRA Studio (Grille | Focus, Simple | Expert).
 * Exactly one segment is selected. It never takes the keyboard focus.
 */
class SegmentedControl : public QFrame {
	Q_OBJECT

public:
	explicit SegmentedControl(const QStringList &labels, QWidget *parent = nullptr);

	int count() const;
	int currentIndex() const;
	/** Selects a segment without sending activated() (used to show a state that did not come from a click). */
	void setCurrentIndex(int index);
	QAbstractButton *segment(int index) const;
	void setSegmentToolTip(int index, const QString &tip);

signals:
	/** The operator clicked a segment (also when it was already selected). */
	void activated(int index);

private:
	QButtonGroup *group_ = nullptr;
};

} // namespace vyra::ui
