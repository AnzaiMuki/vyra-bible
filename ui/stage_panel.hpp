/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QFrame>
#include <QWidget>

class QLabel;

namespace vyra::ui {

/**
 * A 16:9 black screen. It keeps its aspect ratio whatever the width of the dock.
 * Milestone 01: shows a caption only. The real rendering arrives with Milestone 05.
 */
class AspectFrame : public QFrame {
	Q_OBJECT

public:
	explicit AspectFrame(const QString &caption, QWidget *parent = nullptr);

	int heightForWidth(int width) const override;

protected:
	void resizeEvent(QResizeEvent *event) override;

private:
	// No layout on purpose: a child layout would hide our height-for-width from the parent layout.
	QLabel *captionLabel_ = nullptr;
};

/**
 * One of the two monitors of the control room (PREVIEW or PROGRAM):
 * a title above a 16:9 screen with a caption.
 */
class StagePanel : public QWidget {
	Q_OBJECT

public:
	/**
	 * @param title        Heading shown above the screen (already translated).
	 * @param caption      Text shown inside the screen while it is empty (already translated).
	 * @param screenName   Qt object name of the screen, used by the style sheet.
	 */
	StagePanel(const QString &title, const QString &caption, const QString &screenName, QWidget *parent = nullptr);

private:
	QLabel *titleLabel_ = nullptr;
	AspectFrame *screen_ = nullptr;
};

} // namespace vyra::ui
