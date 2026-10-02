// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "save_state.h"
#include "gxruntime/interrupts.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
typedef struct { u32 pc; u32 ram_size; u8* ram; } CPUState;
typedef struct { int unused; } StaticRecompModuleDesc;
typedef struct { bool* profile_prolog_called; bool* rel_prolog_sda_pending; u32* rel_prolog_saved_r13; } HostStateLoop;
typedef struct { u32 cpu_pod_size; } HostStateHeader;
typedef struct { u32 linked_start; u32 size; } HostStateAlias;
static bool g_state_aurora;
static u32 g_state_alias_count = 1;
static HostStateAlias g_state_aliases[] = {{0x90000000, 4}};
static u64 g_host_retrace_count;
static u32* g_cycle_vi_clock;
static DolInterrupts g_interrupts = {
    .pe_token = 0xBEEF,
    .pe_control = DOL_PE_TOKEN_ENABLE_BIT | DOL_PE_FINISH_ENABLE_BIT,
    .pe_token_pending = true,
};
static char g_state_last_path[512];
static u32 host_field = 0x4321;
static const BwStateField k_host_state_fields[] = {{"host_field", &host_field, sizeof host_field}};
static int fail_alias, fail_pack;
static u8 alias_storage[4] = {1,2,3,4};
#define ARAM_SIZE 4u
static u8* aram_buffer(void) { return NULL; }
static u64 host_state_now_us(void) { return 1; }
static size_t dol_aurora_gx_save_state(void** data) { *data = NULL; return 0; }
static void host_state_header(HostStateHeader* header, CPUState* cpu, const StaticRecompModuleDesc* mod) {
    (void)cpu; (void)mod; header->cpu_pod_size = sizeof(u32);
}
static bool ppc_guest_alias_get_storage(u32 address, u32 size, u8** storage) {
    assert(address == 0x90000000 && size == 4);
    *storage = fail_alias ? NULL : alias_storage;
    return !fail_alias;
}
static bool pack_or_fail(const BwStateField* fields, u32 count, u8** data, u64* size) {
    return !fail_pack && bw_state_fields_pack(fields, count, data, size);
}
#define bw_state_fields_pack pack_or_fail
#include "host_state_save_under_test.inc"
#undef bw_state_fields_pack
int main(void) {
    char path[256];
    snprintf(path, sizeof path, "host-state-failure-%lu.bwstate", (unsigned long)getpid());
    u8 ram[16] = {1};
    CPUState cpu = {.pc = 0x80001000, .ram_size = sizeof ram, .ram = ram};
    StaticRecompModuleDesc mod = {0};
    bool prolog = false, pending = false;
    u32 r13 = 0;
    HostStateLoop loop = {&prolog, &pending, &r13};
    assert(host_state_save(path, &cpu, &mod, &loop));
    BwStateReader reader;
    assert(bw_state_reader_open(&reader, path));
    assert(bw_state_find(&reader, "ALIASES") && bw_state_find(&reader, "HOSTVARS"));
    const BwStateChunk* pe = bw_state_find(&reader, "PE");
    assert(pe != NULL && pe->size == sizeof(g_interrupts) - offsetof(DolInterrupts, pe_token));
    assert(memcmp(pe->data, &g_interrupts.pe_token, (size_t)pe->size) == 0);
    const size_t old_size = reader.buffer_size;
    const u64 old_hash = bw_state_hash(reader.buffer, reader.buffer_size, 0);
    bw_state_reader_close(&reader);
    for (int failure = 0; failure < 2; ++failure) {
        fail_alias = failure == 0;
        fail_pack = failure == 1;
        assert(!host_state_save(path, &cpu, &mod, &loop));
        assert(bw_state_reader_open(&reader, path));
        assert(reader.buffer_size == old_size && bw_state_hash(reader.buffer, reader.buffer_size, 0) == old_hash);
        bw_state_reader_close(&reader);
    }
    assert(remove(path) == 0);
    puts("failed host serialization preserves the previous complete state");
}
