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
// Every default is only a default: an environment variable that is already
// set (BLUEWAKE_DISC, BLUEWAKE_CARD_PATH, ...) wins.
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <SDL3/SDL.h>
#include <aurora/aurora.h>

#include "linux_disc.h"
#include "linux_crash.h"
#include "launch_marker.h"

int bluewake_host_main(int argc, char** argv);

static char g_exe_dir[4096];
static char g_data_dir[4096];
static char g_log_path[4096];

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
    // Player data: $XDG_DATA_HOME/BlueWake, else ~/.local/share/BlueWake.
    const char* override = getenv("BLUEWAKE_DATA_DIR");
    const char* data_home = getenv("XDG_DATA_HOME");
    const char* home = getenv("HOME");
    if (override != NULL && override[0] != '\0') {
        snprintf(g_data_dir, sizeof g_data_dir, "%s/", override);
    } else if (data_home != NULL && data_home[0] != '\0') {
        snprintf(g_data_dir, sizeof g_data_dir, "%s/BlueWake/", data_home);
    } else if (home != NULL && home[0] != '\0') {
        snprintf(g_data_dir, sizeof g_data_dir, "%s/.local/share/BlueWake/", home);
    } else {
        snprintf(g_data_dir, sizeof g_data_dir, "%suser/", g_exe_dir);
    }
    mkdir(g_data_dir, 0755);
    char logs[4096];
    snprintf(logs, sizeof logs, "%slogs", g_data_dir);
    mkdir(logs, 0755);
    char states[4096];
    snprintf(states, sizeof states, "%sstates", g_data_dir);
    mkdir(states, 0755);
}

// --- session log -----------------------------------------------------------
// Everything the host writes to stdout and stderr also goes to
// ~/.local/share/BlueWake/logs/session-YYYYMMDD-HHMMSS.log, each line stamped
// with the local time, and still to the terminal BlueWake was started from.
static pthread_t g_log_thread;
static int g_log_pipe[2];
static FILE* g_log_file;
static int g_term_out = -1;

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
    (void)err_fd;
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
            "usage: bluewake [options]\n"
            "  --widescreen       16:9 (the widescreen mod: a wider camera and HUD)\n"
            "  --aspect A         4:3 (the game's own), 16:10 or 16:9\n"
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
            "  --safe-mode        Recover startup with HLE audio and mods off\n"
            "  --hle-audio        Fast audio for this session\n"
            "  --lle-audio        run the DSP's own microcode instead of the HLE ucode\n"
            "  --mods LIST        mods compiled into the module, by name\n"
            "  --disc FILE        the disc image to read (default game/GZLE01.iso)\n"
            "  --module FILE      the translated game module (default gGZLE01_recomp.so)\n"
            "Keyboard: arrows D-pad, J A, K B, U X, I Y, W/A/S/D stick,\n"
            "H/F/T/G C-stick, E/R L/R, Q Z, Return START. Game controllers work too.\n"
            "Mouse: click the game, then move it to turn the camera; Esc releases it.\n"
            "F1 or Esc settings, F11 fullscreen, F10 Smooth Motion, F9 frame rate.\n"
            "The settings menu saves to ~/.config/BlueWake/settings.ini; options given\n"
            "here win for the session.\n");
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) { usage(); return 0; }
    resolve_dirs();
    if (getenv("BLUEWAKE_SESSION_LOG") == NULL || strcmp(getenv("BLUEWAKE_SESSION_LOG"), "0") != 0)
        start_session_log();

    const char* module_arg = NULL;
    char mods[256] = "";
    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];
        const int more = i + 1 < argc;
        if (strcmp(a, "--disc") == 0 && more) {
            setenv("BLUEWAKE_DISC", argv[++i], 1);
        } else if (strcmp(a, "--module") == 0 && more) {
            module_arg = argv[++i];
        } else if (strcmp(a, "--mods") == 0 && more) {
            snprintf(mods + strlen(mods), sizeof mods - strlen(mods), "%s%s", mods[0] ? "," : "", argv[++i]);
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

    // The play configuration as the iOS/Windows apps: one perf line a second,
    // wall-clock pacing, Dolphin's HLE Zelda ucode, the real clock, and the
    // console's SRAM kept with the saves.
    bw_default("BLUEWAKE_PERF_LOG", "1");
    bw_default("BLUEWAKE_WALL_PACE", "1");
    bw_default("DOL_AUDIO_NO_THROTTLE", "1");
    bw_default("BLUEWAKE_RENDERER", "aurora");
    bw_default("BLUEWAKE_CYCLE_CAP", "16384");
    bw_default("BLUEWAKE_MAX_BLOCKS", "100000000000");
    bw_default("BLUEWAKE_DSP_MODE", "hle");
    bw_default("BLUEWAKE_CLOCK", "now");
    bw_default_path("BLUEWAKE_SRAM", g_data_dir, "sram.bin");
    bw_default_path("BLUEWAKE_CARD_PATH", g_data_dir, "GZLE01.card");
    bw_default("BLUEWAKE_STATE_DIR", g_data_dir);

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
    const int disc_status = bw_disc_setup(g_exe_dir, g_data_dir);
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

    fprintf(stderr, "[linux] app=%s data=%s module=%s disc=%s\n", g_exe_dir, g_data_dir, module,
            getenv("BLUEWAKE_DISC"));
    bw_crash_install(g_data_dir);
    SDL_SetAppMetadata("BlueWake", "0.1", "dev.bluewake.BlueWake");
    char* host_argv[3] = {argv[0], module, NULL};
    char launch_marker[4096];
    snprintf(launch_marker, sizeof launch_marker, "%slaunch.pending", g_data_dir);
    if (!bw_launch_begin(launch_marker)) {
        fprintf(stderr, "BlueWake could not create its launch recovery marker. Check that its data folder is writable.\n");
        return 1;
    }
    bw_crash_test();
    const int status = bluewake_host_main(2, host_argv);
    if (status == 0 && !bw_launch_clear(launch_marker))
        fprintf(stderr, "[safe-mode] could not clear launch marker on clean exit\n");
    fflush(stdout);
    fflush(stderr);
    return status;
}
