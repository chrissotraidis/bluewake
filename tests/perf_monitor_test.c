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
#include <string.h>
#include <time.h>

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
#endif

    printf("perf_monitor: rate %.1f fps, frames %.2f/%.2f ms, cpu %.0f%% (%.0f%% this process), "
           "ram %.0f/%.0f MB (%.0f this process), gpu %.0f%% vram %.0f/%.0f MB %s\n",
           stats.fps, stats.frame_ms_avg, stats.frame_ms_max, stats.cpu_total_pct, stats.cpu_game_pct,
           stats.ram_used_mb, stats.ram_total_mb, stats.ram_game_mb, stats.gpu_pct, stats.vram_used_mb,
           stats.vram_total_mb, stats.cpu_name);
    printf("perf_monitor_test: ok\n");
    return 0;
}
