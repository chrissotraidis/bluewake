// SPDX-License-Identifier: GPL-3.0-or-later
// Reading a language catalog (runtime/host/src/language.h): a line is a key
// and its text, a line the code does not know is ignored, a key the catalog
// lacks reads as the English the code carries, and the characters a catalog
// uses are reported so the menu's font can have them.
//
// No ImGui and no window: this is the reader a translator's file meets.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

#include "../runtime/host/src/language.h"

namespace fs = std::filesystem;

static void write_file(const fs::path& path, const std::string& text) {
    FILE* file = std::fopen(path.string().c_str(), "wb");
    assert(file != nullptr);
    assert(std::fwrite(text.data(), 1, text.size(), file) == text.size());
    std::fclose(file);
}

int main() {
    const fs::path folder = fs::temp_directory_path() /
                            ("bluewake_i18n_test_" +
                             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(folder);
    // A catalog as a translator writes one: a name for itself, two settings,
    // a comment, a blank line, an escaped newline, a line that is not a key
    // and a key no row asks for.
    write_file(folder / "es.txt",
               "# Español, a mano\n"
               "\n"
               "language.name = Español\n"
               "tab.display = Pantalla\n"
               "setting.BLUEWAKE_ASPECT.label = Panorámica\n"
               "setting.BLUEWAKE_HD_LOOK.note = La mirada de la versión HD,\\nno escrita por nadie.\n"
               "menu.turn_on = Activa «%s» para cambiarlo.\n"
               "setting.NOT_A_SETTING.label = Nota suelta\n"
               "this line is not a key\n");
    // Not a catalog: no .txt, and so not offered.
    write_file(folder / "notes.md", "setting.BLUEWAKE_ASPECT.label = Nope\n");
    // English is the absence of a file, so it is never listed.
    write_file(folder / "en.txt", "tab.display = English\n");

    // English until a catalog is loaded, and after one is unloaded again.
    const int before = bw_lang::generation();
    assert(bw_lang::english());
    assert(std::strcmp(bw_lang::text("tab.display", "Display"), "Display") == 0);
    assert(bw_lang::characters().empty());

    assert(bw_lang::load(folder.string(), "es"));
    assert(!bw_lang::english());
    assert(bw_lang::code() == "es");
    assert(bw_lang::generation() > before);
    assert(bw_lang::count() == 6);  // the six lines, `NOT_A_SETTING` included
    // A translated line, a key the catalog lacks, and a key it has that no row
    // asks for: the last two still read as the English beside them.
    assert(std::strcmp(bw_lang::text("tab.display", "Display"), "Pantalla") == 0);
    assert(std::strcmp(bw_lang::text("tab.mods", "Mods"), "Mods") == 0);
    // `\n` in a line is a line break in the text, so a quiet line stays one
    // line of the file.
    assert(std::strstr(bw_lang::text("setting.BLUEWAKE_HD_LOOK.note", ""), "\n") != nullptr);
    assert(std::strcmp(bw_lang::text("setting.NOT_A_SETTING.label", "?"), "Nota suelta") == 0);
    // An `=` in the text is part of the text: the first one splits the line.
    assert(bw_lang::display_name(folder.string(), "es") == "Español");
    assert(std::strcmp(bw_lang::text("menu.turn_on", "Turn on"),
                       "Activa «%s» para cambiarlo.") == 0);
    // fill() puts the value where the catalog's %s is, not the English one's.
    assert(std::strcmp(bw_lang::fill("menu.turn_on", "Turn on \"%s\" to change this.", "Panorámica"),
                       "Activa «Panorámica» para cambiarlo.") == 0);
    assert(std::strcmp(bw_lang::fill("menu.off_while", "Off while \"%s\" is set.", "Smooth Motion"),
                       "Off while \"Smooth Motion\" is set.") == 0);
    // The characters the font has to be built with: everything above ASCII the
    // catalog uses, once each, in order.
    const std::vector<unsigned int>& characters = bw_lang::characters();
    assert(!characters.empty());
    assert(characters.front() == 0xAB);  // «
    for (size_t i = 1; i < characters.size(); ++i)
        assert(characters[i - 1] < characters[i]);
    assert(std::find(characters.begin(), characters.end(), 0x00E1u) != characters.end());  // á

    // A row's own keys: its label, and nullptr where the row has none, so a
    // catalog cannot add a line the menu never draws.
    bw_settings::Setting row{};
    row.key = "BLUEWAKE_ASPECT";
    row.label = "Widescreen";
    row.note = nullptr;
    assert(std::strcmp(bw_lang::label(row), "Panorámica") == 0);
    assert(bw_lang::note(row) == nullptr);
    bw_settings::Setting untranslated{};
    untranslated.key = "BLUEWAKE_ASPECT";
    untranslated.label = "Aspect";
    assert(std::strcmp(bw_lang::label(untranslated), "Panorámica") == 0);  // keyed by the setting
    bw_settings::Setting other{};
    other.key = "BLUEWAKE_STICK_ZOOM";
    other.label = "Hold the right-stick click";
    other.group = "";
    assert(std::strcmp(bw_lang::label(other), "Hold the right-stick click") == 0);
    assert(bw_lang::group(other) != nullptr && bw_lang::group(other)[0] == '\0');
    assert(bw_lang::tab(bw_settings::Tab::Display) == std::string("Pantalla"));
    assert(bw_lang::tab(bw_settings::Tab::Mods) == std::string("Mods"));

    // What a setting resolves to: what was chosen, else the system's language,
    // else the language alone when the system names a country too, else English.
    assert(bw_lang::resolve(folder.string(), "es", "") == "es");
    assert(bw_lang::resolve(folder.string(), "", "es-ES") == "es");
    assert(bw_lang::resolve(folder.string(), "", "ES") == "es");  // the system's spelling is not its case
    assert(bw_lang::resolve(folder.string(), "", "de-DE").empty());  // no such catalog
    assert(bw_lang::resolve(folder.string(), "", "").empty());
    assert(bw_lang::resolve(folder.string(), "es", "de-DE") == "es");  // a pick beats the system
    assert(bw_lang::resolve(folder.string(), "zz", "de-DE") == "zz");  // and a pick that has gone is reported by load()

    // The catalogs in the folder: the one .txt that is not English.
    const std::vector<std::string> codes = bw_lang::available(folder.string());
    assert(codes.size() == 1 && codes[0] == "es");
    assert(bw_lang::display_name(folder.string(), "en") == "en");  // no such line: its name

    // English again, and a catalog that is not there leaves it that way.
    assert(bw_lang::load(folder.string(), ""));
    assert(bw_lang::english());
    assert(std::strcmp(bw_lang::text("tab.display", "Display"), "Display") == 0);
    assert(!bw_lang::load(folder.string(), "de"));
    assert(bw_lang::english());
    assert(!bw_lang::load(folder.string(), "notes"));  // not a catalog: no lines
    assert(bw_lang::english());

    std::error_code error;
    fs::remove_all(folder, error);
    std::printf("i18n: a catalog's lines, its fallbacks and the font's characters read as they should\n");
    return 0;
}
