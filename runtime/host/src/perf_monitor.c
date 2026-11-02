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
#include <dlfcn.h>
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

// --- the card, as Vulkan names it --------------------------------------------
//
// The card's counters come from the driver's sysfs, but its name does not: sysfs
// knows the PCI ids, not "Radeon RX 6600", and the host does not link Vulkan while
// the renderer does. So the loader already in the process is asked at run time for
// the device's own name and its device-local memory — the same two facts the TotK
// port's overlay takes from its device. Nothing here is required: a machine with no
// loader, or one whose first device is not the renderer's, leaves the name empty
// and the panel's line reads "-".
#if defined(_WIN32) || !defined(__APPLE__)

#if defined(_WIN32)
#define BW_VK_LIBRARY "vulkan-1.dll"
static void* library_open(const char* name) { return (void*)LoadLibraryA(name); }
static void* library_symbol(void* library, const char* name) { return (void*)GetProcAddress((HMODULE)library, name); }
static void library_close(void* library) {
    if (library != NULL)
        FreeLibrary((HMODULE)library);
}
#else
#define BW_VK_LIBRARY "libvulkan.so.1"
static void* library_open(const char* name) { return dlopen(name, RTLD_NOW | RTLD_LOCAL); }
static void* library_symbol(void* library, const char* name) { return dlsym(library, name); }
static void library_close(void* library) {
    if (library != NULL)
        dlclose(library);
}
#endif

// Only the head of VkPhysicalDeviceProperties is read, and its fields up to the
// name are fixed; the rest is written by the driver into a buffer far larger than
// the structure. The layouts are checked against the real headers in
// tests/perf_monitor_vk_layout_test.c.
typedef struct {
    int sType;
    const void* pNext;
    const char* pApplicationName;
    unsigned applicationVersion;
    const char* pEngineName;
    unsigned engineVersion;
    unsigned apiVersion;
} BwVkApplicationInfo;

typedef struct {
    int sType;
    const void* pNext;
    unsigned flags;
    const void* pApplicationInfo;
    unsigned enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    unsigned enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
} BwVkInstanceCreateInfo;

typedef struct {
    unsigned apiVersion, driverVersion, vendorID, deviceID;
    int deviceType;
    char deviceName[256];
} BwVkDeviceHead;

typedef struct {
    unsigned flags, heapIndex;
} BwVkMemoryType;
typedef struct {
    unsigned long long size;
    unsigned flags;
} BwVkMemoryHeap;
typedef struct {
    unsigned memoryTypeCount, memoryHeapCount;
    BwVkMemoryType memoryTypes[32];
    BwVkMemoryHeap memoryHeaps[16];
} BwVkMemoryProperties;

enum {
    BW_VK_APPLICATION_INFO = 0,
    BW_VK_INSTANCE_CREATE_INFO = 1,
    BW_VK_DEVICE_LOCAL_HEAP = 1u,
    BW_VK_API_1_0 = 1u << 22,
};

// dlsym/GetProcAddress hand back an object pointer and C says a function pointer is
// not one; POSIX and Windows both guarantee the conversion, and going through
// memcpy keeps the cast ISO C forbids out of the source. Like the renderer's own
// loader does.
static void library_function(void* target, size_t size, void* symbol) { memcpy(target, &symbol, size); }

// The device's name into name, and its device-local memory into vram_mb (left
// alone when the driver reports none). 1 when a device answered.
static int vulkan_device(char* name, size_t size, float* vram_mb) {
    typedef int (*CreateInstance)(const BwVkInstanceCreateInfo*, const void*, void**);
    typedef void (*DestroyInstance)(void*, const void*);
    typedef int (*EnumerateDevices)(void*, unsigned*, void**);
    typedef void (*DeviceProperties)(void*, void*);
    typedef void (*DeviceMemory)(void*, BwVkMemoryProperties*);
    void* library = library_open(BW_VK_LIBRARY);
    if (library == NULL)
        return 0;
    CreateInstance create = NULL;
    DestroyInstance destroy = NULL;
    EnumerateDevices enumerate = NULL;
    DeviceProperties properties = NULL;
    DeviceMemory memory = NULL;
    library_function(&create, sizeof create, library_symbol(library, "vkCreateInstance"));
    library_function(&destroy, sizeof destroy, library_symbol(library, "vkDestroyInstance"));
    library_function(&enumerate, sizeof enumerate, library_symbol(library, "vkEnumeratePhysicalDevices"));
    library_function(&properties, sizeof properties, library_symbol(library, "vkGetPhysicalDeviceProperties"));
    library_function(&memory, sizeof memory, library_symbol(library, "vkGetPhysicalDeviceMemoryProperties"));
    int found = 0;
    void* instance = NULL;
    if (create != NULL && destroy != NULL && enumerate != NULL && properties != NULL && memory != NULL) {
        BwVkApplicationInfo app = {BW_VK_APPLICATION_INFO, NULL, "BlueWake", 1u, "BlueWake", 1u, BW_VK_API_1_0};
        BwVkInstanceCreateInfo info = {BW_VK_INSTANCE_CREATE_INFO, NULL, 0u, &app, 0u, NULL, 0u, NULL};
        if (create(&info, NULL, &instance) == 0 && instance != NULL) {
            unsigned count = 0;
            if (enumerate(instance, &count, NULL) == 0 && count > 0u) {
                void* devices[8];
                if (count > 8u)
                    count = 8u;
                if (enumerate(instance, &count, devices) == 0) {
                    unsigned char raw[4096];
                    memset(raw, 0, sizeof raw);
                    properties(devices[0], raw);
                    const BwVkDeviceHead* head = (const BwVkDeviceHead*)raw;
                    snprintf(name, size, "%.*s", (int)size - 1, head->deviceName);
                    BwVkMemoryProperties heaps;
                    memset(&heaps, 0, sizeof heaps);
                    memory(devices[0], &heaps);
                    unsigned long long total = 0ull;
                    for (unsigned i = 0u; i < heaps.memoryHeapCount && i < 16u; ++i)
                        if ((heaps.memoryHeaps[i].flags & BW_VK_DEVICE_LOCAL_HEAP) != 0u)
                            total += heaps.memoryHeaps[i].size;
                    if (total != 0ull && vram_mb != NULL)
                        *vram_mb = (float)((double)total / 1048576.0);
                    found = name[0] != '\0';
                }
            }
            destroy(instance, NULL);
        }
    }
    library_close(library);
    return found;
}
#endif

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
        // The card's name is Vulkan's to give (sysfs knows only the PCI ids), and its
        // device-local memory comes with it: what sysfs reports for that is used first
        // when the loop below finds it.
        float device_local_mb = -1.f;
        if (vulkan_device(s->gpu_name, sizeof s->gpu_name, &device_local_mb) && device_local_mb > 0.f)
            s->vram_total_mb = device_local_mb;
    }
    static const char* const kCpuSensors[] = {"k10temp", "coretemp", "zenpower", "cpu_thermal", "acpitz"};
    if (cpu_sensor_dir[0] == '\0')
        find_hwmon("/sys/class/hwmon", kCpuSensors, (int)(sizeof kCpuSensors / sizeof kCpuSensors[0]),
                   cpu_sensor_dir, sizeof cpu_sensor_dir);
    if (cpu_sensor_dir[0] != '\0' && s->cpu_temp_c < 0.f) {
        const unsigned long long millidegrees = read_number(cpu_sensor_dir, "temp1_input");
        // A sensor reading 0 or 150 C is a sensor that is not reading: the panel
        // says "-" instead of a temperature nobody has.
        if (millidegrees > 1000ull && millidegrees < 150000ull)
            s->cpu_temp_c = (float)((double)millidegrees / 1000.0);
        fprintf(stderr, "[perf] panel cpu sensor: %s temp1_input=%llu (%s)\n", cpu_sensor_dir, millidegrees,
                s->cpu_temp_c < 0.f ? "not a reading, left as -" : "used");
    }

    // The card the game draws on: the first that reports its memory (amdgpu's
    // counters below /sys/class/drm/cardN/device, its temperature and power in
    // the hwmon beside them). A driver that reports none leaves the numbers at
    // "-", and the load stays unknown.
    static int card_logged;
    for (int card = 0; card < 10 && !card_logged; ++card) {
        char base[96];
        snprintf(base, sizeof base, "/sys/class/drm/card%d/device", card);
        const unsigned long long vram_total = read_number(base, "mem_info_vram_total");
        const unsigned long long vram_used = read_number(base, "mem_info_vram_used");
        const unsigned long long busy = read_number(base, "gpu_busy_percent");
        if (vram_total == 0ull && vram_used == 0ull && busy == 0ull)
            continue;   // no counters here: not the card that draws
        if (vram_total != 0ull)
            s->vram_total_mb = (float)((double)vram_total / 1048576.0);
        if (vram_used != 0ull)
            s->vram_used_mb = (float)((double)vram_used / 1048576.0);
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
            // The clock is in Hz (amdgpu's freq1_input), so a bare 43 MHz idle
            // reads as 43000000 — and 699000000 is 699 MHz, not 699 GHz. Anything
            // outside 1-10 GHz is not a clock the panel can show.
            const unsigned long long hz = read_number(hwmon, "freq1_input");
            if (hz >= 1000000ull && hz <= 10000000000ull)
                s->gpu_clock_mhz = (float)((double)hz / 1e6);
        }
        card_logged = 1;
        fprintf(stderr,
                "[perf] panel card: \"%s\" (vulkan), vram %.0f/%.0f MB; %s: busy %.0f%%, temp %.0f C, "
                "power %.0f W, clock %.0f MHz (from freq1_input)\n",
                s->gpu_name[0] != '\0' ? s->gpu_name : "?", s->vram_used_mb, s->vram_total_mb, base, s->gpu_pct,
                s->gpu_temp_c, s->gpu_power_w, s->gpu_clock_mhz);
    }
    if (!card_logged) {
        card_logged = 1;
        fprintf(stderr, "[perf] panel card: no /sys/class/drm/card*/device reports counters (gpu name from "
                        "vulkan: \"%s\", vram total %.0f MB)\n",
                s->gpu_name[0] != '\0' ? s->gpu_name : "?", s->vram_total_mb);
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
