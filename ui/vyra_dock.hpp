/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QWidget>

#include <functional>
#include <memory>
#include <optional>

#include "src/bible/bible_module.hpp"
#include "src/search/passage_query.hpp"

namespace vyra::stage {
class StageController;
}

class QLineEdit;
class QPushButton;
class QLabel;
class QComboBox;

namespace vyra::ui {

class StagePanel;
class PickerPanel;

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
 * Quick selection (typing): after "Jn 3:16", typing just "17" and Enter gives Jean 3:17; "17-19", "4:1" too.
 * After every send the search bar is selected, so the next number replaces the previous one.
 * Quick selection (mouse): the Books / Chapters / Verses picker under the buttons.
 *   click a verse = PREVIEW, Shift+click = range from the last verse, Ctrl+click or double-click = ON AIR.
 *
 * Keys, while the search bar has the focus:
 *   Enter        passage typed -> PREVIEW
 *   Ctrl+Enter   passage typed (or the one in PREVIEW if the bar is empty) -> PROGRAM, ON AIR
 *   Up / Down    previous / next verse, in PREVIEW only
 *   PageUp/Down  previous / next PAGE of a long passage on the PROGRAM (the PREVIEW is not touched)
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

	/**
	 * Action behind the "Add the source to OBS" button; it returns the message to show. Without it the
	 * button is hidden (the dock knows nothing about OBS).
	 */
	using AddSourceHandler = std::function<QString()>;
	void setAddSourceHandler(AddSourceHandler handler);

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
	void refresh(bool follow = false);
	void pickVerse(int book, int chapter, int verse, bool extend, bool onAir);
	void setFieldText(const QString &text, bool selectAll, bool report = false);
	const search::Passage *context() const;
	void setFieldToPreview();
	void notifyProgram();
	void changePage(bool forward);
	void changeTheme(int index);

	const bible::BibleModule *bible_ = nullptr;
	std::unique_ptr<stage::StageController> stage_;
	search::QueryResult query_;
	bool syncingField_ = false;
	ProgramListener programListener_;
	QPushButton *addSourceButton_ = nullptr;
	AddSourceHandler addSourceHandler_;

	QLineEdit *searchEdit_ = nullptr;
	PickerPanel *picker_ = nullptr;
	std::optional<bible::Reference> anchor_; // last verse clicked in the picker, for Shift+click ranges
	StagePanel *preview_ = nullptr;
	StagePanel *program_ = nullptr;
	QPushButton *previousButton_ = nullptr;
	QPushButton *nextButton_ = nullptr;
	QPushButton *hideButton_ = nullptr;
	QPushButton *onAirButton_ = nullptr;
	QComboBox *themeBox_ = nullptr;
	QPushButton *pagePreviousButton_ = nullptr;
	QPushButton *pageNextButton_ = nullptr;
	QLabel *pageLabel_ = nullptr;
	QLabel *statusLabel_ = nullptr;
};

} // namespace vyra::ui
