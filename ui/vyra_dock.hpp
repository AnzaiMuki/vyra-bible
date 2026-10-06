/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QWidget>

#include <functional>
#include <memory>

#include "src/search/passage_query.hpp"

namespace vyra::bible {
class BibleModule;
}
namespace vyra::stage {
class StageController;
}

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
 * Milestone 04: the search bar is live. It only translates what the operator does into calls to
 * StageController (which holds all the rules) and displays the result.
 *
 * Keys, while the search bar has the focus:
 *   Enter        passage typed -> PREVIEW
 *   Ctrl+Enter   passage typed (or the one in PREVIEW if the bar is empty) -> PROGRAM, ON AIR
 *   Up / Down    previous / next verse, in PREVIEW only
 *   Esc          takes the PROGRAM off the air (the passage stays ready)
 */
class VyraDock : public QWidget {
	Q_OBJECT

public:
	/**
	 * @param bible  The Bible to read from; must outlive the dock. May be null (failed to load):
	 *               every control then stays disabled.
	 */
	explicit VyraDock(const bible::BibleModule *bible, QWidget *parent = nullptr);
	~VyraDock() override;

	/**
	 * Called each time what is on the PROGRAM changes (ON AIR, Hide), with the controller holding the new
	 * state, and once at once when the listener is set. The dock knows nothing about who listens.
	 */
	using ProgramListener = std::function<void(const stage::StageController &)>;
	void setProgramListener(ProgramListener listener);

	/** Sets the status line at the bottom of the dock. @p isError shows it in red. */
	void setStatus(const QString &text, bool isError);

protected:
	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	void onQueryChanged();
	void previewFromField();
	void airFromField();
	void navigate(bool forward);
	void hideProgram();
	void refresh();
	void setFieldToPreview();
	void notifyProgram();

	const bible::BibleModule *bible_ = nullptr;
	std::unique_ptr<stage::StageController> stage_;
	search::QueryResult query_;
	bool syncingField_ = false;
	ProgramListener programListener_;

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
