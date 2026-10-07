/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

namespace vyra::stage {

/** What an operator can ask without touching the dock: the actions behind the OBS hotkeys. */
enum class OperatorAction {
	OnAir,           // the passage in PREVIEW (or typed in the search bar, if valid) goes on the air
	Hide,            // takes the PROGRAM off the air
	PreviewNext,     // next verse, in PREVIEW only
	PreviewPrevious, // previous verse, in PREVIEW only
	PageNext,        // next page of the passage on the air
	PagePrevious     // previous page of the passage on the air
};

} // namespace vyra::stage
