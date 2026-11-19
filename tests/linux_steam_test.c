// SPDX-License-Identifier: GPL-3.0-or-later
// The Steam shortcut writer, against synthetic shortcut lists: no Steam, no
// Deck, no network. Every case is a file this test writes itself.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../linux/src/linux_steam.c"

static const char* root = "/tmp/bw_steam_test";

static void put(const char* path, const void* data, size_t size) {
    FILE* f = fopen(path, "wb");
    assert(f);
    assert(fwrite(data, 1, size, f) == size);
    assert(fclose(f) == 0);
}

static size_t read_all(const char* path, unsigned char* out, size_t size) {
    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return 0;
    const size_t n = fread(out, 1, size, f);
    fclose(f);
    return n;
}

// A shortcut list with `count` entries, each "name"/"exe", built the way Steam
// writes one: [0x00]"shortcuts" then one object per entry, [0x08][0x08].
static void write_vdf_ids(const char* path, const char* const* names, const char* const* exes,
                          const char* const* ids, int count) {
    FILE* f = fopen(path, "wb");
    assert(f);
    write_key(f, VDF_OBJECT, "shortcuts");
    for (int i = 0; i < count; ++i) {
        char index[16];
        snprintf(index, sizeof index, "%d", 700000000 + i);
        assert(write_key(f, VDF_OBJECT, index));
        assert(write_u32_field(f, "appid", app_id_for(exes[i], names[i])));
        assert(write_string_field(f, "AppName", names[i]));
        assert(write_string_field(f, "Exe", exes[i]));
        assert(write_string_field(f, "StartDir", "/games"));
        assert(write_string_field(f, "icon", ""));
        assert(write_string_field(f, "ShortcutPath", ids != NULL && ids[i] != NULL ? ids[i] : ""));
        assert(write_string_field(f, "LaunchOptions", "keep-me"));
        assert(write_u32_field(f, "IsHidden", 0));
        assert(write_u32_field(f, "AllowDesktopConfig", 1));
        assert(write_u32_field(f, "AllowOverlay", 1));
        assert(write_u32_field(f, "OpenVR", 0));
        assert(write_u32_field(f, "Devkit", 0));
        assert(write_string_field(f, "DevkitGameID", ""));
        assert(write_u32_field(f, "LastPlayTime", 1234));
        // Steam writes a `tags` object under every entry (a game's categories);
        // the writer has to carry it along, byte for byte.
        assert(write_key(f, VDF_OBJECT, "tags"));
        assert(write_string_field(f, "0", "RPG"));
        assert(write_string_field(f, "1", "Test"));
        const unsigned char tags_end = VDF_END;
        assert(write_bytes(f, &tags_end, 1));
        const unsigned char end = VDF_END;
        assert(write_bytes(f, &end, 1));
    }
    const unsigned char ends[2] = {VDF_END, VDF_END};
    assert(write_bytes(f, ends, sizeof ends));
    assert(fclose(f) == 0);
}

static void write_vdf(const char* path, const char* const* names, const char* const* exes, int count) {
    write_vdf_ids(path, names, exes, NULL, count);
}

static int count_named(const char* path, const char* name) {
    setenv("BLUEWAKE_STEAM_VDF", path, 1);
    bw_entries list = {NULL, 0};
    int existed = 0;
    assert(load_entries(path, &list, &existed));
    int n = 0;
    for (size_t i = 0; i < list.count; ++i) {
        const char* entry_name = field_text(&list.entries[i], "AppName");
        if (entry_name != NULL && strcmp(entry_name, name) == 0)
            ++n;
    }
    entries_free(&list);
    return n;
}

int main(void) {
    char command[512];
    snprintf(command, sizeof command, "rm -rf %s && mkdir -p %s/home/.steam/steam/userdata %s/Games",
             root, root, root);
    assert(system(command) == 0);

    char home[512], vdf[600], exe[600], moved[600];
    snprintf(home, sizeof home, "%s/home", root);
    snprintf(vdf, sizeof vdf, "%s/shortcuts.vdf", root);
    snprintf(exe, sizeof exe, "%s/BlueWake-x86_64.AppImage", root);
    snprintf(moved, sizeof moved, "%s/Games/BlueWake-x86_64.AppImage", root);
    setenv("HOME", home, 1);
    setenv("BLUEWAKE_STEAM_VDF", vdf, 1);
    unsetenv("BLUEWAKE_STEAM");
    unsetenv("SteamDeck");

    // --- detection
    assert(bw_steam_available() == 0);
    setenv("BLUEWAKE_STEAM", "0", 1);
    setenv("SteamDeck", "1", 1);
    assert(bw_steam_available() == 0); // the switch wins over the environment
    assert(strcmp(bw_steam_reason(), "BLUEWAKE_STEAM=0") == 0);
    unsetenv("BLUEWAKE_STEAM");
    assert(bw_steam_available() == 1);
    assert(strstr(bw_steam_reason(), "SteamDeck=1") != NULL);
    unsetenv("SteamDeck");

    // --- one numeric account directory, taken as the library
    char userdata[600];
    snprintf(userdata, sizeof userdata, "%s/.steam/steam/userdata/12345678", home);
    char mkdir_cmd[800];
    snprintf(mkdir_cmd, sizeof mkdir_cmd, "mkdir -p %s/config", userdata);
    assert(system(mkdir_cmd) == 0);
    unsetenv("BLUEWAKE_STEAM_VDF");
    char resolved[1024], expected[1200];
    assert(bw_steam_shortcuts_path(resolved, sizeof resolved) == 1);
    snprintf(expected, sizeof expected, "%s/config/shortcuts.vdf", userdata);
    assert(strcmp(resolved, expected) == 0);

    // Two accounts, and only one of them has a shortcut list: that is the one
    // Steam is signed in to, and it is the one written. (A Deck really does
    // hold two.)
    {
        // The account Steam is signed in to already has a (synthetic) list.
        put(expected, "synthetic", 9);
        char second[700], second_vdf[900];
        snprintf(second, sizeof second, "%s/.steam/steam/userdata/87654321", home);
        snprintf(second_vdf, sizeof second_vdf, "%s/config/shortcuts.vdf", second);
        snprintf(mkdir_cmd, sizeof mkdir_cmd, "mkdir -p %s/config", second);
        assert(system(mkdir_cmd) == 0);
        assert(bw_steam_shortcuts_path(resolved, sizeof resolved) == 1);
        assert(strcmp(resolved, expected) == 0);
        // ... and with both carrying one, the newest wins: Steam touches the
        // list of the account it is using.
        put(second_vdf, "synthetic", 9);
        struct timespec times[2] = {{0, 0}, {0, 0}};
        {   // make the first one clearly older, then the second one newer
            char older[4096];
            snprintf(older, sizeof older, "touch -d '2026-01-01' %s && touch -d '2026-06-01' %s",
                     expected, second_vdf);
            assert(system(older) == 0);
        }
        (void)times;
        assert(bw_steam_shortcuts_path(resolved, sizeof resolved) == 1);
        assert(strcmp(resolved, second_vdf) == 0);
        snprintf(mkdir_cmd, sizeof mkdir_cmd, "rm -rf %s", second);
        assert(system(mkdir_cmd) == 0);
    }
    setenv("BLUEWAKE_STEAM_VDF", vdf, 1);

    // --- absent -> added
    put(exe, "synthetic appimage", 17);
    char detail[512];
    assert(bw_steam_shortcut_state(exe, "BlueWake") == BW_STEAM_ABSENT);
    assert(bw_steam_add_shortcut(exe, exe, "BlueWake", "", detail, sizeof detail) == BW_STEAM_ADDED);
    assert(bw_steam_shortcut_state(exe, "BlueWake") == BW_STEAM_CURRENT);
    assert(count_named(vdf, "BlueWake") == 1);

    unsigned char first[8192];
    const size_t first_size = read_all(vdf, first, sizeof first);
    assert(first_size > 0);

    // --- adding again changes nothing at all
    assert(bw_steam_add_shortcut(exe, exe, "BlueWake", "", detail, sizeof detail) == BW_STEAM_UNCHANGED);
    unsigned char again[8192];
    const size_t again_size = read_all(vdf, again, sizeof again);
    assert(again_size == first_size && memcmp(first, again, first_size) == 0);

    // --- the AppImage moved: the entry is rewritten, not duplicated
    put(moved, "synthetic appimage", 17);
    assert(bw_steam_shortcut_state(moved, "BlueWake") == BW_STEAM_STALE);
    assert(bw_steam_add_shortcut(moved, moved, "BlueWake", "", detail, sizeof detail) == BW_STEAM_UPDATED);
    assert(bw_steam_shortcut_state(moved, "BlueWake") == BW_STEAM_CURRENT);
    assert(bw_steam_shortcut_state(exe, "BlueWake") == BW_STEAM_STALE);
    assert(count_named(vdf, "BlueWake") == 1);

    // --- the backup holds the list as it was before the update
    char backup[700];
    snprintf(backup, sizeof backup, "%s.bak", vdf);
    unsigned char saved[8192];
    const size_t saved_size = read_all(backup, saved, sizeof saved);
    assert(saved_size == first_size && memcmp(first, saved, first_size) == 0);

    // --- other games' entries survive, fields and all
    {
        const char* names[] = {"Another Game", "BlueWake"};
        const char* exes[] = {"/games/another", "/games/old-bluewake"};
        write_vdf(vdf, names, exes, 2);
        assert(bw_steam_add_shortcut(moved, moved, "BlueWake", "", detail, sizeof detail) == BW_STEAM_UPDATED);
        assert(count_named(vdf, "Another Game") == 1);
        assert(count_named(vdf, "BlueWake") == 1);
        bw_entries list = {NULL, 0};
        int existed = 0;
        assert(load_entries(vdf, &list, &existed));
        assert(list.count == 2);
        const char* la = field_text(&list.entries[0], "LaunchOptions");
        assert(la != NULL && strcmp(la, "keep-me") == 0);
        const char* nm = field_text(&list.entries[1], "AppName");
        const char* ex = field_text(&list.entries[1], "Exe");
        assert(nm != NULL && strcmp(nm, "BlueWake") == 0);
        assert(ex != NULL && strcmp(ex, moved) == 0);
        entries_free(&list);
    }

    // --- the AppImage needed the env wrapper (no FUSE): Steam runs /usr/bin/env,
    // and the entry is still recognised as this AppImage's, so no second tile.
    {
        char wrapped[2048];
        snprintf(wrapped, sizeof wrapped, "APPIMAGE_EXTRACT_AND_RUN=1 %s", moved);
        const char* names[] = {"BlueWake"};
        const char* exes[] = {"/usr/bin/env"};
        const char* ids[] = {moved};
        write_vdf_ids(vdf, names, exes, ids, 1);
        assert(bw_steam_shortcut_state(moved, "BlueWake") == BW_STEAM_CURRENT);
        assert(bw_steam_add_shortcut("/usr/bin/env", moved, "BlueWake", wrapped, detail,
                                     sizeof detail) == BW_STEAM_UNCHANGED);
        bw_entries list = {NULL, 0};
        int existed = 0;
        assert(load_entries(vdf, &list, &existed));
        assert(list.count == 1);
        const char* ex = field_text(&list.entries[0], "Exe");
        const char* id = field_text(&list.entries[0], "ShortcutPath");
        const char* lo = field_text(&list.entries[0], "LaunchOptions");
        assert(ex != NULL && strcmp(ex, "/usr/bin/env") == 0);
        assert(id != NULL && strcmp(id, moved) == 0);
        assert(lo != NULL && strcmp(lo, "keep-me") == 0);
        entries_free(&list);
    }

    // --- Steam running: refused, and the list is left exactly as it was
    {
        const char* names[] = {"BlueWake"};
        const char* exes[] = {"/games/old-bluewake"};
        write_vdf(vdf, names, exes, 1);
        unsigned char before[8192];
        const size_t before_size = read_all(vdf, before, sizeof before);
        char pidfile[700], pid_cmd[900];
        snprintf(pidfile, sizeof pidfile, "%s/.steam/steam/steam.pid", home);
        snprintf(pid_cmd, sizeof pid_cmd, "printf '%%d\\n' %d > %s", (int)getpid(), pidfile);
        assert(system(pid_cmd) == 0);
        assert(bw_steam_running() == 1);
        assert(bw_steam_add_shortcut(moved, moved, "BlueWake", "", detail, sizeof detail) == BW_STEAM_FAILED);
        assert(strstr(detail, "Steam is running") != NULL);
        unsigned char after[8192];
        const size_t after_size = read_all(vdf, after, sizeof after);
        assert(after_size == before_size && memcmp(before, after, before_size) == 0);
        snprintf(pid_cmd, sizeof pid_cmd, "rm -f %s", pidfile);
        assert(system(pid_cmd) == 0);
    }

    // --- Steam running and the SteamOS helper present: the helper is asked,
    // rather than the file being refused for a list Steam will overwrite.
    {
        // The fake helper is written beside this binary, not under $root: a
        // test run can sit on a mount that allows data but not execution.
        char self[1024], helper[1600], asked[700], helper_cmd[4096];
        ssize_t got_self = readlink("/proc/self/exe", self, sizeof self - 1);
        assert(got_self > 0);
        self[got_self] = '\0';
        char* slash_self = strrchr(self, '/');
        assert(slash_self != NULL);
        *slash_self = '\0';
        snprintf(helper, sizeof helper, "%s/bw_steam_test_helper.sh", self);
        snprintf(asked, sizeof asked, "%s/helper-argv.txt", root);
        {
            FILE* f = fopen(helper, "wb");
            assert(f);
            fprintf(f, "#!/bin/sh\nprintf '%%s\\n' \"$@\" >> '%s'\n", asked);
            assert(fclose(f) == 0);
        }
        snprintf(helper_cmd, sizeof helper_cmd, "chmod +x %s && rm -f %s", helper, asked);
        assert(system(helper_cmd) == 0);
        setenv("BLUEWAKE_STEAM_HELPER", helper, 1);
        char pidfile[700], pid_cmd[900];
        snprintf(pidfile, sizeof pidfile, "%s/.steam/steam/steam.pid", home);
        snprintf(pid_cmd, sizeof pid_cmd, "printf '%%d\\n' %d > %s", (int)getpid(), pidfile);
        assert(system(pid_cmd) == 0);
        char fresh[700];
        snprintf(fresh, sizeof fresh, "%s/Fresh.AppImage", root);
        put(fresh, "synthetic appimage", 17);
        snprintf(detail, sizeof detail, "%s", "untouched");
        assert(bw_steam_add_shortcut(fresh, fresh, "BlueWake", "", detail, sizeof detail) ==
               BW_STEAM_ADDED);
        char got[1024] = "";
        // The helper is spawned, not waited for: give it a moment to run.
        FILE* f = NULL;
        for (int tries = 0; tries < 100 && f == NULL; ++tries) {
            usleep(20000);
            f = fopen(asked, "rb");
        }
        assert(f != NULL);
        assert(fgets(got, sizeof got, f) != NULL);
        fclose(f);
        got[strcspn(got, "\n")] = '\0';
        assert(strcmp(got, fresh) == 0);
        unsetenv("BLUEWAKE_STEAM_HELPER");
        snprintf(pid_cmd, sizeof pid_cmd, "rm -f %s", pidfile);
        assert(system(pid_cmd) == 0);
    }

    // --- a relative path is refused before anything is touched
    assert(bw_steam_add_shortcut("BlueWake.AppImage", "BlueWake.AppImage", "BlueWake", "", detail, sizeof detail) ==
           BW_STEAM_FAILED);

    // --- no Steam userdata at all: a clear refusal, and no file created
    {
        char empty_home[600], empty_vdf[700], other[1400];
        snprintf(empty_home, sizeof empty_home, "%s/empty", root);
        snprintf(empty_vdf, sizeof empty_vdf, "%s/shortcuts.vdf", empty_home);
        snprintf(other, sizeof other, "rm -rf %s && mkdir -p %s", empty_home, empty_home);
        assert(system(other) == 0);
        setenv("HOME", empty_home, 1);
        unsetenv("BLUEWAKE_STEAM_VDF");
        snprintf(detail, sizeof detail, "%s", "untouched");
        assert(bw_steam_add_shortcut(moved, moved, "BlueWake", "", detail, sizeof detail) == BW_STEAM_FAILED);
        assert(strstr(detail, "no Steam userdata") != NULL);
        FILE* f = fopen(empty_vdf, "rb");
        assert(f == NULL);
    }

    printf("linux_steam_test: OK (synthetic shortcut lists; files retained in %s)\n", root);
    return 0;
}
