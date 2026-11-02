/* BlueWake composite module export glue (P3).
 * Adapts the ModernGekko module-template export for the DOL+REL composite.
 */
#include "generated_composite.h"
#include "StaticRecompABI.h"
#include "module_cpu_contract.h"
#include "dispatch_loop.h"

extern void ppc_set_mem_write_journal(PPCMemWriteJournal fn, void* user);

unsigned dolrecomp_call_depth = 0;
static BluewakeEdgeServiceFn s_edge_service;
static void* s_edge_service_user;
#if defined(BLUEWAKE_DIRECT_CALLS)
/* Use the same selected chunk table and lookup as ordinary dispatch, including
 * mod variants. The donor's direct calls must not bypass player options. */
void (**const bw_chunk_fns)(CPUState*) = s_dolrecomp_chunk_fns;
DolRecompFunction bw_find_chunk(u32 address) {
    return dolrecomp_find_original(address);
}
#endif

#ifdef BLUEWAKE_NATIVE_MATH_CACHED
#include "native_math.h"
static void native_matrix_copy(CPUState* cpu) {
    if (!bluewake_native_math_try(cpu, 0x8030D0C8u)) func_803096E0(cpu);
}
static void native_matrix_concat(CPUState* cpu) {
    if (!bluewake_native_math_try(cpu, 0x8030D0FCu)) func_803096E0(cpu);
}
static void native_matrix_vec(CPUState* cpu) {
    if (!bluewake_native_math_try(cpu, 0x8030DA44u)) func_8030D6E0(cpu);
}
static void native_matrix_array(CPUState* cpu) {
    if (!bluewake_native_math_try(cpu, 0x8030DA98u)) func_8030D6E0(cpu);
}
static DolRecompFunction bluewake_native_math_find(u32 address) {
    /* Cached wrappers always recheck the host handshake, including after disable. */
    switch (address) {
    case 0x8030D0C8u: return native_matrix_copy;
    case 0x8030D0FCu: return native_matrix_concat;
    case 0x8030DA44u: return native_matrix_vec;
    case 0x8030DA98u: return native_matrix_array;
    default: return NULL;
    }
}
#endif

#if defined(BLUEWAKE_DIRECT_CALLS)
int bw_native_call(CPUState* cpu, u32 address) {
#ifdef BLUEWAKE_NATIVE_MATH_CACHED
    return bluewake_native_math_try(cpu, address);
#else
    (void)cpu; (void)address;
    return 0;
#endif
}
#endif

/* The x86-64-v3 dispatch clones come only from DolRecomp's LLVM object
 * backend; the C backend the Builder uses emits none, so an x86-64 build (the
 * Windows port) dispatches through dolrecomp_call unless the source says it
 * carries them. */
#if defined(__x86_64__) && defined(DOLRECOMP_HAS_X86_64_V3_DISPATCH)
#define BLUEWAKE_X86_64_V3_DISPATCH 1
#else
#define BLUEWAKE_X86_64_V3_DISPATCH 0
#endif

#if BLUEWAKE_X86_64_V3_DISPATCH
static int host_has_x86_64_v3(void)
{
#if defined(__GNUC__) || defined(__clang__)
    static int supported = -1;
    if (supported < 0)
    {
        __builtin_cpu_init();
        supported = __builtin_cpu_supports("avx") &&
            __builtin_cpu_supports("avx2") &&
            __builtin_cpu_supports("fma") &&
            __builtin_cpu_supports("bmi") &&
            __builtin_cpu_supports("bmi2") &&
            __builtin_cpu_supports("movbe") &&
            __builtin_cpu_supports("lzcnt");
    }
    return supported;
#else
    return 0;
#endif
}
#endif

static int selected_dispatch(CPUState* ctx, u32 address)
{
#if BLUEWAKE_X86_64_V3_DISPATCH
    if (host_has_x86_64_v3())
        return dolrecomp_call__x86_64_v3(ctx, address);
#endif
    return dolrecomp_call(ctx, address);
}

void dolrecomp_indirect_dispatch(CPUState* ctx, u32 address)
{
    (void)selected_dispatch(ctx, address);
}

static int chassis_dispatch(CPUState* ctx, u32 address)
{
    /* The inlined form, so the loop sees selected_dispatch as a static function
     * rather than as a pointer it must reload and call indirectly at every
     * guest edge. Same body as the exported entry point. */
    return bluewake_chassis_dispatch_loop(
        ctx, address, selected_dispatch, s_edge_service,
        s_edge_service_user);
}

static void chassis_on_state_loaded(CPUState* ctx)
{
    ppc_fpscr_updated(ctx);
}

#include "module_tables.inc"
#include "rel_modules.inc"
typedef struct BlueWakeRelData
{
    u32 module_id;
    u32 section_index;
    u32 linked_start;
    u32 size;
    const u8* bytes;
} BlueWakeRelData;
typedef struct BlueWakeRelLifecycle
{
    u32 module_id;
    u32 prolog_address;
} BlueWakeRelLifecycle;
#include "rel_data.inc"

static const StaticRecompModuleDesc s_desc = {
#if defined(BW_GUEST_MEM1)
    BLUEWAKE_FIXED_MEM1_ABI_VERSION,
#elif defined(BLUEWAKE_FIXED_CPU)
    BLUEWAKE_FIXED_CPU_ABI_VERSION,
#else
    STATICRECOMP_ABI_VERSION,
#endif
    GXRUNTIME_CPU_ABI_VERSION,
    (u32)sizeof(CPUState),
    MODULE_GAME_ID,
    DOLRECOMP_ENTRY_POINT,
    chassis_dispatch,
    chassis_on_state_loaded,
    s_code_ranges,
    MODULE_CODE_RANGE_COUNT,
    s_smc_ranges,
    MODULE_SMC_RANGE_COUNT,
    s_chunk_ranges,
    MODULE_CHUNK_RANGE_COUNT,
    s_chunk_hashes,
    s_rel_modules,
    MODULE_REL_MODULE_COUNT,
};

#if defined(_WIN32)
#define RECOMP_MODULE_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define RECOMP_MODULE_EXPORT __attribute__((visibility("default")))
#else
#define RECOMP_MODULE_EXPORT
#endif

#if defined(BLUEWAKE_NATIVE_ENTRIES)
/* Wind Waker Recomp's certified native entries (scripts/windows/native_entries.py
 * hooks them only where the translation is the one their tests compared): the
 * J3D FIFO matrix loads, two collision checks, PSMTXMultVecSR, the joint matrix
 * calculations and the actor search by name. Built only with --native-entries;
 * BLUEWAKE_NATIVE_ENTRIES=0 leaves them to the translation. */
#include "native_fifo.h"
#include "native_bg.h"
#include "native_vec.h"
#include "native_mtxcalc.h"
#include "native_search.h"
/* The seventh set: leaf compute code. */
#include "native_libm.h"
#include "native_bgblk.h"
#include "native_rot.h"
#include "native_calc.h"
#include "native_geom.h"
#include "native_jas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void bluewake_native_entries_enable(void)
{
    static int done;
    if (done) return;
    done = 1;
    const char* setting = getenv("BLUEWAKE_NATIVE_ENTRIES");
    const int on = !(setting && strcmp(setting, "0") == 0);
    bluewake_native_fifo_enabled = on;
    bluewake_native_bg_enabled = on;
    bluewake_native_vec_sr_enabled = on;
    bluewake_native_mtxcalc_enabled = on;
    bluewake_native_search_enabled = on;
    /* The seventh set follows the same switch. BLUEWAKE_NATIVE_<GROUP>=0
     * leaves that group to the translation. */
    {
        static const struct {
            const char* name;
            int* enabled;
            void (*report)(void);
        } groups[] = {
            {"BLUEWAKE_NATIVE_LIBM", &bluewake_native_libm_enabled, bluewake_native_libm_report},
            {"BLUEWAKE_NATIVE_BGBLK", &bluewake_native_bgblk_enabled, bluewake_native_bgblk_report},
            {"BLUEWAKE_NATIVE_ROT", &bluewake_native_rot_enabled, bluewake_native_rot_report},
            {"BLUEWAKE_NATIVE_CALC", &bluewake_native_calc_enabled, bluewake_native_calc_report},
            {"BLUEWAKE_NATIVE_GEOM", &bluewake_native_geom_enabled, bluewake_native_geom_report},
            {"BLUEWAKE_NATIVE_JAS", &bluewake_native_jas_enabled, bluewake_native_jas_report},
        };
        for (unsigned g = 0; g < sizeof groups / sizeof groups[0]; ++g) {
            const char* value = getenv(groups[g].name);
            *groups[g].enabled = on && !(value && strcmp(value, "0") == 0);
            if (*groups[g].enabled)
                atexit(groups[g].report);
        }
    }
    if (on) {
        atexit(bluewake_native_fifo_report);
        atexit(bluewake_native_bg_report);
        atexit(bluewake_native_vec_sr_report);
        atexit(bluewake_native_mtxcalc_report);
        atexit(bluewake_native_search_report);
        fprintf(stderr, "[native-entries] certified native entries enabled\n");
    }
}
#endif

#if defined(BLUEWAKE_NATIVE_SET4)
#include "native_kankyo.h"
#include "native_anim.h"
#include "native_cc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void bluewake_native_set4_enable(void)
{
    static int done;
    if (done) return;
    done = 1;
    const char* setting = getenv("BLUEWAKE_NATIVE_SET4");
    const int on = !(setting && strcmp(setting, "0") == 0);
    {
        static const struct {
            const char* name;
            int* enabled;
            void (*report)(void);
        } groups[] = {
            {"BLUEWAKE_NATIVE_KANKYO", &bluewake_native_kankyo_enabled, bluewake_native_kankyo_report},
            {"BLUEWAKE_NATIVE_ANIM", &bluewake_native_anim_enabled, bluewake_native_anim_report},
            {"BLUEWAKE_NATIVE_CC", &bluewake_native_cc_enabled, bluewake_native_cc_report},
        };
        for (unsigned g = 0; g < sizeof groups / sizeof groups[0]; ++g) {
            const char* value = getenv(groups[g].name);
            *groups[g].enabled = on && !(value && strcmp(value, "0") == 0);
            if (*groups[g].enabled)
                atexit(groups[g].report);
        }
    }
    if (on)
        fprintf(stderr, "[native-set4] colour, animation and collision setup enabled\n");
}
#endif

#if defined(BLUEWAKE_NATIVE_GX)
#include "native_gx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void bluewake_native_gx_enable(void)
{
    static int done;
    if (done) return;
    done = 1;
    const char* setting = getenv("BLUEWAKE_NATIVE_GX");
    const int on = !(setting && strcmp(setting, "0") == 0);
    bluewake_native_gx_enabled = on;
    if (on) {
        atexit(bluewake_native_gx_report);
        fprintf(stderr, "[native-gx] GX FIFO writers enabled\n");
    }
}
#endif

RECOMP_MODULE_EXPORT const StaticRecompModuleDesc* staticrecomp_get_module(void)
{
#if defined(BLUEWAKE_NATIVE_ENTRIES)
    bluewake_native_entries_enable();
#endif
#if defined(BLUEWAKE_NATIVE_GX)
    bluewake_native_gx_enable();
#endif
#if defined(BLUEWAKE_NATIVE_SET4)
    bluewake_native_set4_enable();
#endif
    return &s_desc;
}

RECOMP_MODULE_EXPORT const BlueWakeRelData* staticrecomp_get_rel_data(u32* count)
{
    if (count) *count = MODULE_REL_DATA_COUNT;
    return s_rel_data;
}

RECOMP_MODULE_EXPORT const BlueWakeRelLifecycle* staticrecomp_get_rel_lifecycle(u32* count)
{
    if (count) *count = MODULE_REL_LIFECYCLE_COUNT;
    return s_rel_lifecycle;
}

/* Host-only diagnostics need to reach the runtime instance embedded here. */
RECOMP_MODULE_EXPORT void bluewake_set_mem_write_journal(
    PPCMemWriteJournal fn, void* user)
{
    ppc_set_mem_write_journal(fn, user);
}

RECOMP_MODULE_EXPORT void bluewake_set_edge_service(
    BluewakeEdgeServiceFn fn, void* user)
{
    s_edge_service = fn;
    s_edge_service_user = user;
}

/* Mods (scripts/mods/build_mod_variants.py). A mod is compiled in as variant
 * chunks, extra chunks and data writes; the host enables mods once, at boot,
 * before the first dispatch fills the pc cache. The types are declared in
 * generated_composite.h, whose dispatcher consults the extra chunks. */
typedef void (*BlueWakeModWriteFn)(void* user, u32 address, const u8* bytes, u32 size);
#include "mod_variants.inc"
static u32 s_mod_enabled_mask;

static DolRecompFunction bluewake_mod_extra_find(u32 address)
{
#if MODULE_MOD_COUNT > 0
    for (u32 i = 0; i < MODULE_MOD_EXTRA_CHUNK_COUNT; ++i) {
        const BlueWakeModExtraChunk* c = &s_mod_extra_chunks[i];
        if ((s_mod_enabled_mask & (1u << c->mod)) != 0u && c->start <= address &&
            address < c->end && ((address - c->start) & 3u) == 0u)
            return c->fn;
    }
#else
    (void)address;
#endif
    return NULL;
}

RECOMP_MODULE_EXPORT u32 bluewake_composite_mod_count(void)
{
    return MODULE_MOD_COUNT;
}

RECOMP_MODULE_EXPORT const char* bluewake_composite_mod_name(u32 index)
{
#if MODULE_MOD_COUNT > 0
    return index < MODULE_MOD_COUNT ? s_mod_names[index] : NULL;
#else
    (void)index;
    return NULL;
#endif
}

/* Points the chunk table at the enabled mods' variants; returns how many
 * chunks were replaced. A variant applies when every mod it requires is
 * enabled; the table lists combined variants last, so they win. */
RECOMP_MODULE_EXPORT u32 bluewake_composite_apply_mods(u32 mask)
{
    u32 replaced = 0;
    s_mod_enabled_mask = mask;
#if MODULE_MOD_COUNT > 0
    for (u32 i = 0; i < MODULE_MOD_CHUNK_COUNT; ++i) {
        const BlueWakeModChunk* c = &s_mod_chunks[i];
        if ((mask & c->mask) != c->mask)
            continue;
        for (u32 k = 0; k < DOLRECOMP_CHUNK_COUNT; ++k) {
            if (s_dolrecomp_chunk_starts[k] == c->start) {
                s_dolrecomp_chunk_fns[k] = c->fn;
                replaced++;
                break;
            }
        }
    }
#else
    (void)mask;
#endif
    return replaced;
}

/* Hands the enabled mods' data writes to the host, which writes guest RAM:
 * every write at boot (per_frame_only 0), the per-frame ones (a Gecko code's
 * data writes) at every retrace (per_frame_only 1). */
RECOMP_MODULE_EXPORT u32 bluewake_composite_mod_writes(
    u32 mask, u32 per_frame_only, BlueWakeModWriteFn fn, void* user)
{
    u32 count = 0;
#if MODULE_MOD_COUNT > 0
    for (u32 i = 0; i < MODULE_MOD_WRITE_COUNT; ++i) {
        const BlueWakeModWrite* w = &s_mod_writes[i];
        if ((mask & (1u << w->mod)) == 0u)
            continue;
        if (per_frame_only && !w->every_frame)
            continue;
        if (fn)
            fn(user, w->address, w->bytes, w->size);
        count++;
    }
#else
    (void)mask;
    (void)per_frame_only;
    (void)fn;
    (void)user;
#endif
    return count;
}
