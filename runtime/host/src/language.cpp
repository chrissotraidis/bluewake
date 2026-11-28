// SPDX-License-Identifier: GPL-3.0-or-later
// Reading a language catalog (language.h, docs/LANGUAGES.md). One file, one
// language: `key = text` lines, `#` for a comment, a blank line for nothing.
// The menu draws on one thread, so the strings this holds are not guarded; a
// load happens when the menu opens or the player picks a language, never
// while a frame is being drawn.
#include "language.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// windows/compat turns rename() into a function-like macro for the memory
// card's atomic save (bw_posix_compat.h), and <filesystem> defines its own
// rename() beside the rest of std::filesystem: with the macro in the way that
// definition is rewritten and the header cannot parse. Nothing here renames
// anything, so the name goes back before the header that uses it.
#if defined(_WIN32)
#undef rename
#endif
#include <filesystem>
#include <map>

namespace bw_lang {
namespace {

std::map<std::string, std::string> g_text;  // key -> the loaded language's text
std::string g_code;                         // "" = English
std::vector<unsigned int> g_characters;     // above ASCII, in the loaded catalog
int g_generation;                           // counts loads, for the menu's font
char g_key[160];                            // the derived key of the row drawn now
char g_filled[1024];                        // fill()'s answer

std::string trim(const std::string& in) {
    const size_t first = in.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    const size_t last = in.find_last_not_of(" \t\r\n");
    return in.substr(first, last - first + 1);
}

// `\n`, `\t` and `\\` in a value, so a label that needs a line break stays on
// one line of the file where a translator can see it whole.
std::string unescape(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] != '\\' || i + 1 >= in.size()) {
            out += in[i];
            continue;
        }
        switch (in[++i]) {
        case 'n': out += '\n'; break;
        case 't': out += '\t'; break;
        case '\\': out += '\\'; break;
        default: out += '\\'; out += in[i]; break;  // \= and anything else as written
        }
    }
    return out;
}

const char* lookup(const char* key) {
    const auto found = g_text.find(key);
    return found != g_text.end() ? found->second.c_str() : nullptr;
}

// The codepoints of one UTF-8 string, so the font can be built with exactly
// the characters a catalog uses. Bytes that are not UTF-8 are skipped rather
// than drawn as garbage.
void add_characters(const std::string& text) {
    for (size_t i = 0; i < text.size();) {
        const unsigned char lead = static_cast<unsigned char>(text[i]);
        unsigned int point = 0;
        size_t width = 0;
        if (lead < 0x80) {
            ++i;
            continue;  // ASCII: the atlas has it already
        } else if ((lead & 0xE0) == 0xC0) {
            point = lead & 0x1Fu, width = 2;
        } else if ((lead & 0xF0) == 0xE0) {
            point = lead & 0x0Fu, width = 3;
        } else if ((lead & 0xF8) == 0xF0) {
            point = lead & 0x07u, width = 4;
        } else {
            ++i;
            continue;
        }
        if (i + width > text.size())
            break;
        bool complete = true;
        for (size_t k = 1; k < width; ++k) {
            const unsigned char next = static_cast<unsigned char>(text[i + k]);
            if ((next & 0xC0) != 0x80) {
                complete = false;
                break;
            }
            point = (point << 6) | (next & 0x3Fu);
        }
        if (!complete) {
            ++i;
            continue;
        }
        g_characters.push_back(point);
        i += width;
    }
}

// one line of a catalog: the key and the text, or nothing when the line is
// not one of those (a comment, a blank line, a line with no `=`).
bool parse_line(const std::string& raw, std::string* key, std::string* value) {
    const std::string line = trim(raw);
    if (line.empty() || line[0] == '#' || line[0] == ';')
        return false;
    const size_t equals = line.find('=');
    if (equals == std::string::npos || equals == 0)
        return false;
    *key = trim(line.substr(0, equals));
    *value = unescape(trim(line.substr(equals + 1)));
    return !key->empty();
}

}  // namespace

std::string folder_for(const std::string& config_dir) {
    if (config_dir.empty())
        return {};
    return (std::filesystem::path(config_dir) / "languages").string();
}

bool load(const std::string& folder, const std::string& code) {
    g_text.clear();
    g_characters.clear();
    g_code.clear();
    ++g_generation;
    if (code.empty() || code == "en")
        return true;
    const std::string path = (std::filesystem::path(folder) / (code + ".txt")).string();
    FILE* file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) {
        std::fprintf(stderr, "[language] %s: cannot read %s; the menu stays in English\n", code.c_str(),
                     path.c_str());
        return false;
    }
    char line[4096];
    int lines = 0, skipped = 0, repeated = 0, number = 0;
    while (std::fgets(line, sizeof line, file) != nullptr) {
        ++number;
        const std::string raw = trim(line);
        if (raw.empty() || raw[0] == '#' || raw[0] == ';')
            continue;
        std::string key, value;
        if (!parse_line(raw, &key, &value)) {
            std::fprintf(stderr, "[language] %s:%d: no `key = text`, skipped\n", path.c_str(), number);
            ++skipped;
            continue;
        }
        if (!g_text.emplace(key, value).second) {
            g_text[key] = value;  // the last line wins, and the file is not refused for it
            ++repeated;
        }
        add_characters(value);
        ++lines;
    }
    std::fclose(file);
    std::sort(g_characters.begin(), g_characters.end());
    g_characters.erase(std::unique(g_characters.begin(), g_characters.end()), g_characters.end());
    if (lines == 0) {
        std::fprintf(stderr, "[language] %s: no strings in %s; the menu stays in English\n", code.c_str(),
                     path.c_str());
        g_characters.clear();
        return false;
    }
    g_code = code;
    std::fprintf(stderr, "[language] %s: %d strings from %s%s%s\n", code.c_str(), lines, path.c_str(),
                 skipped != 0 ? ", some lines skipped" : "", repeated != 0 ? ", repeated keys" : "");
    return true;
}

std::string resolve(const std::string& folder, const std::string& chosen, const std::string& system) {
    const auto have = [&folder](const std::string& code) {
        if (code.empty())
            return false;
        std::error_code error;
        return std::filesystem::is_regular_file(std::filesystem::path(folder) / (code + ".txt"), error);
    };
    if (!chosen.empty())
        return chosen;  // what the player picked; load() says if it has gone
    std::string language = system;
    std::transform(language.begin(), language.end(), language.begin(),
                   [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    if (have(language))
        return language;
    const size_t cut = language.find_first_of("-_");
    if (cut != std::string::npos)
        language.resize(cut);
    return have(language) ? language : std::string();
}

const std::string& code() { return g_code; }
int generation() { return g_generation; }
std::size_t count() { return g_text.size(); }
bool english() { return g_text.empty(); }

const char* text(const char* key, const char* english) {
    if (key == nullptr || g_text.empty())
        return english;
    const char* found = lookup(key);
    return found != nullptr ? found : english;
}

const char* fill(const char* key, const char* english, const char* value) {
    const char* whole = text(key, english);
    const char* where = std::strstr(whole, "%s");
    if (where == nullptr)
        return whole;  // a line that lost its %s still reads as a sentence
    const size_t before = static_cast<size_t>(where - whole);
    std::snprintf(g_filled, sizeof g_filled, "%.*s%s%s", static_cast<int>(before), whole,
                  value != nullptr ? value : "", where + 2);
    return g_filled;
}

const char* label(const bw_settings::Setting& s) {
    if (s.label == nullptr)
        return nullptr;
    std::snprintf(g_key, sizeof g_key, "setting.%s.label", s.key);
    return text(g_key, s.label);
}

const char* note(const bw_settings::Setting& s) {
    if (s.note == nullptr)
        return nullptr;
    std::snprintf(g_key, sizeof g_key, "setting.%s.note", s.key);
    return text(g_key, s.note);
}

const char* group(const bw_settings::Setting& s) {
    if (s.group == nullptr)
        return nullptr;
    if (s.group[0] == '\0')
        return s.group;  // the plain separator between sections
    std::snprintf(g_key, sizeof g_key, "setting.%s.group", s.key);
    return text(g_key, s.group);
}

const char* experimental(const bw_settings::Setting& s) {
    if (s.experimental == nullptr)
        return nullptr;
    std::snprintf(g_key, sizeof g_key, "setting.%s.experimental", s.key);
    return text(g_key, s.experimental);
}

const char* tab(bw_settings::Tab which) {
    // Each name beside its key, spelled out as bw_lang:: so the check in
    // tests/test_i18n_strings.py reads it like every other call, and a catalog
    // can reword a tab.
    switch (which) {
    case bw_settings::Tab::Display: return bw_lang::text("tab.display", "Display");
    case bw_settings::Tab::Gameplay: return bw_lang::text("tab.gameplay", "Gameplay");
    case bw_settings::Tab::Mods: return bw_lang::text("tab.mods", "Mods");
    case bw_settings::Tab::Controls: return bw_lang::text("tab.controls", "Controls");
    case bw_settings::Tab::Config: return bw_lang::text("tab.config", "Config");
    }
    return "";
}

std::vector<std::string> available(const std::string& folder) {
    std::vector<std::string> codes;
    if (folder.empty())
        return codes;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(folder, error)) {
        if (!entry.is_regular_file(error))
            continue;
        const std::string name = entry.path().filename().string();
        if (name.size() < 5 || entry.path().extension() != ".txt")
            continue;
        const std::string stem = entry.path().stem().string();
        if (stem != "en")
            codes.push_back(stem);
    }
    std::sort(codes.begin(), codes.end());
    return codes;
}

std::string display_name(const std::string& folder, const std::string& code) {
    const std::string path = (std::filesystem::path(folder) / (code + ".txt")).string();
    FILE* file = std::fopen(path.c_str(), "rb");
    if (file != nullptr) {
        char line[4096];
        while (std::fgets(line, sizeof line, file) != nullptr) {
            std::string key, value;
            if (parse_line(line, &key, &value) && key == "language.name" && !value.empty()) {
                std::fclose(file);
                return value;
            }
        }
        std::fclose(file);
    }
    return code;  // a file that names itself nothing reads as its own name
}

const std::vector<unsigned int>& characters() { return g_characters; }

}  // namespace bw_lang
