// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "core/types.h"
#include <assert.h>
#include <stdio.h>

typedef struct { u32 pc, lr, exception, gpr[32]; } CPUState;
typedef struct { int unused; } StaticRecompModuleDesc;
struct HostStateLoop;
static u64 g_host_retrace_count;
static struct { u32 retrace, scene; } guest;
static unsigned saves, loads, directories, file_version;
static char status[64], saved_path[1100];
static CPUState saved_cpu;

static u32 mem_read32(CPUState* cpu, u32 address);
static void guest_read_cstr(CPUState* cpu, u32 address, char* out, size_t size);
static int dvd_path_to_entrynum(const char* path);
static bool dol_hle_callback_idle(void) { return true; }
static bool bluewake_quick_doors_busy(void) { return false; }
static const char* host_state_dir(void) { return "/synthetic/states"; }
static bool host_state_latest(char* out, size_t size) { (void)out; (void)size; return false; }
static bool host_state_save(const char*, CPUState*, const StaticRecompModuleDesc*, const struct HostStateLoop*);
static bool host_state_load(const char*, CPUState*, const StaticRecompModuleDesc*, const struct HostStateLoop*);
static void bluewake_status_post(const char* text) { snprintf(status, sizeof status, "%s", text); }
static int guard_mkdir(const char* path, unsigned mode) {
    assert(strcmp(path, host_state_dir()) == 0 && mode == 0755);
    ++directories;
    return 0;
}

// Only I/O endpoints are replaced. No real directory, state file or game data
// is touched; a call to either endpoint makes a visible, testable change.
#ifdef mkdir
#undef mkdir
#endif
#define mkdir guard_mkdir
#include "host_state_guard_under_test.inc"
#undef mkdir

static u32 mem_read32(CPUState* cpu, u32 address) {
    (void)cpu;
    if (address == BLUEWAKE_VI_RETRACE_COUNT) return guest.retrace;
    if (address == 0x803F6160u) return guest.scene;
    if (address == 0x80001010u) return 0; // synthetic module is not loaded
    assert(address == 0x8000101Cu);
    return 0x80002000u; // synthetic module name
}
static void guest_read_cstr(CPUState* cpu, u32 address, char* out, size_t size) {
    (void)cpu;
    assert(address == 0x80002000u);
    snprintf(out, size, "synthetic");
}
static int dvd_path_to_entrynum(const char* path) {
    assert(strcmp(path, "/rels/synthetic.rel") == 0);
    return 1;
}
static bool host_state_save(const char* path, CPUState* cpu,
                            const StaticRecompModuleDesc* mod, const HostStateLoop* loop) {
    (void)mod; (void)loop;
    ++saves;
    ++file_version;
    saved_cpu = *cpu;
    snprintf(saved_path, sizeof saved_path, "%s", path);
    return true;
}
static bool host_state_load(const char* path, CPUState* cpu,
                            const StaticRecompModuleDesc* mod, const HostStateLoop* loop) {
    (void)mod; (void)loop;
    assert(strcmp(path, "/synthetic/states/existing.bwstate") == 0);
    ++loads;
    cpu->pc = 0x80009000u;
    guest.scene = 0x80003000u;
    return true;
}

static bool prolog, pending;
static u32 r13;
static const StaticRecompModuleDesc module = {0};
static const HostStateLoop loop = {&prolog, &pending, &r13};

static CPUState reset(void) {
    memset(g_loose_rel_waits, 0, sizeof g_loose_rel_waits);
    memset(g_state_requests, 0, sizeof g_state_requests);
    g_loose_rel_retraces = 1;
    g_loose_rel_reports = 64;
    g_host_retrace_count = 100;
    guest.retrace = 10;
    guest.scene = 0;
    g_state_request_count = g_state_refusals = 0;
    g_state_hotkey_save = g_state_hotkey_load = g_state_save_armed = false;
    g_state_armed_retrace = g_state_test_load_retrace = 0;
    g_state_poll_retrace = UINT64_MAX;
    g_state_test_load_done = g_state_aurora = false;
    snprintf(g_state_last_path, sizeof g_state_last_path, "/synthetic/states/existing.bwstate");
    prolog = pending = false;
    r13 = saves = loads = directories = 0;
    file_version = 7;
    status[0] = saved_path[0] = '\0';
    CPUState cpu = {.pc = HOST_STATE_DRAW_DONE_PC, .gpr = {[1] = 0x80070000u}};
    return cpu;
}
static void turn(CPUState* cpu) { host_state_turn(cpu, &module, &loop); }
static CPUState start_wait(unsigned slot) {
    CPUState cpu = {.pc = BLUEWAKE_DO_LOAD, .lr = 0x80008000u,
                   .gpr = {[1] = 0x80040000u + slot * 0x1000u, [3] = 0x80001000u}};
    assert(host_loose_rel_wait(&cpu));
    assert(cpu.pc == BLUEWAKE_VI_WAIT_FOR_RETRACE && cpu.lr == BLUEWAKE_DO_LOAD);
    // Move an isolated wait to the final slot to catch first-slot-only guards.
    if (slot == BLUEWAKE_MAX_LOOSE_REL_WAITS - 1u) {
        assert(g_loose_rel_waits[slot].stack == 0);
        g_loose_rel_waits[slot] = g_loose_rel_waits[0];
        g_loose_rel_waits[0] = (BlueWakeLooseRelWait){0};
    }
    assert(g_loose_rel_waits[slot].stack == cpu.gpr[1]);
    return cpu;
}
static void retire_wait(CPUState* waiter) {
    guest.retrace = 11;
    waiter->pc = BLUEWAKE_DO_LOAD;
    waiter->gpr[3] = 0xDEADBEEFu; // VIWaitForRetrace need not preserve r3
    assert(!host_loose_rel_wait(waiter));
    assert(waiter->gpr[3] == 0x80001000u && waiter->lr == 0x80008000u);
}
static void untouched_io(void) { assert(saves == 0 && loads == 0 && directories == 0 && file_version == 7); }

static void save_waits_for_retirement(unsigned slot) {
    CPUState cpu = reset(), waiter = start_wait(slot);
    const CPUState before = cpu;
    bluewake_save_state_hotkey(false);
    turn(&cpu); // a different guest thread is at draw-done
    untouched_io();
    assert(g_state_save_armed && g_state_hotkey_save);
    guest.retrace = 11; // elapsed is not retired
    ++g_host_retrace_count;
    turn(&cpu);
    untouched_io();
    assert(memcmp(&before, &cpu, sizeof cpu) == 0);
    retire_wait(&waiter);
    turn(&cpu); // still the same retrace: draw-done can now save
    assert(saves == 1 && directories == 1 && file_version == 8);
    assert(memcmp(&saved_cpu, &cpu, sizeof cpu) == 0);
    assert(!g_state_save_armed && !g_state_hotkey_save && strcmp(status, "State saved") == 0);
    ++g_host_retrace_count;
    turn(&cpu);
    assert(saves == 1);
}
static void scheduled_retrace_fallback(void) {
    CPUState cpu = reset(), waiter = start_wait(0);
    cpu.pc = 0x80005000u; // no draw-done point
    g_state_request_count = 1;
    g_state_requests[0] = (HostStateRequest){"/synthetic/scheduled.bwstate", 100, false};
    turn(&cpu);
    guest.retrace = 11;
    g_host_retrace_count = 220; // fallback point must also respect the slot
    turn(&cpu);
    untouched_io();
    assert(!g_state_requests[0].done);
    retire_wait(&waiter);
    ++g_host_retrace_count;
    turn(&cpu);
    assert(saves == 1 && directories == 0 && g_state_requests[0].done);
    assert(strcmp(saved_path, "/synthetic/scheduled.bwstate") == 0);
    ++g_host_retrace_count;
    turn(&cpu);
    assert(saves == 1);
}
static void every_wait_must_retire(void) {
    CPUState cpu = reset(), first = start_wait(0), second = start_wait(1);
    bluewake_save_state_hotkey(false);
    turn(&cpu);
    retire_wait(&first);
    ++g_host_retrace_count;
    turn(&cpu);
    untouched_io();
    retire_wait(&second);
    turn(&cpu);
    assert(saves == 1);
}
static void busy_timeout(void) {
    CPUState cpu = reset();
    (void)start_wait(0);
    const BlueWakeLooseRelWait before = g_loose_rel_waits[0];
    g_state_request_count = 2;
    g_state_requests[0] = (HostStateRequest){"/synthetic/due.bwstate", 100, false};
    g_state_requests[1] = (HostStateRequest){"/synthetic/future.bwstate", 9000, false};
    bluewake_save_state_hotkey(false);
    turn(&cpu);
    guest.retrace = 11;
    g_host_retrace_count = 3700;
    turn(&cpu);
    assert(g_state_save_armed);
    ++g_host_retrace_count;
    turn(&cpu);
    untouched_io();
    assert(!g_state_save_armed && !g_state_hotkey_save);
    assert(g_state_requests[0].done && !g_state_requests[1].done);
    assert(strcmp(status, "State not saved: the game was busy") == 0);
    assert(memcmp(&before, &g_loose_rel_waits[0], sizeof before) == 0);
}
static void load_refuses_without_changing_machine(unsigned slot) {
    CPUState cpu = reset(), waiter = start_wait(slot);
    const CPUState before = cpu;
    const BlueWakeLooseRelWait waiting = g_loose_rel_waits[slot];
    bluewake_save_state_hotkey(true);
    turn(&cpu);
    untouched_io();
    assert(!g_state_hotkey_load && strcmp(status, "State not loaded: the game was busy") == 0);
    assert(memcmp(&cpu, &before, sizeof cpu) == 0 && guest.scene == 0 && guest.retrace == 10);
    assert(memcmp(&waiting, &g_loose_rel_waits[slot], sizeof waiting) == 0);
    retire_wait(&waiter);
    ++g_host_retrace_count;
    turn(&cpu);
    assert(loads == 0); // refusal consumes the request; no surprise later load
    bluewake_save_state_hotkey(true);
    ++g_host_retrace_count;
    turn(&cpu);
    assert(loads == 1 && cpu.pc == 0x80009000u && guest.scene == 0x80003000u);
    assert(strcmp(status, "State loaded") == 0);
}
static void disabled_wait_keeps_save_behavior(void) {
    CPUState cpu = reset();
    g_loose_rel_retraces = 0;
    assert(!host_loose_rel_wait(&cpu));
    bluewake_save_state_hotkey(false);
    turn(&cpu);
    assert(saves == 1 && strcmp(status, "State saved") == 0);
}
void host_state_guard_tests(void) {
    save_waits_for_retirement(0);
    save_waits_for_retirement(BLUEWAKE_MAX_LOOSE_REL_WAITS - 1u);
    scheduled_retrace_fallback();
    every_wait_must_retire();
    busy_timeout();
    load_refuses_without_changing_machine(0);
    load_refuses_without_changing_machine(BLUEWAKE_MAX_LOOSE_REL_WAITS - 1u);
    disabled_wait_keeps_save_behavior();
    puts("8 loose-REL save/load guard cases passed (synthetic guest and I/O endpoints)");
}
