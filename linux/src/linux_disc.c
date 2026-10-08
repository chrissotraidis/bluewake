// BlueWake for Linux: the player's disc, the first time and whenever it goes
// missing. See linux_disc.h.
//
// The launcher asks for the player's own disc image (GameCube USA, GZLE01
// revision 0): an .iso or .gcm is used where it is; other Dolphin formats are
// refused with a clear message (convert to ISO in Dolphin). The disc is
// checked (its id, and main.dol's SHA-1 for revision 0) and prepared once by
// the iOS app's importer (apple/ios/src/disc_import.c): main.dol and the 415
// RELs, into a fresh import folder. disc.txt remembers the disc and game.txt
// the completed preparation; prior folders are retained.
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>

#include "disc_import.h"
#include "linux_disc.h"
#include "atomic_file.h"

#define BW_GC_DISC_SIZE 1459978240ull

static const char* g_exe;
static const char* g_data;

static int is_file(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static int is_dir(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int mkdir_p(const char* path) {
    char tmp[4096];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
    return 0;
}

static int read_line(const char* path, char* out, size_t size) {
    out[0] = '\0';
    FILE* f = fopen(path, "rb");
    if (f == NULL) return 0;
    if (fgets(out, (int)size, f) == NULL) out[0] = '\0';
    fclose(f);
    size_t n = strlen(out);
    while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == '\r')) out[--n] = '\0';
    return n > 0;
}

static int write_line(const char* path, const char* line) {
    char* pending = bw_atomic_path(path);
    FILE* f = pending != NULL ? fopen(pending, "wb") : NULL;
    if (f == NULL) { free(pending); return 0; }
    const int written = f != NULL && fprintf(f, "%s\n", line) > 0;
    const int ok = pending != NULL && bw_atomic_finish(f, pending, path, written);
    free(pending);
    return ok;
}

static int fresh_import_dir(char* out, size_t size) {
    for (unsigned attempt = 0; attempt < 128; ++attempt) {
        int n = snprintf(out, size, "%sdisc-import-%ld-%llu-%u", g_data,
                         (long)getpid(), (unsigned long long)time(NULL), attempt);
        if (n < 0 || (size_t)n >= size) return -1;
        if (mkdir(out, 0755) == 0) return 0;
        if (errno != EEXIST) return -1;
    }
    return -1;
}

static int rel_count(const char* rels) {
    DIR* dir = opendir(rels);
    if (dir == NULL) return -1;
    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        count++;
    }
    closedir(dir);
    return count;
}

static int prepared_ready(const char* dol, const char* rels) {
    return is_file(dol) && rel_count(rels) == 415;
}

static void disc_stamp(const char* disc, char* out, size_t size) {
    struct stat info;
    if (stat(disc, &info) != 0) { snprintf(out, size, "%s", disc); return; }
    snprintf(out, size, "%s|%llu|%llu", disc,
             (unsigned long long)info.st_size,
             (unsigned long long)info.st_mtim.tv_sec);
}

static const char* extension(const char* path) {
    const char* dot = strrchr(path, '.');
    const char* slash = strrchr(path, '/');
    return dot != NULL && (slash == NULL || dot > slash) ? dot : "";
}

static int needs_unpacking(const char* path) {
    static const char* const kinds[] = {".rvz", ".wia", ".gcz", ".ciso", ".nfs", ".wbfs"};
    for (size_t i = 0; i < sizeof kinds / sizeof kinds[0]; i++)
        if (strcasecmp(extension(path), kinds[i]) == 0)
            return 1;
    return 0;
}

// --- the file picker (SDL3's native dialog) ---------------------------------

typedef struct {
    char result[4096];
    volatile bool done;
} ChooseState;

static void SDLCALL choose_callback(void* userdata, const char* const* filelist, int filter) {
    ChooseState* state = (ChooseState*)userdata;
    if (filelist != NULL && filelist[0] != NULL)
        snprintf(state->result, sizeof state->result, "%s", filelist[0]);
    state->done = true;
}

// The explanation goes to stderr (the session log); the picker is SDL's.
// 1 and the chosen path, or 0 when the player cancels.
static int choose(const char* why, char* out, size_t size) {
    const char* testing = getenv("BLUEWAKE_DISC_CHOICE");
    if (testing != NULL) {
        snprintf(out, size, "%s", testing);
        return testing[0] != '\0';
    }
    if (getenv("BLUEWAKE_NO_DIALOG") != NULL)
        return 0;
    fprintf(stderr,
            "%sBlueWake plays The Legend of Zelda: The Wind Waker from your own "
            "copy of the game: the GameCube disc for the USA (GZLE01).\n\n"
            "Choose your disc image next: an .iso or .gcm file. BlueWake checks "
            "it and prepares it once, then remembers it.\n", why);
    static const SDL_DialogFileFilter filters[] = {
        {"GameCube disc images", "iso;gcm"},
        {"All files", "*"},
    };
    ChooseState state = {{0}, false};
    SDL_ShowOpenFileDialog(choose_callback, &state, NULL, filters,
                           (int)(sizeof filters / sizeof filters[0]), NULL, false);
    // Pump until the callback fires (the dialog is modal and asynchronous).
    while (!state.done)
        SDL_PumpEvents();
    if (state.result[0] == '\0')
        return 0;
    snprintf(out, size, "%s", state.result);
    return 1;
}

int bw_disc_setup(const char* exe_dir, const char* data_dir) {
    g_exe = exe_dir;
    g_data = data_dir;
    const char* given_dol = getenv("BLUEWAKE_DOL");
    if (given_dol != NULL && given_dol[0] != '\0')
        return 0;
    char path[4096];
    const char* given = getenv("BLUEWAKE_DISC");
    const int explicit_disc = given != NULL && given[0] != '\0';
    // A folder the builder made from the player's disc has it all beside the app.
    char app_disc[4096], app_dol[4096], app_rels[4096];
    snprintf(app_disc, sizeof app_disc, "%sgame/GZLE01.iso", exe_dir);
    snprintf(app_dol, sizeof app_dol, "%sgame/main.dol", exe_dir);
    snprintf(app_rels, sizeof app_rels, "%sgame/rels", exe_dir);
    if (!explicit_disc && is_file(app_disc) && prepared_ready(app_dol, app_rels))
        return 0;

    char remembered[4096], game[900], game_record[4096], dol[4096], rels[4096], stamp_path[4096];
    snprintf(remembered, sizeof remembered, "%sdisc.txt", data_dir);
    snprintf(game_record, sizeof game_record, "%sgame.txt", data_dir);
    if (!read_line(game_record, game, sizeof game))
        snprintf(game, sizeof game, "%sgame", data_dir);
    snprintf(dol, sizeof dol, "%s/main.dol", game);
    snprintf(rels, sizeof rels, "%s/rels", game);
    snprintf(stamp_path, sizeof stamp_path, "%s/prepared.txt", game);
    char disc[4096] = "";
    if (explicit_disc)
        snprintf(disc, sizeof disc, "%s", given);
    else
        read_line(remembered, disc, sizeof disc);
    if (explicit_disc && !is_file(disc)) {
        char text[4096 + 128];
        snprintf(text, sizeof text, "BlueWake cannot find the disc image it was given:\n\n%s", disc);
        fprintf(stderr, "[disc] %s\n", text);
        return -1;
    }
    const char* why = disc[0] != '\0' && !is_file(disc) ? "The disc image BlueWake used before is gone.\n\n" : "";
    const int scripted = getenv("BLUEWAKE_DISC_CHOICE") != NULL;
    for (;;) {
        if (disc[0] == '\0' || !is_file(disc)) {
            if (!choose(why, path, sizeof path)) {
                fprintf(stderr, "[disc] no disc image chosen\n");
                return 1;
            }
            snprintf(disc, sizeof disc, "%s", path);
        }
        if (needs_unpacking(disc)) {
            fprintf(stderr,
                    "[disc] %s is a compressed Dolphin image; BlueWake on Linux reads an "
                    "uncompressed .iso or .gcm. Convert it in Dolphin (right-click the game, "
                    "Convert File -> ISO).\n", disc);
            if (explicit_disc || scripted)
                return -1;
            disc[0] = '\0';
            why = "";
            continue;
        }
        char error[1024] = "";
        if (bluewake_disc_check(disc, error, sizeof error) != 0) {
            fprintf(stderr, "[disc] %s: %s\n", disc, error);
            if (explicit_disc || scripted)
                return -1;
            disc[0] = '\0';
            why = "";
            continue;
        }
        char stamp[4096 + 64], prepared[4096 + 64];
        disc_stamp(disc, stamp, sizeof stamp);
        read_line(stamp_path, prepared, sizeof prepared);
        if (strcmp(stamp, prepared) != 0 || !prepared_ready(dol, rels)) {
            if (fresh_import_dir(game, sizeof game) != 0) {
                fprintf(stderr, "[disc] could not create a new preparation folder\n");
                return -1;
            }
            snprintf(dol, sizeof dol, "%s/main.dol", game);
            snprintf(rels, sizeof rels, "%s/rels", game);
            snprintf(stamp_path, sizeof stamp_path, "%s/prepared.txt", game);
            fprintf(stderr, "[disc] preparing %s\n", disc);
            if (bluewake_disc_prepare(disc, game, NULL, NULL, error, sizeof error) != 0) {
                fprintf(stderr, "[disc] %s: %s\n", disc, error);
                if (explicit_disc || scripted)
                    return -1;
                disc[0] = '\0';
                why = "";
                continue;
            }
            if (!write_line(stamp_path, stamp) || !write_line(game_record, game)) {
                fprintf(stderr, "[disc] could not remember the prepared files\n");
                return -1;
            }
        }
        break;
    }
    if (!explicit_disc && !write_line(remembered, disc)) {
        fprintf(stderr, "[disc] could not remember this disc\n");
        return -1;
    }
    setenv("BLUEWAKE_DISC", disc, 1);
    setenv("BLUEWAKE_DOL", dol, 1);
    setenv("BLUEWAKE_RELS_DIR", rels, 1);
    fprintf(stderr, "[disc] %s (prepared in %s)\n", disc, game);
    return 0;
}
