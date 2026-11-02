// SPDX-License-Identifier: GPL-3.0-or-later
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L   // sysconf, clock_gettime, nanosleep
#endif

#include "perf_monitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <unistd.h>
#else
#include <unistd.h>
#endif

// Twice a second, as the panel's numbers are read at a glance; a longer gap
// between two frames is a stall (a load, the window dragged), not a frame time.
#define SAMPLE_NS 500000000ull
#define STALL_NS 2000000000ull

static struct {
    unsigned long long last_ns;    // the frame before, 0 before the first
    unsigned long long window_ns;  // when the rate's half second began
    unsigned long long window_frames;
    float window_max_ms;
    int count;                     // frame times kept, oldest first
    unsigned long long sampled_ns;
    BwPerfStats stats;
} g;

float bw_perf_rate(unsigned long long frames, unsigned long long ns) {
    return ns == 0ull ? 0.f : (float)((double)frames * 1e9 / (double)ns);
}

float bw_perf_average(const float* values, int count) {
    if (values == NULL || count <= 0)
        return 0.f;
    double sum = 0.0;
    for (int i = 0; i < count; ++i)
        sum += (double)values[i];
    return (float)(sum / (double)count);
}

// A counter read in microseconds, over a wall time in nanoseconds: a share of
// one core, as the frame-time diagnostics already report the host's threads.
float bw_perf_percent(unsigned long long current, unsigned long long previous, unsigned long long wall_ns) {
    if (wall_ns == 0ull || current < previous)
        return 0.f;
    return (float)(100.0 * (double)(current - previous) * 1000.0 / (double)wall_ns);
}

int bw_perf_mode_of(const char* value) {
    if (value == NULL)
        return BW_PERF_OFF;
    if (strcmp(value, "full") == 0 || strcmp(value, "2") == 0)
        return BW_PERF_FULL;
    if (strcmp(value, "fps") == 0 || strcmp(value, "1") == 0)
        return BW_PERF_FPS;
    return BW_PERF_OFF;
}

const char* bw_perf_mode_value(int mode) {
    return mode == BW_PERF_FULL ? "full" : mode == BW_PERF_FPS ? "fps" : "off";
}

void bw_perf_snapshot(BwPerfStats* out) {
    if (out != NULL)
        *out = g.stats;
}

// The machine's counters as they were at the last read, for the deltas that
// make a load a share rather than a total since the machine came up.
static struct {
    unsigned long long at_ns, cpu_total, cpu_busy, cpu_idle, cpu_process_us;
} machine;

static void sample_machine(unsigned long long now_ns);

// The parts' names and the sensor directory, read once: they do not change
// while the game runs (a reset reads them again).
static int named;
static char cpu_sensor_dir[192];

void bw_perf_reset(void) {
    named = 0;
    cpu_sensor_dir[0] = '\0';
    memset(&g, 0, sizeof g);
    memset(&machine, 0, sizeof machine);
    g.stats.cpu_total_pct = g.stats.cpu_game_pct = g.stats.cpu_temp_c = -1.f;
    g.stats.ram_used_mb = g.stats.ram_total_mb = g.stats.ram_game_mb = -1.f;
    g.stats.gpu_pct = g.stats.gpu_temp_c = g.stats.gpu_power_w = g.stats.gpu_clock_mhz = -1.f;
    g.stats.vram_used_mb = g.stats.vram_total_mb = -1.f;
}

void bw_perf_present(unsigned long long now_ns) {
    if (g.last_ns == 0ull) {      // the first frame: the rate starts at the next one
        g.last_ns = now_ns;
        g.window_ns = now_ns;
        sample_machine(now_ns);
        return;
    }
    const unsigned long long dt = now_ns - g.last_ns;
    g.last_ns = now_ns;
    if (dt > STALL_NS) {          // a stall, counted in neither the rate nor the graph
        g.window_ns = now_ns;
        g.window_frames = 0ull;
        g.window_max_ms = 0.f;
        return;
    }
    const float ms = (float)((double)dt / 1e6);
    if (g.count < BW_PERF_HISTORY) {
        g.stats.frame_ms[g.count++] = ms;
    } else {                      // oldest first: the graph reads it as it is
        memmove(g.stats.frame_ms, g.stats.frame_ms + 1, sizeof g.stats.frame_ms - sizeof g.stats.frame_ms[0]);
        g.stats.frame_ms[BW_PERF_HISTORY - 1] = ms;
    }
    ++g.window_frames;
    if (ms > g.window_max_ms)
        g.window_max_ms = ms;
    const unsigned long long span = now_ns - g.window_ns;
    if (span >= SAMPLE_NS) {
        g.stats.fps = bw_perf_rate(g.window_frames, span);
        g.stats.frame_ms_avg = bw_perf_average(g.stats.frame_ms, g.count);
        g.stats.frame_ms_max = g.window_max_ms;
        g.window_ns = now_ns;
        g.window_frames = 0ull;
        g.window_max_ms = 0.f;
    }
    if (now_ns - machine.at_ns >= SAMPLE_NS)
        sample_machine(now_ns);
}

// --- the machine, as each system reports it ----------------------------------

#if defined(__linux__)

static int read_line(const char* path, char* out, size_t size) {
    FILE* f = fopen(path, "r");
    if (f == NULL)
        return 0;
    const int ok = fgets(out, (int)size, f) != NULL;
    fclose(f);
    if (ok) {
        char* end = strpbrk(out, "\r\n");
        if (end != NULL)
            *end = '\0';
    }
    return ok;
}

// A number in a file of one, as sysfs keeps them: 0 when it is not there.
static unsigned long long read_number(const char* dir, const char* name) {
    char path[224], line[64];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    return read_line(path, line, sizeof line) ? strtoull(line, NULL, 10) : 0ull;
}

// The hwmon directory called one of these (k10temp, amdgpu...), so the
// temperature, power and clock beside it are that part's.
static int find_hwmon(const char* root, const char* const* names, int count, char* out, size_t size) {
    for (int i = 0; i < 8; ++i) {
        char dir[192], path[224], name[64];
        snprintf(dir, sizeof dir, "%s/hwmon%d", root, i);
        snprintf(path, sizeof path, "%s/name", dir);
        if (!read_line(path, name, sizeof name))
            continue;
        for (int w = 0; w < count; ++w)
            if (strcmp(name, names[w]) == 0) {
                snprintf(out, size, "%s", dir);
                return 1;
            }
    }
    return 0;
}

static void read_cpu_name(char* out, size_t size) {
    FILE* f = fopen("/proc/cpuinfo", "r");
    if (f == NULL)
        return;
    char line[256];
    while (fgets(line, sizeof line, f) != NULL) {
        if (strncmp(line, "model name", 10) != 0)
            continue;
        const char* colon = strchr(line, ':');
        const char* name = colon != NULL ? colon + 1 : line + 10;
        while (*name == ' ' || *name == '\t')
            ++name;
        snprintf(out, size, "%.*s", (int)size - 1, name);
        char* end = out + strlen(out);
        while (end > out && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' '))
            --end;
        *end = '\0';
        break;
    }
    fclose(f);
}

// This process's own work, in microseconds, from /proc/self/stat (utime and
// stime are the 12th and 13th after the command's name, which may hold spaces).
static unsigned long long process_cpu_us(void) {
    char line[512];
    if (!read_line("/proc/self/stat", line, sizeof line))
        return 0ull;
    const char* p = strrchr(line, ')');
    if (p == NULL)
        return 0ull;
    ++p;
    unsigned long long field = 3ull, utime = 0ull, stime = 0ull;
    char* at = (char*)p;
    while (*at != '\0' && field < 15ull) {
        while (*at == ' ')
            ++at;
        if (field == 14ull)
            utime = strtoull(at, NULL, 10);
        else if (field == 15ull)
            stime = strtoull(at, NULL, 10);
        while (*at != ' ' && *at != '\0')
            ++at;
        ++field;
    }
    const long hz = sysconf(_SC_CLK_TCK);
    return hz > 0 ? (utime + stime) * 1000000ull / (unsigned long long)hz : 0ull;
}

static void sample_machine(unsigned long long now_ns) {
    BwPerfStats* s = &g.stats;
    // The whole machine: busy over the time since the last read.
    unsigned long long total = 0ull, idle = 0ull;
    FILE* f = fopen("/proc/stat", "r");
    if (f != NULL) {
        char line[256];
        if (fgets(line, sizeof line, f) != NULL && strncmp(line, "cpu ", 4) == 0) {
            unsigned long long v[8] = {0};
            const int n = sscanf(line + 4, "%llu %llu %llu %llu %llu %llu %llu %llu",
                                 &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7]);
            for (int i = 0; i < n && i < 8; ++i)
                total += v[i];
            idle = v[3] + (n > 4 ? v[4] : 0ull);   // idle and iowait
        }
        fclose(f);
    }
    const unsigned long long busy = total - idle;
    const unsigned long long wall = machine.at_ns != 0ull ? now_ns - machine.at_ns : 0ull;
    if (machine.at_ns != 0ull && total > machine.cpu_total) {
        const unsigned long long d_total = total - machine.cpu_total, d_busy = busy - machine.cpu_busy;
        s->cpu_total_pct = d_total != 0ull ? (float)(100.0 * (double)d_busy / (double)d_total) : 0.f;
    }
    machine.cpu_total = total;
    machine.cpu_busy = busy;
    const unsigned long long process_us = process_cpu_us();
    if (machine.at_ns != 0ull && wall != 0ull)
        s->cpu_game_pct = bw_perf_percent(process_us, machine.cpu_process_us, wall);
    machine.cpu_process_us = process_us;
    machine.at_ns = now_ns;

    // Memory: what the machine has free to give, and what this process holds.
    FILE* mem = fopen("/proc/meminfo", "r");
    if (mem != NULL) {
        char key[32], unit[8];
        unsigned long long value = 0ull, total_kb = 0ull, available_kb = 0ull;
        while (fscanf(mem, "%31s %llu %7s", key, &value, unit) == 3) {
            if (strcmp(key, "MemTotal:") == 0)
                total_kb = value;
            else if (strcmp(key, "MemAvailable:") == 0)
                available_kb = value;
            if (total_kb != 0ull && available_kb != 0ull)
                break;
        }
        fclose(mem);
        if (total_kb != 0ull) {
            s->ram_total_mb = (float)total_kb / 1024.f;
            s->ram_used_mb = (float)(total_kb - available_kb) / 1024.f;
        }
    }
    char statm[128];
    if (read_line("/proc/self/statm", statm, sizeof statm)) {
        unsigned long long pages_all = 0ull, pages_resident = 0ull;
        const long page = sysconf(_SC_PAGESIZE);
        if (sscanf(statm, "%llu %llu", &pages_all, &pages_resident) == 2 && page > 0)
            s->ram_game_mb = (float)((double)pages_resident * (double)page / 1048576.0);
    }

    // The parts' names and temperatures, once: they do not change while the game runs.
    if (!named) {
        named = 1;
        read_cpu_name(s->cpu_name, sizeof s->cpu_name);
        snprintf(s->gpu_name, sizeof s->gpu_name, "%s", "");
    }
    static const char* const kCpuSensors[] = {"k10temp", "coretemp", "zenpower", "cpu_thermal", "acpitz"};
    if (cpu_sensor_dir[0] == '\0')
        find_hwmon("/sys/class/hwmon", kCpuSensors, (int)(sizeof kCpuSensors / sizeof kCpuSensors[0]),
                   cpu_sensor_dir, sizeof cpu_sensor_dir);
    if (cpu_sensor_dir[0] != '\0' && s->cpu_temp_c < 0.f) {
        const unsigned long long millidegrees = read_number(cpu_sensor_dir, "temp1_input");
        if (millidegrees != 0ull)
            s->cpu_temp_c = (float)((double)millidegrees / 1000.0);
    }

    // The card the game draws on: the first that reports its memory (amdgpu's
    // counters below /sys/class/drm/cardN/device, its temperature and power in
    // the hwmon beside them). A driver that reports none leaves the numbers at
    // "-", and the load stays unknown.
    for (int card = 0; card < 4 && s->vram_total_mb < 0.f; ++card) {
        char base[96];
        snprintf(base, sizeof base, "/sys/class/drm/card%d/device", card);
        const unsigned long long vram_total = read_number(base, "mem_info_vram_total");
        if (vram_total == 0ull)
            continue;
        const unsigned long long vram_used = read_number(base, "mem_info_vram_used");
        s->vram_total_mb = (float)((double)vram_total / 1048576.0);
        s->vram_used_mb = (float)((double)vram_used / 1048576.0);
        const unsigned long long busy = read_number(base, "gpu_busy_percent");
        s->gpu_pct = (float)busy;
        char hwmon_root[128], hwmon[192];
        snprintf(hwmon_root, sizeof hwmon_root, "%s/hwmon", base);
        static const char* const kGpuSensors[] = {"amdgpu", "radeon"};
        if (find_hwmon(hwmon_root, kGpuSensors, (int)(sizeof kGpuSensors / sizeof kGpuSensors[0]), hwmon, sizeof hwmon)) {
            const unsigned long long millidegrees = read_number(hwmon, "temp1_input");
            if (millidegrees != 0ull)
                s->gpu_temp_c = (float)((double)millidegrees / 1000.0);
            const unsigned long long microwatts = read_number(hwmon, "power1_average");
            if (microwatts != 0ull)
                s->gpu_power_w = (float)((double)microwatts / 1e6);
            const unsigned long long mhz = read_number(hwmon, "freq1_input");
            if (mhz != 0ull)
                s->gpu_clock_mhz = (float)mhz;
        }
    }
}

#elif defined(_WIN32)
// Windows: the system's own counters for the processor and the memory, and this
// process's share of the processor. The card's counters need NVML or ADL, which
// this does not read yet, so its numbers stay "-".
static unsigned long long filetime_to_u64(FILETIME time) {
    ULARGE_INTEGER value;
    value.LowPart = time.dwLowDateTime;
    value.HighPart = time.dwHighDateTime;
    return (unsigned long long)value.QuadPart;
}

static unsigned long long process_cpu_us(void) {
    FILETIME created, exited, kernel, user;
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
        return 0ull;
    return (filetime_to_u64(kernel) + filetime_to_u64(user)) / 10ull;   // 100 ns units
}

static void sample_machine(unsigned long long now_ns) {
    BwPerfStats* s = &g.stats;
    FILETIME idle_time, kernel_time, user_time;
    const unsigned long long wall = machine.at_ns != 0ull ? now_ns - machine.at_ns : 0ull;
    if (GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
        const unsigned long long idle = filetime_to_u64(idle_time);
        const unsigned long long kernel = filetime_to_u64(kernel_time);   // busy and idle together
        const unsigned long long total = kernel + filetime_to_u64(user_time);
        if (machine.at_ns != 0ull && total > machine.cpu_total) {
            const unsigned long long d_total = total - machine.cpu_total;
            const unsigned long long d_busy = (kernel - idle) - (machine.cpu_busy - machine.cpu_idle);
            s->cpu_total_pct = d_total != 0ull ? (float)(100.0 * (double)d_busy / (double)d_total) : 0.f;
        }
        machine.cpu_total = total;
        machine.cpu_busy = kernel;
        machine.cpu_idle = idle;
    }
    const unsigned long long process_us = process_cpu_us();
    if (machine.at_ns != 0ull && wall != 0ull)
        s->cpu_game_pct = bw_perf_percent(process_us, machine.cpu_process_us, wall);
    machine.cpu_process_us = process_us;
    MEMORYSTATUSEX status;
    memset(&status, 0, sizeof status);
    status.dwLength = sizeof status;
    if (GlobalMemoryStatusEx(&status)) {
        s->ram_total_mb = (float)((double)status.ullTotalPhys / 1048576.0);
        s->ram_used_mb = (float)((double)(status.ullTotalPhys - status.ullAvailPhys) / 1048576.0);
    }
    PROCESS_MEMORY_COUNTERS counters;
    if (K32GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof counters))
        s->ram_game_mb = (float)((double)counters.WorkingSetSize / 1048576.0);
    machine.at_ns = now_ns;
}

#else
// A Mac's counters are not read yet: the panel shows the rate and the frame
// times, and "-" for the machine.
static void sample_machine(unsigned long long now_ns) { (void)now_ns; }

#endif
