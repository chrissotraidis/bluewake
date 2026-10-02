/* Synthetic loader contract checks. No game code or player files. */
#include "../cmake/composite/module_cpu_contract.h"
#include <stdio.h>

static CPUState fallback, fixed;
static unsigned getter_calls;
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
    CPUState* selected = NULL;
    CHECK(bw_module_select_cpu(&module, NULL, 0, &fallback, &selected) == NULL);
    CHECK(selected == &fallback && getter_calls == 0);
    CHECK(bw_module_select_cpu(&module, fixed_cpu, 0, &fallback, &selected) != NULL);
    CHECK(selected == NULL && getter_calls == 0);

    module.abi_version = BLUEWAKE_FIXED_CPU_ABI_VERSION;
    CHECK(bw_module_select_cpu(&module, NULL, 0, &fallback, &selected) != NULL);
    CHECK(selected == NULL && getter_calls == 0);
    CHECK(bw_module_select_cpu(&module, fixed_cpu, 1, &fallback, &selected) != NULL);
    CHECK(selected == NULL && getter_calls == 0);

    StaticRecompModuleDesc bad = module;
    bad.cpu_abi_version++;
    CHECK(bw_module_select_cpu(&bad, fixed_cpu, 0, &fallback, &selected) != NULL);
    bad = module; bad.cpu_state_size--;
    CHECK(bw_module_select_cpu(&bad, fixed_cpu, 0, &fallback, &selected) != NULL);
    bad = module; bad.game_id[0] = 'X';
    CHECK(bw_module_select_cpu(&bad, fixed_cpu, 0, &fallback, &selected) != NULL);
    bad = module; bad.game_id[6] = 'X';
    CHECK(bw_module_select_cpu(&bad, fixed_cpu, 0, &fallback, &selected) != NULL);
    bad = module; bad.abi_version++;
    CHECK(bw_module_select_cpu(&bad, fixed_cpu, 0, &fallback, &selected) != NULL);
    bad = module; bad.dispatch = NULL;
    CHECK(bw_module_select_cpu(&bad, fixed_cpu, 0, &fallback, &selected) != NULL);
    CHECK(getter_calls == 0 && selected == NULL);

    CHECK(bw_module_select_cpu(&module, null_cpu, 0, &fallback, &selected) != NULL);
    CHECK(selected == NULL && getter_calls == 1);
    CHECK(bw_module_select_cpu(&module, misaligned_cpu, 0, &fallback, &selected) != NULL);
    CHECK(selected == NULL && getter_calls == 2);
    CHECK(bw_module_select_cpu(&module, fixed_cpu, 0, &fallback, &selected) == NULL);
    CHECK(selected == &fixed && getter_calls == 3);
    selected->gpr[3] = 0x12345678u;
    CHECK(fixed.gpr[3] == 0x12345678u && fallback.gpr[3] == 0);
    puts("module CPU contract: ordinary/fixed selection and invalid modules checked");
    return 0;
}
