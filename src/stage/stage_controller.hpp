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
 */

#include <optional>
#include <string>
#include <vector>

#include "src/bible/bible_module.hpp"
#include "src/search/passage_query.hpp"

namespace vyra::stage {

struct SlideVerse {
	int chapter = 0;
	int verse = 0;
	std::string text;
};

/** Everything needed to draw a passage: its printed reference and its verses, in reading order. */
struct Slide {
	std::string reference;
	std::vector<SlideVerse> verses;
};

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

private:
	std::optional<bible::Reference> verseAfterPreview() const;
	std::optional<bible::Reference> verseBeforePreview() const;

	const bible::BibleModule &module_;
	std::optional<search::Passage> preview_;
	std::optional<search::Passage> program_;
	bool programVisible_ = false;
};

} // namespace vyra::stage
