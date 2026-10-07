/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <functional>

#include "src/stage/operator_action.hpp"

namespace vyra::obs {

/**
 * The global OBS hotkeys of the plugin (Settings > Hotkeys > "VYRA Bible: ..."). They work even when the
 * dock is not focused. Keys are chosen by the operator in OBS and saved with the OBS profile; none is
 * bound by default, so the plugin can never steal a key from another one.
 *
 * OBS calls the handler from ITS hotkey thread, not from the Qt thread: the handler given here must hand
 * the action over to the Qt thread (the plugin does it with a queued call) and return at once.
 * Only the key press is reported, not the release.
 */
using HotkeyHandler = std::function<void(stage::OperatorAction)>;

/** Registers the hotkeys. @p description turns a hotkey name into its translated label. Call once, from the Qt thread. */
void registerHotkeys(HotkeyHandler handler);

/** Unregisters them and forgets the handler. Safe to call when nothing was registered. */
void unregisterHotkeys();

} // namespace vyra::obs
