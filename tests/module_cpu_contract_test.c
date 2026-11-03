/* Synthetic loader contract checks. No game code or player files. */
#include "../cmake/composite/module_cpu_contract.h"
#include <stdio.h>

_Alignas(16) static CPUState fallback, fixed;
static unsigned getter_calls, mem1_calls;
_Alignas(16) static u8 mem1[BLUEWAKE_MODULE_MEM1_SIZE];
static u32 reported_size = sizeof mem1;
static u8* reported_mem1 = mem1;
static u8* fixed_mem1(u32* size) {
    ++mem1_calls; *size = reported_size; return reported_mem1;
}
static CPUState* fixed_cpu(void) { ++getter_calls; return &fixed; }
static CPUState* null_cpu(void) { ++getter_calls; return NULL; }
static CPUState* misaligned_cpu(void) {
    ++getter_calls;
    return (CPUState*)((unsigned char*)&fixed + 1);
}
static int dispatch(CPUState* cpu, u32 address) {
    cpu->pc = address;
    return 1;
}
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    StaticRecompModuleDesc module = {0};
    module.abi_version = STATICRECOMP_ABI_VERSION;
    module.cpu_abi_version = GXRUNTIME_CPU_ABI_VERSION;
    module.cpu_state_size = sizeof(CPUState);
    memcpy(module.game_id, "GZLE01", sizeof("GZLE01"));
    module.dispatch = dispatch;
    BlueWakeModuleStorage selected;
    CHECK(bw_module_select_storage(&module, NULL, NULL, &fallback, &selected) == NULL);
    CHECK(selected.cpu == &fallback && getter_calls == 0);
    CHECK(bw_module_select_storage(&module, fixed_cpu, NULL, &fallback, &selected) != NULL);
    CHECK(selected.cpu == NULL && getter_calls == 0);

    module.abi_version = BLUEWAKE_FIXED_CPU_ABI_VERSION;
    CHECK(bw_module_select_storage(&module, NULL, NULL, &fallback, &selected) != NULL);
    CHECK(selected.cpu == NULL && getter_calls == 0);
    CHECK(bw_module_select_storage(&module, fixed_cpu, fixed_mem1, &fallback, &selected) != NULL);
    CHECK(selected.cpu == NULL && getter_calls == 0);

    StaticRecompModuleDesc bad = module;
    bad.cpu_abi_version++;
    CHECK(bw_module_select_storage(&bad, fixed_cpu, NULL, &fallback, &selected) != NULL);
    bad = module; bad.cpu_state_size--;
    CHECK(bw_module_select_storage(&bad, fixed_cpu, NULL, &fallback, &selected) != NULL);
    bad = module; bad.game_id[0] = 'X';
    CHECK(bw_module_select_storage(&bad, fixed_cpu, NULL, &fallback, &selected) != NULL);
    bad = module; bad.game_id[6] = 'X';
    CHECK(bw_module_select_storage(&bad, fixed_cpu, NULL, &fallback, &selected) != NULL);
    bad = module; bad.abi_version = 99;
    CHECK(bw_module_select_storage(&bad, fixed_cpu, NULL, &fallback, &selected) != NULL);
    bad = module; bad.dispatch = NULL;
    CHECK(bw_module_select_storage(&bad, fixed_cpu, NULL, &fallback, &selected) != NULL);
    CHECK(getter_calls == 0 && selected.cpu == NULL);

    CHECK(bw_module_select_storage(&module, null_cpu, NULL, &fallback, &selected) != NULL);
    CHECK(selected.cpu == NULL && getter_calls == 1);
    CHECK(bw_module_select_storage(&module, misaligned_cpu, NULL, &fallback, &selected) != NULL);
    CHECK(selected.cpu == NULL && getter_calls == 2);
    CHECK(bw_module_select_storage(&module, fixed_cpu, NULL, &fallback, &selected) == NULL);
    CHECK(selected.cpu == &fixed && getter_calls == 3);
    selected.cpu->gpr[3] = 0x12345678u;
    CHECK(fixed.gpr[3] == 0x12345678u && fallback.gpr[3] == 0);
    CHECK(mem1_calls == 0);
    module.abi_version = BLUEWAKE_FIXED_MEM1_ABI_VERSION;
    CHECK(bw_module_select_storage(&module, fixed_cpu, NULL, &fallback, &selected) != NULL);
    CHECK(bw_module_select_storage(&module, NULL, fixed_mem1, &fallback, &selected) != NULL);
    CHECK(getter_calls == 3 && mem1_calls == 0);
    reported_mem1 = NULL;
    CHECK(bw_module_select_storage(&module, fixed_cpu, fixed_mem1, &fallback, &selected) != NULL);
    reported_mem1 = mem1 + 1;
    CHECK(bw_module_select_storage(&module, fixed_cpu, fixed_mem1, &fallback, &selected) != NULL);
    reported_mem1 = mem1; reported_size--;
    CHECK(bw_module_select_storage(&module, fixed_cpu, fixed_mem1, &fallback, &selected) != NULL);
    reported_size = sizeof mem1 + 1;
    CHECK(bw_module_select_storage(&module, fixed_cpu, fixed_mem1, &fallback, &selected) != NULL);
    reported_size = sizeof mem1; reported_mem1 = (u8*)&fixed;
    CHECK(bw_module_select_storage(&module, fixed_cpu, fixed_mem1, &fallback, &selected) != NULL);
    reported_mem1 = mem1;
    CHECK(bw_module_select_storage(&module, fixed_cpu, fixed_mem1, &fallback, &selected) == NULL);
    CHECK(selected.cpu == &fixed && selected.mem1 == mem1 && selected.mem1_size == sizeof mem1);
    memset(mem1, 0xA5, sizeof mem1);
    CHECK(bw_module_cpu_init(&selected));
    CHECK(fixed.ram == mem1 && fixed.ram_size == sizeof mem1);
    CHECK(mem1[0] == 0 && mem1[sizeof mem1 - 1] == 0 && fixed.gpr[3] == 0);
    fixed.ram[7] = 42;
    bw_module_cpu_free(&selected);  /* ASan must not observe a free of static RAM. */
    CHECK(fixed.ram == NULL && mem1[7] == 42);
    module.abi_version = STATICRECOMP_ABI_VERSION;
    CHECK(bw_module_select_storage(&module, NULL, NULL, &fallback, &selected) == NULL);
    CHECK(bw_module_cpu_init(&selected));
    CHECK(fallback.ram != NULL && fallback.ram != mem1 && fallback.ram_size == GC_MAIN_RAM_SIZE);
    bw_module_cpu_free(&selected);
    CHECK(fallback.ram == NULL);
    puts("module storage: ordinary/fixed CPU and MEM1 selection, rejection and ownership checked");
    return 0;
}
