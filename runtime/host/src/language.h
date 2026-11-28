// SPDX-License-Identifier: GPL-3.0-or-later
// The settings menu's text in another language: the file a translator writes
// down (docs/LANGUAGES.md). Nothing here draws, and nothing here includes
// ImGui, so the host tests can read a catalog on their own and the font can
// ask which characters the menu is about to draw.
//
// A catalog is lines of `key = text`. A setting's key is its own name, so a
// translator's file names the same thing the settings file does, and the
// English stays in the code beside the key: a catalog missing a line shows
// that one row in English rather than a blank, and a half-finished
// translation is usable from its first sentence.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include "settings_table.h"

namespace bw_lang {

// Where a player's catalogs live: `languages` beside the settings file.
std::string folder_for(const std::string& config_dir);

// Load `<folder>/<code>.txt`. "" and "en" load none, so every string is the
// English the code carries. False when the file cannot be read, and then the
// menu stays in English rather than in half of a language.
bool load(const std::string& folder, const std::string& code);

// The catalog a setting resolves to, so that a player who has never chosen one
// gets their system's language and everyone else gets what they chose: `chosen`
// when it names anything (load() reports a file that has since gone), else the
// catalog matching `system` — "pt-br" first, then "pt" — else "" for English.
// `system` is the platform's own answer (SDL's preferred locale here): "es",
// "pt-br", "" when it has none.
std::string resolve(const std::string& folder, const std::string& chosen, const std::string& system);

// The catalog in use ("" for English), how many strings it holds, and whether
// that is none of them.
const std::string& code();
std::size_t count();
bool english();

// How many times a catalog has been loaded. The menu builds its font when this
// changes: a language can need characters the atlas it was built with has none
// of, so picking one has to rebuild it rather than wait for a restart.
int generation();

// The text for `key`, or the English the caller carries when the catalog has
// no line for it. The returned pointer stays good until the next load.
const char* text(const char* key, const char* english);

// A one-value sentence: `why` is the catalog's line for `key` ("Turn on %s to
// change this.") with its one %s replaced by `value`. English when there is no
// such line. Handing a catalog's line to snprintf as a format instead would
// put a translator's text in charge of the arguments.
const char* fill(const char* key, const char* english, const char* value);

// A setting row's text, from the row's own keys: `setting.<its key>.label`,
// `.note`, `.group` and `.experimental`. A row with no note in English has no
// note in any language (nullptr), so a catalog cannot add a line the menu does
// not draw; an empty group is still the plain separator it was.
const char* label(const bw_settings::Setting& s);
const char* note(const bw_settings::Setting& s);
const char* group(const bw_settings::Setting& s);
const char* experimental(const bw_settings::Setting& s);

// A tab's name: `tab.display`, `tab.gameplay`, `tab.mods`, `tab.controls`,
// `tab.config`.
const char* tab(bw_settings::Tab which);

// Every catalog in the folder, by its file's name without `.txt`, in order;
// "en" is left out, as English is always on offer and needs no file.
std::vector<std::string> available(const std::string& folder);

// What a catalog calls itself: its `language.name` line ("Español"), or its
// file's name when it has none.
std::string display_name(const std::string& folder, const std::string& code);

// Every character above ASCII in the catalog in use, so the menu's font can be
// built with them: an atlas made for English has no Cyrillic or Greek in it,
// and a letter it lacks draws as a box.
const std::vector<unsigned int>& characters();

}  // namespace bw_lang
