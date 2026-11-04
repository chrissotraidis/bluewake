// SPDX-License-Identifier: GPL-3.0-or-later
// The numbers a performance overlay shows (perf_monitor.c): the frame rate and
// frame times of the presents, and the machine's CPU, memory and graphics card
// load, temperatures, power and clocks. A value the machine does not give is
// negative, and the overlay prints "-" for it.
//
// The frame numbers come from the presents themselves (bw_perf_present, once per
// drawn frame); the machine is sampled twice a second from there, and only while
// the overlay is on, so leaving it off costs one comparison per frame.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define BW_PERF_HISTORY 120 // frame times kept for the graph

// Off, the frame rate alone (Aurora draws that one), or the full panel.
enum { BW_PERF_OFF = 0, BW_PERF_FPS = 1, BW_PERF_FULL = 2 };

typedef struct {
    // The presents of the last half second, and the frame times kept for the graph (oldest first).
    float fps, frame_ms_avg, frame_ms_max;
    float frame_ms[BW_PERF_HISTORY];
    // The machine, sampled twice a second while the overlay is on.
    char cpu_name[64], gpu_name[96];
    float cpu_total_pct, cpu_game_pct, cpu_temp_c;
    // The mean clock of the cores and the fastest a core can go: what a platform that will not say how busy it
    // is (a phone: /proc/stat is denied to apps) does say about the same thing.
    float cpu_clock_mhz, cpu_clock_max_mhz;
    float ram_used_mb, ram_total_mb, ram_game_mb;
    float gpu_pct, gpu_temp_c, gpu_power_w, gpu_clock_mhz, vram_used_mb, vram_total_mb;
} BwPerfStats;

// A stored setting ("off", "fps", "full") as the mode, and the mode as it is stored.
int bw_perf_mode_of(const char* value);
const char* bw_perf_mode_value(int mode);

// Start again: no frame time from before, and the machine sampled at once.
void bw_perf_reset(void);
// A frame was drawn (the overlay's own clock).
void bw_perf_present(unsigned long long now_ns);
// The latest numbers.
void bw_perf_snapshot(BwPerfStats* out);

// Pure, shared with the test: a rate from frames seen over a time, the average
// of the frame times, and a percentage from two readings of a counter.
float bw_perf_rate(unsigned long long frames, unsigned long long ns);
float bw_perf_average(const float* values, int count);
float bw_perf_percent(unsigned long long current, unsigned long long previous, unsigned long long wall_ns);

#ifdef __cplusplus
}
#endif
