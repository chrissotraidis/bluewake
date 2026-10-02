#include "card_runtime.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CARD_PROBE_EX 0x8031D438u
#define CARD_MOUNT_ASYNC 0x8031DAFCu
#define CARD_MOUNT 0x8031DC9Cu
#define CARD_OPEN 0x8031E74Cu
#define CARD_CLOSE 0x8031E8C4u
#define CARD_CREATE 0x8031EC68u
#define CARD_READ 0x8031F0E0u
#define CARD_WRITE 0x8031F45Cu
#define CARD_CALLBACK_RETURN 0x7FFF0000u

static void call_card(CPUState* cpu, u32 address) {
    cpu->pc = address;
    cpu->lr = 0x80001000u;
    assert(bluewake_card_runtime_dispatch(cpu));
    assert(cpu->pc == 0x80001000u);
}

int main(void) {
    char directory[] = "/tmp/bluewake-card-runtime-XXXXXX";
    assert(mkdtemp(directory) != NULL);
    char path[512];
    assert(snprintf(path, sizeof path, "%s/nested/GZLE01.card", directory) <
           (int)sizeof path);

    CPUState cpu;
    assert(cpu_init(&cpu));
    assert(bluewake_card_runtime_open(path));
    assert(strcmp(bluewake_card_runtime_path(), path) == 0);

    bluewake_card_runtime_suspend_writes(true);
    cpu.gpr[3] = 0u;
    call_card(&cpu, CARD_MOUNT);
    assert((s32)cpu.gpr[3] == DOL_CARD_RESULT_BUSY);
    bluewake_card_runtime_suspend_writes(false);

    const u32 size_address = 0x80002000u;
    const u32 sector_address = 0x80002004u;
    cpu.gpr[3] = 0u;
    cpu.gpr[4] = size_address;
    cpu.gpr[5] = sector_address;
    call_card(&cpu, CARD_PROBE_EX);
    assert((s32)cpu.gpr[3] == 0);
    assert(mem_read32(&cpu, size_address) == 4u);
    assert(mem_read32(&cpu, sector_address) == 8192u);

    cpu.gpr[3] = 0u;
    cpu.gpr[6] = 0xFFFFFFFFu;
    call_card(&cpu, CARD_MOUNT);
    assert((s32)cpu.gpr[3] == 0);

    cpu.gpr[3] = 0u;
    cpu.gpr[6] = 0x80003000u;
    call_card(&cpu, CARD_MOUNT_ASYNC);
    assert((s32)cpu.gpr[3] == 0);
    cpu.gpr[20] = 0x12345678u;
    bluewake_card_runtime_service_callback(&cpu);
    assert(cpu.pc == 0x80003000u);
    assert(cpu.lr == CARD_CALLBACK_RETURN);
    assert(cpu.gpr[3] == 0u);
    assert(cpu.gpr[4] == 0u);
    cpu.pc = CARD_CALLBACK_RETURN;
    cpu.gpr[20] = 0u;
    bluewake_card_runtime_service_callback(&cpu);
    assert(cpu.pc == 0x80001000u);
    assert(cpu.gpr[20] == 0x12345678u);

    const u32 name_address = 0x80002100u;
    const u32 info_address = 0x80002200u;
    const u32 write_address = 0x80004000u;
    const u32 read_address = 0x80008000u;
    const char name[] = "gczelda";
    for (size_t i = 0; i < sizeof name; i++)
        mem_write8(&cpu, name_address + (u32)i, (u8)name[i]);

    cpu.gpr[3] = 0u;
    cpu.gpr[4] = name_address;
    cpu.gpr[5] = 8192u;
    cpu.gpr[6] = info_address;
    cpu.gpr[7] = 0xFFFFFFFFu;
    call_card(&cpu, CARD_CREATE);
    assert((s32)cpu.gpr[3] == 0);

    for (u32 i = 0; i < 8192u; i++)
        mem_write8(&cpu, write_address + i, (u8)(i * 37u));
    cpu.gpr[3] = info_address;
    cpu.gpr[4] = write_address;
    cpu.gpr[5] = 8192u;
    cpu.gpr[6] = 0u;
    cpu.gpr[7] = 0xFFFFFFFFu;
    call_card(&cpu, CARD_WRITE);
    assert((s32)cpu.gpr[3] == 0);
    cpu.gpr[3] = info_address;
    call_card(&cpu, CARD_CLOSE);
    assert((s32)cpu.gpr[3] == 0);

    bluewake_card_runtime_close();
    assert(bluewake_card_runtime_open(path));
    cpu.gpr[3] = 0u;
    cpu.gpr[6] = 0xFFFFFFFFu;
    call_card(&cpu, CARD_MOUNT);

    cpu.gpr[3] = 0u;
    cpu.gpr[4] = name_address;
    cpu.gpr[5] = info_address;
    call_card(&cpu, CARD_OPEN);
    assert((s32)cpu.gpr[3] == 0);
    cpu.gpr[3] = info_address;
    cpu.gpr[4] = read_address;
    cpu.gpr[5] = 8192u;
    cpu.gpr[6] = 0u;
    cpu.gpr[7] = 0xFFFFFFFFu;
    call_card(&cpu, CARD_READ);
    assert((s32)cpu.gpr[3] == 0);
    for (u32 i = 0; i < 8192u; i++)
        assert(mem_read8(&cpu, read_address + i) == (u8)(i * 37u));

    bluewake_card_runtime_close();
    cpu_free(&cpu);
    assert(remove(path) == 0);

    assert(setenv("HOME", directory, 1) == 0);
    assert(bluewake_card_runtime_open(NULL));
    char default_path[512];
    assert(snprintf(default_path, sizeof default_path,
                    "%s/Library/Application Support/BlueWake/GZLE01.card",
                    directory) < (int)sizeof default_path);
    assert(strcmp(bluewake_card_runtime_path(), default_path) == 0);
    bluewake_card_runtime_close();
    assert(access(default_path, F_OK) == 0);
    assert(remove(default_path) == 0);
    puts("CARD bridge persistence contract test passed.");
    return 0;
}
