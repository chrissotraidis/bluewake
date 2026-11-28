// The performance panel's numbers without a game or a graphics device: the rate
// and the frame times of a made-up run of presents, and the machine's own
// counters (which every platform reports differently, or not at all).

#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L   // sysconf, clock_gettime, nanosleep
#endif

#include "perf_monitor.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>   // setenv, mkdtemp, for the prepared sysfs tree
#include <string.h>
#include <time.h>

#if defined(__linux__)
#include <sys/stat.h>   // mkdir, for the prepared sysfs tree below
#include <unistd.h>     // rmdir-free: the tree stays for the next run to overwrite
#endif

#if defined(__linux__)
static unsigned long long real_ns(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (unsigned long long)now.tv_sec * 1000000000ull + (unsigned long long)now.tv_nsec;
}
#endif

// Within a tenth of the expected value.
static int near(float value, float expected) { return fabsf(value - expected) < expected * 0.1f + 0.05f; }

int main(void) {
    // A rate from frames over a time, and the average of the frame times.
    assert(near(bw_perf_rate(30, 500000000ull), 60.f));
    assert(bw_perf_rate(30, 0ull) == 0.f);
    const float times[] = {10.f, 20.f, 30.f};
    assert(near(bw_perf_average(times, 3), 20.f));
    assert(bw_perf_average(times, 0) == 0.f);
    assert(bw_perf_average(NULL, 3) == 0.f);
    // A counter over a wall time, and the two cases that are not a reading.
    assert(near(bw_perf_percent(2000000, 1000000, 1000000000ull), 100.f));
    assert(bw_perf_percent(10, 250000, 1000000000ull) == 0.f); // it reset
    assert(bw_perf_percent(10, 5, 0ull) == 0.f);

    // The setting's values, and the ones that are none of them.
    assert(bw_perf_mode_of("off") == BW_PERF_OFF && bw_perf_mode_of("fps") == BW_PERF_FPS &&
           bw_perf_mode_of("full") == BW_PERF_FULL);
    assert(bw_perf_mode_of(NULL) == BW_PERF_OFF && bw_perf_mode_of("") == BW_PERF_OFF &&
           bw_perf_mode_of("nonsense") == BW_PERF_OFF);
    assert(strcmp(bw_perf_mode_value(BW_PERF_FULL), "full") == 0);
    assert(strcmp(bw_perf_mode_value(BW_PERF_FPS), "fps") == 0);
    assert(strcmp(bw_perf_mode_value(BW_PERF_OFF), "off") == 0);

    // A second of presents at 60: the rate, the average and the longest frame.
    bw_perf_reset();
    BwPerfStats stats;
    bw_perf_snapshot(&stats);
    assert(stats.fps == 0.f && stats.frame_ms_avg == 0.f);
    assert(stats.cpu_total_pct < 0.f && stats.ram_total_mb < 0.f && stats.gpu_pct < 0.f); // unknown, not zero
    unsigned long long now = 1000000000ull;
    for (int i = 0; i < 62; ++i) {
        bw_perf_present(now);
        now += 16666666ull; // 60 FPS
    }
    bw_perf_snapshot(&stats);
    assert(near(stats.fps, 60.f));
    assert(near(stats.frame_ms_avg, 16.666f));
    assert(near(stats.frame_ms_max, 16.666f));
    // The graph's history is oldest first and no longer than it can be.
    int kept = 0;
    while (kept < BW_PERF_HISTORY && stats.frame_ms[kept] > 0.f)
        ++kept;
    assert(kept == 61);   // the first present only starts the clock

    // A stall is not a frame time: five seconds of nothing, then 60 FPS again.
    now += 5000000000ull;
    bw_perf_present(now);
    stats.frame_ms[0] = 0.f;
    bw_perf_snapshot(&stats);
    assert(stats.fps > 0.f); // the last good half second stands
    for (int i = 0; i < 20; ++i) {
        now += 16666666ull;
        bw_perf_present(now);
    }
    bw_perf_snapshot(&stats);
    assert(near(stats.fps, 60.f)); // the stall did not drag it down

    // Which card the panel reports (#535): memory of its own first, then the first card
    // that publishes the counters at all — an APU's VRAM is a carve-out that reads 0, and
    // its busy counter reads 0 while the GPU is idle, so neither may rule it out.
    {
        const BwPerfCardProbe apu_only[] = {
            {.has_vram_total = true, .has_busy = true, .vram_total = 0ull, .busy = 0ull}};
        assert(bw_perf_card_choice(apu_only, 1) == 0);
        const BwPerfCardProbe apu_and_card[] = {
            {.has_vram_total = true, .has_busy = true, .vram_total = 0ull, .busy = 0ull},
            {.has_vram_total = true, .has_busy = true, .vram_total = 8589934592ull, .busy = 3ull}};
        assert(bw_perf_card_choice(apu_and_card, 2) == 1);   // the one with memory of its own
        const BwPerfCardProbe bare_then_apu[] = {
            {.has_vram_total = false, .has_busy = false, .vram_total = 0ull, .busy = 0ull},
            {.has_vram_total = true, .has_busy = true, .vram_total = 0ull, .busy = 0ull}};
        assert(bw_perf_card_choice(bare_then_apu, 2) == 1);  // a card with no counters is not one
        const BwPerfCardProbe none[] = {
            {.has_vram_total = false, .has_busy = false, .vram_total = 0ull, .busy = 0ull},
            {.has_vram_total = false, .has_busy = false, .vram_total = 0ull, .busy = 0ull}};
        assert(bw_perf_card_choice(none, 2) == -1);
        assert(bw_perf_card_choice(NULL, 3) == -1);
        assert(bw_perf_card_choice(none, 0) == -1);
    }

#if defined(__linux__)
    // The machine, where the platform reports it: sampled twice a second of the
    // real clock (a made-up timeline moves it faster than the counters move).
    bw_perf_reset();
    for (int i = 0; i < 140; ++i) {
        bw_perf_present(real_ns());
        // Some work of its own, so the process's processor counters move: a
        // process that used no time is a "-" too, not a 0%.
        volatile unsigned long long spin = 0ull;
        for (int k = 0; k < 300000; ++k)
            spin += (unsigned long long)k;
        (void)spin;
        struct timespec wait = {0, 4000000};
        nanosleep(&wait, NULL);
    }
    bw_perf_snapshot(&stats);
    assert(stats.ram_total_mb > 0.f && stats.ram_used_mb > 0.f && stats.ram_used_mb <= stats.ram_total_mb);
    assert(stats.ram_game_mb > 0.f && stats.ram_game_mb < stats.ram_total_mb);
    assert(stats.cpu_total_pct >= 0.f && stats.cpu_total_pct <= 100.f);
    assert(stats.cpu_game_pct > 0.f && stats.cpu_game_pct <= 100.f);
    assert(stats.cpu_name[0] != '\0');
    assert(stats.cpu_temp_c < -0.5f || (stats.cpu_temp_c > 0.f && stats.cpu_temp_c < 150.f));
    // A card whose driver reports no counters keeps them at "-", never at 0.
    assert(stats.gpu_pct < 0.f || (stats.gpu_pct >= 0.f && stats.gpu_pct <= 100.f));
    assert(stats.vram_total_mb < 0.f || stats.vram_used_mb <= stats.vram_total_mb);

    // The card's row from a prepared sysfs tree, because no CI machine is an APU: an APU's
    // VRAM is a carve-out that reads 0 and its memory is the machine's (GTT, shown shared),
    // and a card with memory of its own reports vram and its own load (#535).
    char root[] = "/tmp/bw_perf_drm_XXXXXX";
    assert(mkdtemp(root) != NULL);
    char dir[256];
    snprintf(dir, sizeof dir, "%s/card0", root);
    assert(mkdir(dir, 0755) == 0);
    snprintf(dir, sizeof dir, "%s/card0/device", root);
    assert(mkdir(dir, 0755) == 0);
    const struct { const char* name; const char* value; } apu[] = {
        {"mem_info_vram_total", "0"},          {"mem_info_vram_used", "0"},
        {"mem_info_gtt_total", "17179869184"}, {"mem_info_gtt_used", "1073741824"},
        {"gpu_busy_percent", "0"},
    };
    for (size_t i = 0; i < sizeof apu / sizeof apu[0]; ++i) {
        char path[320];
        snprintf(path, sizeof path, "%s/card0/device/%s", root, apu[i].name);
        FILE* file = fopen(path, "w");
        assert(file != NULL);
        fputs(apu[i].value, file);
        fclose(file);
    }
    assert(setenv("BLUEWAKE_DRM_ROOT", root, 1) == 0);
    bw_perf_reset();
    unsigned long long card_now = 1000000000ull;
    for (int i = 0; i < 62; ++i) {
        bw_perf_present(card_now);
        card_now += 16666666ull;
    }
    bw_perf_snapshot(&stats);
    assert(stats.gpu_pct == 0.f);              // idle is a reading, not an unknown
    assert(stats.vram_shared);                 // the machine's memory, not the card's own
    assert(near(stats.vram_total_mb, 16384.f));
    assert(near(stats.vram_used_mb, 1024.f));

    // The same tree with memory of its own: vram, and the load it reports.
    const struct { const char* name; const char* value; } card[] = {
        {"mem_info_vram_total", "8589934592"}, {"mem_info_vram_used", "1073741824"},
        {"mem_info_gtt_total", "17179869184"}, {"mem_info_gtt_used", "0"},
        {"gpu_busy_percent", "42"},
    };
    for (size_t i = 0; i < sizeof card / sizeof card[0]; ++i) {
        char path[320];
        snprintf(path, sizeof path, "%s/card0/device/%s", root, card[i].name);
        FILE* file = fopen(path, "w");
        assert(file != NULL);
        fputs(card[i].value, file);
        fclose(file);
    }
    bw_perf_reset();
    card_now = 2000000000ull;
    for (int i = 0; i < 62; ++i) {
        bw_perf_present(card_now);
        card_now += 16666666ull;
    }
    bw_perf_snapshot(&stats);
    assert(!stats.vram_shared);
    assert(near(stats.vram_total_mb, 8192.f));
    assert(near(stats.vram_used_mb, 1024.f));
    assert(near(stats.gpu_pct, 42.f));
    unsetenv("BLUEWAKE_DRM_ROOT");
    bw_perf_reset();
#endif

    printf("perf_monitor: rate %.1f fps, frames %.2f/%.2f ms, cpu %.0f%% (%.0f%% this process), "
           "ram %.0f/%.0f MB (%.0f this process), gpu %.0f%% vram %.0f/%.0f MB %s\n",
           stats.fps, stats.frame_ms_avg, stats.frame_ms_max, stats.cpu_total_pct, stats.cpu_game_pct,
           stats.ram_used_mb, stats.ram_total_mb, stats.ram_game_mb, stats.gpu_pct, stats.vram_used_mb,
           stats.vram_total_mb, stats.cpu_name);
    printf("perf_monitor_test: ok\n");
    return 0;
}
