/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "src/obs/hotkeys.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <mutex>
#include <vector>

namespace vyra::obs {

namespace {

struct Entry {
	stage::OperatorAction action;
	const char *name;     // stable id saved in the OBS profile: never rename
	const char *labelKey; // locale key of the label shown in OBS
	obs_hotkey_id id = OBS_INVALID_HOTKEY_ID;
};

std::vector<Entry> g_entries = {
	{stage::OperatorAction::OnAir, "VyraBible.OnAir", "Hotkey.OnAir"},
	{stage::OperatorAction::Hide, "VyraBible.Hide", "Hotkey.Hide"},
	{stage::OperatorAction::PreviewNext, "VyraBible.PreviewNext", "Hotkey.PreviewNext"},
	{stage::OperatorAction::PreviewPrevious, "VyraBible.PreviewPrevious", "Hotkey.PreviewPrevious"},
	{stage::OperatorAction::PageNext, "VyraBible.PageNext", "Hotkey.PageNext"},
	{stage::OperatorAction::PagePrevious, "VyraBible.PagePrevious", "Hotkey.PagePrevious"},
};

constexpr const char *kSaveKey = "vyra_bible_hotkeys";

std::mutex g_mutex; // the handler is read from the hotkey thread and replaced from the Qt thread
HotkeyHandler g_handler;
bool g_saveCallbackAdded = false;

void onHotkey(void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (!pressed)
		return;
	HotkeyHandler handler;
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		handler = g_handler;
	}
	if (handler)
		handler(*static_cast<stage::OperatorAction *>(data));
}

/** Keeps the key bindings in the OBS profile, next to the rest of the scene collection data. */
void onSaveLoad(obs_data_t *saveData, bool saving, void *)
{
	if (saving) {
		obs_data_t *all = obs_data_create();
		for (const Entry &entry : g_entries) {
			if (entry.id == OBS_INVALID_HOTKEY_ID)
				continue;
			obs_data_array_t *bindings = obs_hotkey_save(entry.id);
			obs_data_set_array(all, entry.name, bindings);
			obs_data_array_release(bindings);
		}
		obs_data_set_obj(saveData, kSaveKey, all);
		obs_data_release(all);
		return;
	}
	obs_data_t *all = obs_data_get_obj(saveData, kSaveKey);
	if (!all)
		return;
	for (const Entry &entry : g_entries) {
		if (entry.id == OBS_INVALID_HOTKEY_ID)
			continue;
		obs_data_array_t *bindings = obs_data_get_array(all, entry.name);
		if (bindings) {
			obs_hotkey_load(entry.id, bindings);
			obs_data_array_release(bindings);
		}
	}
	obs_data_release(all);
}

} // namespace

void registerHotkeys(HotkeyHandler handler)
{
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		g_handler = std::move(handler);
	}
	for (Entry &entry : g_entries) {
		if (entry.id != OBS_INVALID_HOTKEY_ID)
			continue; // already registered
		entry.id = obs_hotkey_register_frontend(entry.name, obs_module_text(entry.labelKey), onHotkey,
							&entry.action);
	}
	if (!g_saveCallbackAdded) {
		obs_frontend_add_save_callback(onSaveLoad, nullptr);
		g_saveCallbackAdded = true;
	}
}

void unregisterHotkeys()
{
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		g_handler = nullptr;
	}
	if (g_saveCallbackAdded) {
		obs_frontend_remove_save_callback(onSaveLoad, nullptr);
		g_saveCallbackAdded = false;
	}
	for (Entry &entry : g_entries) {
		if (entry.id != OBS_INVALID_HOTKEY_ID)
			obs_hotkey_unregister(entry.id);
		entry.id = OBS_INVALID_HOTKEY_ID;
	}
}

} // namespace vyra::obs
