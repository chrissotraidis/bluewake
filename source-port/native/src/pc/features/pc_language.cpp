// The game's language in the European build (docs/PAL_DISC_PLAN.md, "Language").
//
// The PAL disc holds five languages, dComIfGs_getPalLanguage() 0-4: English, German, French,
// Spanish, Italian (the SDK's OS_LANGUAGE_* order). On the GameCube the logo scene takes the one
// the memory card's save remembers (the file select can change it), else the console's (IPL)
// language, OSGetLanguage(). Here the console's language is the Switch's system language, or the
// Mac's/Linux's preferred locale (SDL_GetPreferredLocales: the system settings on the Mac, LANGUAGE/
// LC_ALL/LC_MESSAGES/LANG on Linux); it goes into the SDK's SRAM so OSGetLanguage returns it.
// A language other than the five is English, as on the GameCube (the game maps 5, Dutch, to 0).
//
// COS_GAME_LANGUAGE (the options menu, PAL build only; restart): "auto" (default) keeps the
// GameCube's order (the card's language, else the console's); en/de/fr/es/it always start in that
// language. The language is chosen once, in the logo scene (it loads that language's message
// archives), so a change takes effect at the next start.
#include "pc_internal.h"

#include "global.h" // VERSION_PAL
#include "d/d_com_inf_game.h"
#include "pc/game_hooks.h"
#include "pc/pc_settings.h"

#include <cstdlib>
#include <cstring>
#include <dolphin/os.h>
#include <unistd.h>

#if defined(__SWITCH__)
#include "cos_switch.h"
#else
#include <SDL3/SDL_locale.h>
#endif

#if VERSION == VERSION_PAL

namespace {

constexpr const char* kCodes[] = {"en", "de", "fr", "es", "it"};
constexpr const char* kNames[] = {"English", "Deutsch", "Français", "Español", "Italiano"};
constexpr int kLanguages = 5;

const PcSettingChoice kChoices[] = {
    {"auto", "Automático (idioma del sistema)", "Automatic (system language)"},
    {"en", "English", nullptr},
    {"de", "Deutsch", nullptr},
    {"fr", "Français", nullptr},
    {"es", "Español", nullptr},
    {"it", "Italiano", nullptr},
};

const PcSettingDesc kSetting = {
    "COS_GAME_LANGUAGE", "Idioma del juego",
    "Idioma de los textos del juego. Automático: el de la partida guardada, si no el del sistema. "
    "Requiere reiniciar.",
    PC_SETTING_TAB_GRAPHICS, PC_SETTING_RESTART, kChoices, (int)(sizeof(kChoices) / sizeof(kChoices[0])),
    "auto", nullptr, nullptr, 99, // the last row of the tab: the menu scripts count rows from the top
    "Game language",
    "Language of the game's text. Automatic: the saved game's, else the system's. Needs a restart."};

bool sChosen = false;       // the logo scene has chosen the language (languageFrame logs changes after)
char sSystemCode[32] = ""; // the code the console language came from ("de-DE"), for the log
int sSystem = 0;           // its language, 0-4

// "de", "de-DE", "de_DE.UTF-8" -> 1; anything else -> -1
int codeLanguage(const char* code) {
    if (code == nullptr) {
        return -1;
    }
    for (int i = 0; i < kLanguages; i++) {
        if (strncmp(code, kCodes[i], 2) == 0 && (code[2] == '\0' || code[2] == '-' || code[2] == '_' ||
                                                 code[2] == '.')) {
            return i;
        }
    }
    return -1;
}

void readSystemLanguage() {
#if defined(__SWITCH__)
    cos_switch_system_language(sSystemCode, sizeof(sSystemCode));
#else
    int count = 0;
    SDL_Locale** locales = SDL_GetPreferredLocales(&count);
    if (locales != nullptr && count > 0 && locales[0] != nullptr && locales[0]->language != nullptr) {
        snprintf(sSystemCode, sizeof(sSystemCode), "%s%s%s", locales[0]->language,
                 locales[0]->country != nullptr ? "-" : "", locales[0]->country != nullptr ? locales[0]->country : "");
    }
    SDL_free(locales);
#endif
    const int lang = codeLanguage(sSystemCode);
    sSystem = lang >= 0 ? lang : 0;
}

} // namespace

namespace pc {
void languageInit() {
    pc_settings_register(&kSetting);
    readSystemLanguage();
    // the console's language, for the game's OSGetLanguage (the logo scene, when the card has none)
    OSSetLanguage((u8)sSystem);
    writef(STDERR_FILENO, "[cos] language: system %s -> %s; COS_GAME_LANGUAGE=%s\n",
           sSystemCode[0] != '\0' ? sSystemCode : "(unknown)", kCodes[sSystem],
           pc_settings_get("COS_GAME_LANGUAGE"));
}

void languageFrame() {
    static int sLast = -1;
    if (!sChosen) {
        return;
    }
    const int lang = dComIfGs_getPalLanguage();
    if (sLast >= 0 && lang != sLast) {
        writef(STDERR_FILENO, "[cos] language: the game changed it to %s\n", lang < kLanguages ? kNames[lang] : "?");
    }
    sLast = lang;
}
} // namespace pc

int pc_logo_pal_language(int game, bool fromCard) {
    const char* setting = pc_settings_get("COS_GAME_LANGUAGE");
    const int chosen = codeLanguage(setting);
    const int lang = chosen >= 0 ? chosen : (game >= 0 && game < kLanguages ? game : 0);
    sChosen = true;
    pc::writef(STDERR_FILENO, "[cos] language: %s (%s)\n", kNames[lang],
           chosen >= 0 ? "COS_GAME_LANGUAGE"
                       : (fromCard ? "the memory card's save" : "the system language"));
    return lang;
}

unsigned char pc_card_pal_language(unsigned char saved) {
    const int chosen = codeLanguage(pc_settings_get("COS_GAME_LANGUAGE"));
    return chosen >= 0 ? (unsigned char)chosen : saved;
}

#else // the USA build has one language

namespace pc {
void languageInit() {}
void languageFrame() {}
} // namespace pc

#endif
