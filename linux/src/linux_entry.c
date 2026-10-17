// BlueWake Linux entry shim.
//
// The Linux counterpart of windows/src/win_entry.c and apple/ios/src/ios_entry.m:
// it fills in default paths beside the executable and in the player's data
// folder, starts the session log, then runs the unchanged host
// (runtime/host/src/main.c, compiled with main renamed to bluewake_host_main).
//
// App folder (scripts/linux/build.py writes it; it is a personal build that
// contains code translated from your disc, never share it):
//   bluewake, libSDL3.so.*, libwebgpu_dawn.so    the host and its runtime
//   gGZLE01_recomp.so                           the translated game module
//   game/GZLE01.iso                              the disc image the game reads
//   game/main.dol, game/rels/                    prepared from that disc
//   dsp/dsp_rom.bin, dsp/dsp_coef.bin            only for BLUEWAKE_DSP_MODE=lle
// Player data, kept outside the app folder so a rebuild never touches it:
//   ~/.local/share/BlueWake/GZLE01.card          the memory card (saves)
//   ~/.local/share/BlueWake/sram.bin             the console's settings
//   ~/.local/share/BlueWake/logs/session-*.log   the newest eight sessions
// Portable mode (#64), as on Windows: with a file named portable.txt beside
// the bluewake executable, the player data goes in a user folder beside it
// instead (BlueWake/user/...).
// Every default is only a default: an environment variable that is already
// set (BLUEWAKE_DISC, BLUEWAKE_CARD_PATH, ...) wins.
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <SDL3/SDL.h>
#include <aurora/aurora.h>

#include "linux_disc.h"
#include "linux_crash.h"
#include "linux_setup.h"
#include "launch_marker.h"
#include "settings_menu.h"

int bluewake_host_main(int argc, char** argv);

static char g_exe_dir[4096];
static char g_data_dir[4096];
static char g_log_path[4096];
static int g_portable;

static int file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static int dir_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static void bw_default(const char* name, const char* value) {
    const char* existing = getenv(name);
    if (existing != NULL && existing[0] != '\0')
        return;
    setenv(name, value, 1);
}

static void bw_default_path(const char* name, const char* dir, const char* relative) {
    char path[4096];
    snprintf(path, sizeof path, "%s%s", dir, relative);
    bw_default(name, path);
}

// Aurora's own files: controller button remaps (*.controller), keyboard
// bindings, the controller port choice and imgui.ini. In portable mode they
// follow DOL_AURORA_USER_DIR into the user folder; copy the ones it doesn't
// have yet from the usual folder, so remaps made there carry over, as on
// Windows (#64). Nothing is moved or replaced.
static void bw_copy_aurora_files(void) {
    static const char* const names[] = {"*.controller", "keyboard_bindings.dat", "controller_ports.dat", "imgui.ini"};
    const char* data_home = getenv("XDG_DATA_HOME");
    const char* home = getenv("HOME");
    char usual[4096];
    if (data_home != NULL && data_home[0] != '\0')
        snprintf(usual, sizeof usual, "%s/BlueWake/", data_home);
    else if (home != NULL && home[0] != '\0')
        snprintf(usual, sizeof usual, "%s/.local/share/BlueWake/", home);
    else
        return;
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        char pattern[4096];
        snprintf(pattern, sizeof pattern, "%s%s", usual, names[i]);
        glob_t found;
        if (glob(pattern, 0, NULL, &found) != 0)
            continue;
        for (size_t j = 0; j < found.gl_pathc; j++) {
            const char* from = found.gl_pathv[j];
            const char* name = strrchr(from, '/') + 1;
            char to[4096];
            snprintf(to, sizeof to, "%s%s", g_data_dir, name);
            if (file_exists(to))
                continue;
            FILE* in = fopen(from, "rb");
            FILE* out = in != NULL ? fopen(to, "wb") : NULL;
            if (out != NULL) {
                char buf[16384];
                size_t n;
                while ((n = fread(buf, 1, sizeof buf, in)) > 0)
                    fwrite(buf, 1, n, out);
                fclose(out);
                fprintf(stderr, "[portable] copied %s from %s\n", name, usual);
            }
            if (in != NULL)
                fclose(in);
        }
        globfree(&found);
    }
}

// mkdir -p: the data folder and any missing folders above it (a fresh home
// without ~/.local/share, or a BLUEWAKE_DATA_DIR that does not exist yet).
static void make_dirs(const char* path) {
    char partial[4096];
    snprintf(partial, sizeof partial, "%s", path);
    for (char* p = partial + 1; *p != '\0'; p++) {
        if (*p != '/')
            continue;
        *p = '\0';
        mkdir(partial, 0755);
        *p = '/';
    }
    mkdir(partial, 0755);
}

static void resolve_dirs(void) {
    // The executable's own path.
    char link[64];
    snprintf(link, sizeof link, "/proc/self/exe");
    ssize_t n = readlink(link, g_exe_dir, sizeof g_exe_dir - 1);
    if (n > 0) {
        g_exe_dir[n] = '\0';
        char* slash = strrchr(g_exe_dir, '/');
        if (slash != NULL) slash[1] = '\0';
    } else {
        snprintf(g_exe_dir, sizeof g_exe_dir, "./");
    }
    // Player data: BLUEWAKE_DATA_DIR, else the user folder beside the
    // executable with portable.txt (#64), else $XDG_DATA_HOME/BlueWake, else
    // ~/.local/share/BlueWake.
    const char* override = getenv("BLUEWAKE_DATA_DIR");
    const char* data_home = getenv("XDG_DATA_HOME");
    const char* home = getenv("HOME");
    char portable[4096];
    snprintf(portable, sizeof portable, "%sportable.txt", g_exe_dir);
    if (override != NULL && override[0] != '\0') {
        snprintf(g_data_dir, sizeof g_data_dir, "%s/", override);
    } else if (file_exists(portable)) {
        snprintf(g_data_dir, sizeof g_data_dir, "%suser/", g_exe_dir);
        g_portable = 1;
    } else if (data_home != NULL && data_home[0] != '\0') {
        snprintf(g_data_dir, sizeof g_data_dir, "%s/BlueWake/", data_home);
    } else if (home != NULL && home[0] != '\0') {
        snprintf(g_data_dir, sizeof g_data_dir, "%s/.local/share/BlueWake/", home);
    } else {
        snprintf(g_data_dir, sizeof g_data_dir, "%suser/", g_exe_dir);
    }
    make_dirs(g_data_dir);
    char logs[4096];
    snprintf(logs, sizeof logs, "%slogs", g_data_dir);
    mkdir(logs, 0755);
    char states[4096];
    snprintf(states, sizeof states, "%sstates", g_data_dir);
    mkdir(states, 0755);
    // Portable mode keeps the settings file there too, instead of
    // ~/.config/BlueWake/settings.ini (as Windows keeps it in the user folder).
    if (g_portable)
        bw_default_path("BLUEWAKE_SETTINGS", g_data_dir, "settings.ini");
}

// --- session log -----------------------------------------------------------
// Everything the host writes to stdout and stderr also goes to
// ~/.local/share/BlueWake/logs/session-YYYYMMDD-HHMMSS.log, each line stamped
// with the local time, and still to the terminal BlueWake was started from.
static pthread_t g_log_thread;
static int g_log_pipe[2];
static FILE* g_log_file;
static int g_term_out = -1;
static int g_term_err = -1;  // the terminal's stderr, kept for restore_terminal_streams

static void* log_pump(void* arg) {
    (void)arg;
    char buffer[16384];
    char line[8192];
    size_t line_len = 0;
    for (;;) {
        ssize_t n = read(g_log_pipe[0], buffer, sizeof buffer);
        if (n <= 0)
            break;
        for (ssize_t i = 0; i < n; i++) {
            if (line_len < sizeof line - 1)
                line[line_len++] = buffer[i];
            if (buffer[i] != '\n')
                continue;
            line[line_len] = '\0';
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            struct tm local;
            localtime_r(&ts.tv_sec, &local);
            fprintf(g_log_file, "%02d:%02d:%02d.%03ld %s", local.tm_hour, local.tm_min, local.tm_sec,
                    ts.tv_nsec / 1000000, line);
            fflush(g_log_file);
            if (g_term_out >= 0)
                (void)write(g_term_out, line, line_len);
            line_len = 0;
        }
    }
    return NULL;
}

static int session_name_cmp(const void* a, const void* b) {
    return strcmp(*(const char* const*)a, *(const char* const*)b);
}

static void prune_logs(const char* dir, int keep) {
    char names[256][64];
    int count = 0;
    DIR* d = opendir(dir);
    if (d == NULL)
        return;
    struct dirent* entry;
    while ((entry = readdir(d)) != NULL && count < 256) {
        if (strncmp(entry->d_name, "session-", 8) == 0 &&
            strlen(entry->d_name) < sizeof names[0])
            snprintf(names[count++], sizeof names[0], "%s", entry->d_name);
    }
    closedir(d);
    const char* ptrs[256];
    for (int i = 0; i < count; i++) ptrs[i] = names[i];
    qsort(ptrs, (size_t)count, sizeof ptrs[0], session_name_cmp);
    for (int i = 0; i + keep < count; i++) {
        char path[4096];
        snprintf(path, sizeof path, "%s%s", dir, ptrs[i]);
        remove(path);
    }
}

static void finish_session_log(void) {
    if (!g_log_file)
        return;
    fflush(stdout);
    fflush(stderr);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    pthread_join(g_log_thread, NULL);
    fclose(g_log_file);
    g_log_file = NULL;
}

// Before Restart now's execve: the session log closed, and the terminal's own
// streams back on 1 and 2, so the next session's log doesn't write into this
// one's pipe (with no reader after the execve, it filled and the game froze).
static void restore_terminal_streams(void) {
    finish_session_log();
    if (g_term_out >= 0) dup2(g_term_out, STDOUT_FILENO);
    if (g_term_err >= 0) dup2(g_term_err, STDERR_FILENO);
}

static void start_session_log(void) {
    char logs[4096];
    snprintf(logs, sizeof logs, "%slogs/", g_data_dir);
    mkdir(logs, 0755);
    prune_logs(logs, 7);
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    char name[64];
    strftime(name, sizeof name, "session-%Y%m%d-%H%M%S", &local);
    snprintf(g_log_path, sizeof g_log_path, "%s%s.log", logs, name);
    g_log_file = fopen(g_log_path, "w");
    if (g_log_file == NULL || pipe(g_log_pipe) != 0) {
        g_log_path[0] = '\0';
        if (g_log_file) { fclose(g_log_file); g_log_file = NULL; }
        return;
    }
    // Both standard streams point at the pipe; the terminal's own fd 2 stays open.
    fflush(stdout);
    fflush(stderr);
    int out_fd = dup(STDOUT_FILENO);
    int err_fd = dup(STDERR_FILENO);
    if (out_fd < 0 || err_fd < 0) { if (out_fd >= 0) close(out_fd); if (err_fd >= 0) close(err_fd); }
    g_term_out = out_fd;
    g_term_err = err_fd;
    // None of these may reach a restarted BlueWake (Restart now's execve):
    // its own session log starts from the terminal's streams, put back first.
    fcntl(g_log_pipe[0], F_SETFD, FD_CLOEXEC);
    if (out_fd >= 0) fcntl(out_fd, F_SETFD, FD_CLOEXEC);
    if (err_fd >= 0) fcntl(err_fd, F_SETFD, FD_CLOEXEC);
    dup2(g_log_pipe[1], STDOUT_FILENO);
    dup2(g_log_pipe[1], STDERR_FILENO);
    close(g_log_pipe[1]);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    pthread_create(&g_log_thread, NULL, log_pump, NULL);
    atexit(finish_session_log);
}

static void usage(void) {
    fprintf(stderr,
            "usage: bluewake [options] [DISC.iso]\n"
            "  --widescreen       16:9 (the widescreen mod: a wider camera and HUD)\n"
            "  --aspect A         4:3 (the game's own), 16:10, 16:9 or 21:9\n"
            "  --smooth           Smooth Motion (experimental): 60 FPS with in-between\n"
            "                     frames; F10 toggles it\n"
            "  --no-smooth        the game's own 30 FPS (the default)\n"
            "  --betterww         Better Wind Waker's settings, at their defaults\n"
            "  --options LIST     change them: name,-name,... (mods/betterww/options.txt)\n"
            "  --fullscreen       start in fullscreen (F11 toggles it while playing)\n"
            "  --window WxH       the window's size\n"
            "  --scale N          render at N x 480 lines (0: the window's own pixels)\n"
            "  --fps              show the frame rate\n"
            "  --stretch          fill the window instead of keeping the game's aspect\n"
            "  --no-mouse-camera  keep the mouse out of the camera\n"
            "  --safe-mode        start safe: Fast sound, 4:3, a window, no mods, options or extras\n"
            "  --hle-audio        Fast audio for this session\n"
            "  --lle-audio        run the DSP's own microcode instead of the HLE ucode\n"
            "  --stage-select-saves  keep saving on after using the stage select (it can\n"
            "                     leave your file in a state the story never reaches)\n"
            "  --mods LIST        mods compiled into the module, by name\n"
            "  --setup            configure the disc, texture pack and app launchers\n"
            "  --iso              choose and remember a disc with the simple file picker\n"
            "  --disc FILE        the disc image to read (a bare .iso/.gcm path also works)\n"
            "  --module FILE      the translated game module (default gGZLE01_recomp.so)\n"
            "Keyboard: arrows D-pad, J A, K B, U X, I Y, W/A/S/D stick,\n"
            "H/F/T/G C-stick, E/R L/R, Q Z, Return START. Game controllers work too.\n"
            "Mouse: click the game, then move it to turn the camera; Esc releases it.\n"
            "F1 or Esc settings, F11 or Alt+Enter fullscreen, F10 Smooth Motion, F9 frame rate,\n"
            "F5 save state, F8 load it, F2 HD look, F7 stage select.\n"
            "The settings menu saves to ~/.config/BlueWake/settings.ini; options given\n"
            "here win for the session.\n"
            "Portable mode: with a file named portable.txt beside bluewake, saves,\n"
            "settings, logs and caches stay in a user folder beside it instead.\n");
}

// The environment BlueWake started with, for Restart now: the new session
// gets the same command line and environment, and the settings file decides
// the rest (as on Windows).
extern char** environ;
static char** g_launch_env;

static void capture_launch_env(void) {
    size_t count = 0;
    while (environ[count] != NULL)
        ++count;
    g_launch_env = calloc(count + 1, sizeof *g_launch_env);
    for (size_t i = 0; g_launch_env != NULL && i < count; ++i)
        g_launch_env[i] = strdup(environ[i]);
}

int main(int argc, char** argv) {
    capture_launch_env();
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) { usage(); return 0; }
    resolve_dirs();
    SDL_SetAppMetadata("BlueWake", "0.1", "dev.bluewake.BlueWake");
    if (getenv("BLUEWAKE_SESSION_LOG") == NULL || strcmp(getenv("BLUEWAKE_SESSION_LOG"), "0") != 0)
        start_session_log();

    const char* module_arg = NULL;
    int setup_requested = 0;
    int iso_picker_requested = 0;
    char mods[256] = "";
    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];
        const int more = i + 1 < argc;
        if (strcmp(a, "--disc") == 0 && more) {
            setenv("BLUEWAKE_DISC", argv[++i], 1);
        } else if (a[0] != '-' &&
                   (getenv("BLUEWAKE_DISC") == NULL || getenv("BLUEWAKE_DISC")[0] == '\0')) {
            const char* dot = strrchr(a, '.');
            if (dot != NULL && (strcasecmp(dot, ".iso") == 0 || strcasecmp(dot, ".gcm") == 0))
                setenv("BLUEWAKE_DISC", a, 1);
            else {
                fprintf(stderr, "BlueWake: unknown option %s\n", a);
                usage();
                return 2;
            }
        } else if (strcmp(a, "--module") == 0 && more) {
            module_arg = argv[++i];
        } else if (strcmp(a, "--mods") == 0 && more) {
            snprintf(mods + strlen(mods), sizeof mods - strlen(mods), "%s%s", mods[0] ? "," : "", argv[++i]);
        } else if (strcmp(a, "--setup") == 0) {
            setup_requested = 1;
        } else if (strcmp(a, "--iso") == 0) {
            iso_picker_requested = 1;
        } else if (strcmp(a, "--betterww") == 0) {
            snprintf(mods + strlen(mods), sizeof mods - strlen(mods), "%sbetterww", mods[0] ? "," : "");
        } else if (strcmp(a, "--options") == 0 && more) {
            setenv("BLUEWAKE_OPTIONS", argv[++i], 1);
        } else if (strcmp(a, "--widescreen") == 0) {
            setenv("BLUEWAKE_ASPECT", "16:9", 1);
        } else if (strcmp(a, "--aspect") == 0 && more) {
            setenv("BLUEWAKE_ASPECT", argv[++i], 1);
        } else if (strcmp(a, "--smooth") == 0) {
            setenv("DOL_AURORA_FRAME_INTERP", "1", 1);
            setenv("DOL_AURORA_FRAME_INTERP_STEPS", "1", 1);
            aurora_set_frame_interp_steps(1);
            aurora_set_frame_interpolation(true);
        } else if (strcmp(a, "--no-smooth") == 0) {
            setenv("DOL_AURORA_FRAME_INTERP", "0", 1);
            aurora_set_frame_interpolation(false);
        } else if (strcmp(a, "--fullscreen") == 0) {
            setenv("DOL_AURORA_FULLSCREEN", "1", 1);
        } else if (strcmp(a, "--window") == 0 && more) {
            setenv("DOL_AURORA_WINDOW", argv[++i], 1);
        } else if (strcmp(a, "--scale") == 0 && more) {
            setenv("DOL_AURORA_RENDER_SCALE", argv[++i], 1);
        } else if (strcmp(a, "--fps") == 0) {
            setenv("DOL_AURORA_SHOW_FPS", "1", 1);
            aurora_set_fps_overlay(true);
        } else if (strcmp(a, "--stretch") == 0) {
            setenv("DOL_AURORA_ASPECT_FIT", "0", 1);
        } else if (strcmp(a, "--no-mouse-camera") == 0) {
            setenv("BLUEWAKE_MOUSE_CAMERA", "0", 1);
        } else if (strcmp(a, "--safe-mode") == 0) {
            setenv("BLUEWAKE_SAFE_MODE", "1", 1);
        } else if (strcmp(a, "--stage-select-saves") == 0) {
            setenv("BLUEWAKE_STAGE_SELECT_SAVES", "1", 1);
        } else if (strcmp(a, "--hle-audio") == 0) {
            setenv("BLUEWAKE_DSP_MODE", "hle", 1);
        } else if (strcmp(a, "--lle-audio") == 0) {
            setenv("BLUEWAKE_DSP_MODE", "lle", 1);
        } else if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
            usage();
            return 0;
        } else {
            fprintf(stderr, "BlueWake: unknown option %s\n", a);
            usage();
            return 2;
        }
    }
    if (mods[0] != '\0')
        setenv("BLUEWAKE_MODS", mods, 1);

    const char* appimage = getenv("APPIMAGE");
    const char* explicit_disc = getenv("BLUEWAKE_DISC");
    const int has_explicit_disc = explicit_disc != NULL && explicit_disc[0] != '\0';
    // A downloaded AppImage guides a first-time player through disc selection,
    // optional textures and shortcut installation. Once its remembered disc is
    // usable, an ordinary launch goes straight to the game. --setup always
    // reopens the populated window; --iso preserves the former simple picker.
    const int use_setup = setup_requested ||
        (!iso_picker_requested && !has_explicit_disc && appimage != NULL && appimage[0] != '\0' &&
         !bw_disc_has_usable_source(g_exe_dir, g_data_dir));
    const int use_iso_picker = iso_picker_requested && !use_setup;
    if (use_setup) {
        char selected_disc[4096] = "";
        if (explicit_disc != NULL)
            snprintf(selected_disc, sizeof selected_disc, "%s", explicit_disc);
        const int setup = bw_linux_setup(g_data_dir, selected_disc, sizeof selected_disc);
        if (setup <= 0)
            return setup < 0 ? 1 : 0;
        setenv("BLUEWAKE_DISC", selected_disc, 1);
    }

    // The play configuration as the iOS/Windows apps: one perf line a second,
    // wall-clock pacing, the real clock, and the
    // console's SRAM kept with the saves.
    bw_default("BLUEWAKE_PERF_LOG", "1");
    bw_default("BLUEWAKE_WALL_PACE", "1");
    bw_default("DOL_AUDIO_NO_THROTTLE", "1");
    bw_default("BLUEWAKE_RENDERER", "aurora");
    bw_default("BLUEWAKE_CYCLE_CAP", "16384");
    bw_default("BLUEWAKE_MAX_BLOCKS", "100000000000");
    // Fast sound (BLUEWAKE_DSP_MODE=hle) is the settings loader's default, so the
    // settings file's choice isn't taken for a launch option (settings_menu.cpp).
    bw_default("BLUEWAKE_CLOCK", "now");
    bw_default_path("BLUEWAKE_SRAM", g_data_dir, "sram.bin");
    bw_default_path("BLUEWAKE_CARD_PATH", g_data_dir, "GZLE01.card");
    bw_default("BLUEWAKE_STATE_DIR", g_data_dir);
    // Aurora's shader and pipeline caches (dawn_cache.db, pipeline_cache.db)
    // and its own files (controller remaps, keyboard bindings, imgui.ini) go
    // with the rest of the player's data: the same ~/.local/share/BlueWake as
    // before (SDL's folder for the app), or the user folder in portable mode,
    // which otherwise still filled ~/.local/share/BlueWake (#64).
    bw_default("DOL_AURORA_CACHE_DIR", g_data_dir);
    bw_default("DOL_AURORA_USER_DIR", g_data_dir);
    if (g_portable)
        bw_copy_aurora_files();
    // The stage select's English names, which the builder installs beside the executable.
    char stage_names[4096];
    snprintf(stage_names, sizeof stage_names, "%sstage_select", g_exe_dir);
    struct stat stage_names_dir;
    if (stat(stage_names, &stage_names_dir) == 0 && S_ISDIR(stage_names_dir.st_mode))
        bw_default("BLUEWAKE_STAGE_SELECT_NAMES", stage_names);

    char module[4096];
    const char* module_env = getenv("BLUEWAKE_COMPOSITE");
    if (module_arg != NULL)
        snprintf(module, sizeof module, "%s", module_arg);
    else if (module_env != NULL && module_env[0] != '\0')
        snprintf(module, sizeof module, "%s", module_env);
    else
        snprintf(module, sizeof module, "%sgGZLE01_recomp.so", g_exe_dir);

    if (!file_exists(module)) {
        fprintf(stderr,
                "The player-generated game module is missing. Build your own copy from "
                "your USA revision-0 disc with python scripts/linux/build.py YOUR_DISC.iso. "
                "Keep that personal build local.\n");
        return 1;
    }
    const int disc_status = bw_disc_setup(g_exe_dir, g_data_dir, use_setup, use_iso_picker);
    if (disc_status != 0) return disc_status < 0 ? 1 : 0;
    bw_default_path("BLUEWAKE_DOL", g_exe_dir, "game/main.dol");
    bw_default_path("BLUEWAKE_RELS_DIR", g_exe_dir, "game/rels");
    bw_default_path("BLUEWAKE_DISC", g_exe_dir, "game/GZLE01.iso");
    bw_default_path("BLUEWAKE_DSP_IROM", g_exe_dir, "dsp/dsp_rom.bin");
    bw_default_path("BLUEWAKE_DSP_COEF", g_exe_dir, "dsp/dsp_coef.bin");

    const char* missing = NULL;
    const char* missing_path = NULL;
    if (!file_exists(module)) {
        missing = "the translated game module";
        missing_path = module;
    } else if (!file_exists(getenv("BLUEWAKE_DOL"))) {
        missing = "the prepared game executable";
        missing_path = getenv("BLUEWAKE_DOL");
    } else if (!dir_exists(getenv("BLUEWAKE_RELS_DIR"))) {
        missing = "the prepared game modules";
        missing_path = getenv("BLUEWAKE_RELS_DIR");
    } else if (!file_exists(getenv("BLUEWAKE_DISC"))) {
        missing = "the disc image";
        missing_path = getenv("BLUEWAKE_DISC");
    }
    if (missing != NULL) {
        fprintf(stderr,
                "BlueWake cannot start: %s is missing.\n\n%s\n\n"
                "Build your own copy from your disc with\n"
                "python scripts/linux/build.py YOUR_DISC.iso\n",
                missing, missing_path);
        return 1;
    }

    fprintf(stderr, "[linux] app=%s data=%s%s module=%s disc=%s\n", g_exe_dir, g_data_dir,
            g_portable ? " (portable)" : "", module, getenv("BLUEWAKE_DISC"));
    bw_crash_install(g_data_dir);
    bluewake_settings_set_data_dir(g_data_dir);
    char* host_argv[3] = {argv[0], module, NULL};
    char launch_marker[4096];
    snprintf(launch_marker, sizeof launch_marker, "%slaunch.pending", g_data_dir);
    // The last launch never got as far as a clean exit: start in safe mode,
    // as Windows does (settings_menu.cpp's bluewake_settings_load).
    if (bw_launch_pending(launch_marker)) {
        fprintf(stderr, "[safe-mode] the last launch didn't finish; starting in safe mode\n");
        setenv("BLUEWAKE_SAFE_MODE", "1", 1);
    }
    if (!bw_launch_begin(launch_marker)) {
        fprintf(stderr, "BlueWake could not create its launch recovery marker in %s. Check that the folder "
                "can be created and written to%s.\n", g_data_dir,
                g_portable ? " (portable.txt beside bluewake puts it in the app folder; remove that file to "
                             "use ~/.local/share/BlueWake instead)" : "");
        return 1;
    }
    bw_crash_test();
    const int status = bluewake_host_main(2, host_argv);
    if (status == 0 && !bw_launch_clear(launch_marker))
        fprintf(stderr, "[safe-mode] could not clear launch marker on clean exit\n");
    if (status == 0 && bluewake_settings_restart_requested() && g_launch_env != NULL) {
        fprintf(stderr, "[settings] restarting with the new settings\n");
        restore_terminal_streams();
        // Not --safe-mode again: the restart is to leave it with the settings
        // chosen since.
        int kept = 0;
        for (int i = 0; i < argc; ++i)
            if (i == 0 || strcmp(argv[i], "--safe-mode") != 0)
                argv[kept++] = argv[i];
        argv[kept] = NULL;
        execve("/proc/self/exe", argv, g_launch_env);
        fprintf(stderr, "[settings] restart failed: %s\n", strerror(errno));
    }
    fflush(stdout);
    fflush(stderr);
    return status;
}
