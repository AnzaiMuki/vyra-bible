/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

/*
 * The Preview / Program state machine of the control room.
 *
 * Independent of OBS and Qt, so that the rules which protect the live display can be tested.
 * The rules, in one place:
 *   - Everything the operator does (typing, previous, next) only changes the PREVIEW.
 *   - Only takeOnAir() changes what is on the PROGRAM: it copies the preview to the program.
 *   - hideProgram() takes the passage off the air but keeps it, so it can be put back with takeOnAir().
 *   - Preview and Program are never changed by a failed operation.
 *   - A long passage on the program is cut in pages (paginate); nextPage/previousPage move the PROGRAM
 *     only, on purpose and one page at a time. Putting a passage on the air always starts at page 1.
 *   - Changing the theme re-cuts the pages of the program, so the program goes back to page 1.
 */

#include <optional>
#include <string>
#include <vector>

#include "src/bible/bible_module.hpp"
#include "src/search/passage_query.hpp"
#include "src/stage/paginator.hpp"
#include "src/stage/slide.hpp"

namespace vyra::stage {

/** How the passage is drawn on the air. Chosen by the operator; changing it never changes what is on the air. */
enum class Theme { LowerThird, FullScreen, Minimal };

/** Name used on the wire and in the overlay page: "lower", "full", "minimal". */
const char *themeName(Theme theme);
/** Characters that fit one screen of the theme (the overlay then shrinks the font to fit exactly). */
std::size_t pageBudget(Theme theme);

class StageController {
public:
	/** @p module must outlive the controller. */
	explicit StageController(const bible::BibleModule &module) : module_(module) {}

	const std::optional<search::Passage> &preview() const { return preview_; }
	/** The passage held by the program, shown or not. Check programLive() to know if it is on the air. */
	const std::optional<search::Passage> &program() const { return program_; }
	bool programLive() const { return program_.has_value() && programVisible_; }

	/** Puts a passage in the preview. Refuses (false) a passage that is not fully inside the Bible. */
	bool showInPreview(const search::Passage &passage);

	/** The single verse after the end of the preview (crossing chapters and books). False if none. */
	bool previewNext();
	/** The single verse before the start of the preview (crossing chapters and books). False if none. */
	bool previewPrevious();
	bool hasNext() const;
	bool hasPrevious() const;

	/** Copies the preview to the program and shows it. False if the preview is empty. */
	bool takeOnAir();
	/** Takes the program off the air, keeping its passage. False if it was not on the air. */
	bool hideProgram();

	Slide slide(const search::Passage &passage) const;

	const bible::BibleModule &module() const { return module_; }
	Theme theme() const { return theme_; }
	/** Sets the theme of the display. Re-cuts the program's pages (back to page 1). */
	void setTheme(Theme theme);

	/** Pages of the passage held by the program (empty if none). */
	const std::vector<Page> &programPages() const { return programPages_; }
	/** 0-based index of the page shown. */
	std::size_t programPage() const { return programPage_; }
	bool nextPage();     // false if not live or already on the last page
	bool previousPage(); // false if not live or already on the first page

private:
	std::optional<bible::Reference> verseAfterPreview() const;
	std::optional<bible::Reference> verseBeforePreview() const;

	void repaginate();

	const bible::BibleModule &module_;
	Theme theme_ = Theme::LowerThird;
	std::vector<Page> programPages_;
	std::size_t programPage_ = 0;
	std::optional<search::Passage> preview_;
	std::optional<search::Passage> program_;
	bool programVisible_ = false;
};

} // namespace vyra::stage
