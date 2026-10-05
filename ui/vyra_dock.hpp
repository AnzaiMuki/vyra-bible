/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;

namespace vyra::ui {

class StagePanel;

/**
 * The operator dock, laid out as a control room:
 *
 *   [ search bar                         ]
 *   [ PREVIEW        ] [ PROGRAM         ]
 *   [ Previous | Next | Hide |  ON AIR   ]
 *   [ status line                        ]
 *
 * Milestone 01: layout only. Every control is disabled on purpose, so that an
 * operator never believes a button works when it does not yet.
 */
class VyraDock : public QWidget {
	Q_OBJECT

public:
	explicit VyraDock(QWidget *parent = nullptr);

	/** Sets the status line at the bottom of the dock. @p isError shows it in red. */
	void setStatus(const QString &text, bool isError);

private:
	QLineEdit *searchEdit_ = nullptr;
	StagePanel *preview_ = nullptr;
	StagePanel *program_ = nullptr;
	QPushButton *previousButton_ = nullptr;
	QPushButton *nextButton_ = nullptr;
	QPushButton *hideButton_ = nullptr;
	QPushButton *onAirButton_ = nullptr;
	QLabel *statusLabel_ = nullptr;
};

} // namespace vyra::ui
