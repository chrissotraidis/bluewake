#include "game_options.h"
#include "bmg_patch.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The native code at the options' hook sites (the ids in mods/betterww/
// options.txt). Each does what Better Wind Waker's added assembly does at the
// same place, on the guest state the site hands it.
enum {
    HOOK_SWING_TURN = 1,
    HOOK_INVERT_CAMERA_X = 2,
    HOOK_SAIL_WIND = 3,
    HOOK_SAIL_BRAKE = 4,
    HOOK_SAIL_IDLE_BRAKE = 5,
};

typedef u32 (*NativeHookFn)(CPUState* ctx, u32 id);
typedef void (*OptionWriteFn)(void* user, u32 address, const u8* bytes, u32 size);

typedef const char* (*OptionFn)(u32, u32*, u32*, const char**);

static volatile unsigned char* g_flags;
static OptionFn g_option;
static CPUState* g_cpu;
static u32 g_instant_text;
static u32 g_brisk_sail;
static u32 g_invert_camera_x;

static bool option_on(u32 index) { return g_flags != NULL && index != 0u && g_flags[index] != 0u; }

static float f32_from_bits(u32 bits) {
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static void set_single(CPUState* cpu, u32 reg, float value) {
    // As lfs leaves it: the single's value in both halves.
    cpu->fpr[reg] = value;
    cpu->ps1[reg] = value;
}

static s32 truncate_to_s32(float value) {
    // fctiwz: toward zero, saturating.
    if (value != value)
        return (s32)0x80000000u;
    if (value >= 2147483647.0f)
        return 0x7FFFFFFF;
    if (value <= -2147483648.0f)
        return (s32)0x80000000u;
    return (s32)value;
}

// Turn while swinging (procRopeSwing, before the lfs the patch branched from):
// the stick's horizontal axis turns Link by up to the rope's base rotation
// speed a frame, ignoring the stick's first quarter. r31 is the player.
static u32 hook_swing_turn(CPUState* cpu) {
    const float stick = f32_from_bits(mem_read32(cpu, 0x803A4DF0u));
    const float base = f32_from_bits(mem_read32(cpu, 0x803FA2E8u));
    const s32 velocity = truncate_to_s32(base * stick);
    const s32 threshold = (s32)((u32)truncate_to_s32(base) >> 2);
    if (velocity >= threshold || velocity <= -threshold) {
        const u32 player = cpu->gpr[31];
        const u16 angle = (u16)(mem_read16(cpu, player + 0x20Eu) - (u32)velocity);
        mem_write16(cpu, player + 0x20Eu, angle);
        mem_write16(cpu, player + 0x206u, angle);
    }
    return 0u;
}

// Swift Sail's wind, after KoRL's sail sound call: the wind blows the way he
// faces, rounded to the nearest eighth of a turn, through the game's own
// dKyw_tact_wind_set(0, angle). The call is made by returning into it with the
// link register pointing back at this site; the second visit is the return.
static u32 hook_sail_wind(CPUState* cpu) {
    static bool returning;
    if (returning) {
        returning = false;
        return 0u;
    }
    const u32 korl = mem_read32(cpu, 0x803CA75Cu);
    if (korl == 0u)
        return 0u;
    const s32 facing = -(s32)(s16)mem_read16(cpu, korl + 0x206u) + 0x4000 + 0x1000;
    returning = true;
    cpu->gpr[3] = 0u;
    cpu->gpr[4] = (u32)facing & 0xFFFFE000u;
    cpu->lr = cpu->pc;
    return 0x8008A7D0u; // dKyw_tact_wind_set__Fss
}

static u32 native_hook(CPUState* cpu, u32 id) {
    // The first call of each hook is logged: it shows a setting's code ran.
    static u32 seen;
    if (id < 32u && (seen & (1u << id)) == 0u) {
        seen |= 1u << id;
        fprintf(stderr, "[options] hook %u first ran at pc=0x%08X\n", id, cpu->pc);
    }
    switch (id) {
    case HOOK_SWING_TURN:
        return hook_swing_turn(cpu);
    case HOOK_INVERT_CAMERA_X:
        // After the C-stick's horizontal axis is loaded for the camera.
        cpu->fpr[1] = -cpu->fpr[1];
        return 0u;
    case HOOK_SAIL_WIND:
        return hook_sail_wind(cpu);
    case HOOK_SAIL_BRAKE:
        // cLib_addCalc's largest and smallest step when A stops the boat
        // (1.0 and 0.1 in the game).
        set_single(cpu, 3, option_on(g_brisk_sail) ? 3.0f : 2.0f);
        set_single(cpu, 4, 0.2f);
        return 0u;
    case HOOK_SAIL_IDLE_BRAKE:
        // ... and when Link is knocked off a moving boat (1.0 and 0.05).
        set_single(cpu, 3, option_on(g_brisk_sail) ? 4.0f : 2.0f);
        set_single(cpu, 4, option_on(g_brisk_sail) ? 0.2f : 0.1f);
        return 0u;
    default:
        return 0u;
    }
}

static void write_guest(void* user, u32 address, const u8* bytes, u32 size) {
    for (u32 i = 0; i < size; ++i)
        mem_write8((CPUState*)user, address + i, bytes[i]);
}

static bool named(const char* list, const char* name, bool* on) {
    const size_t len = strlen(name);
    bool found = false;
    for (const char* p = list; p != NULL && *p != '\0';) {
        const char* end = strchr(p, ',');
        size_t n = end ? (size_t)(end - p) : strlen(p);
        bool value = true;
        const char* word = p;
        if (n > 0 && *word == '-') {
            value = false;
            ++word;
            --n;
        }
        if (n == len && strncmp(word, name, n) == 0) {
            *on = value;
            found = true;
        }
        p = end ? end + 1 : NULL;
    }
    return found;
}

void bluewake_game_options_enable(void* lib, CPUState* cpu, bool mod_enabled) {
    typedef volatile unsigned char* (*FlagsFn)(void);
    typedef void (*SetHookFn)(NativeHookFn);
    typedef u32 (*CountFn)(void);
    typedef u32 (*WritesFn)(OptionWriteFn, void*);
    FlagsFn flags = (FlagsFn)dlsym(lib, "bluewake_composite_option_flags");
    SetHookFn set_hook = (SetHookFn)dlsym(lib, "bluewake_composite_set_native_hook");
    CountFn count = (CountFn)dlsym(lib, "bluewake_composite_option_count");
    OptionFn option = (OptionFn)dlsym(lib, "bluewake_composite_option");
    WritesFn writes = (WritesFn)dlsym(lib, "bluewake_composite_option_writes");
    const u32 options = count ? count() : 0u;
    if (flags == NULL || set_hook == NULL || option == NULL || options == 0u)
        return;
    g_flags = flags();
    g_option = option;
    g_cpu = cpu;
    set_hook(native_hook);

    const char* wanted = getenv("BLUEWAKE_OPTIONS");
    const bool none = wanted != NULL && strncmp(wanted, "none", 4) == 0;
    u32 swift = 0u;
    char list[768] = "";
    for (u32 i = 0; i < options; ++i) {
        u32 index = 0u, default_on = 0u;
        const char* name = option(i, &index, &default_on, NULL);
        bool on = default_on != 0u && !none;
        named(wanted, name, &on);
        on = on && mod_enabled;
        if (strcmp(name, "instant_text") == 0)
            g_instant_text = index;
        else if (strcmp(name, "brisk_sail") == 0)
            g_brisk_sail = index;
        else if (strcmp(name, "invert_camera_x") == 0)
            g_invert_camera_x = index;
        else if (strcmp(name, "swift_sail") == 0)
            swift = index;
        g_flags[index] = on ? 1u : 0u;
        if (on)
            snprintf(list + strlen(list), sizeof list - strlen(list), "%s%s", list[0] ? "," : "", name);
    }
    // Brisk Sail is Swift Sail with harder braking: it needs Swift Sail's sites.
    if (option_on(g_brisk_sail) && swift != 0u)
        g_flags[swift] = 1u;
    const u32 written = writes ? writes(write_guest, cpu) : 0u;
    fprintf(stderr, "[options] %s (%u values written)\n", list[0] ? list : "none", written);
}

bool bluewake_game_options_invert_camera_x(void) { return option_on(g_invert_camera_x); }

const char* bluewake_game_options_describe(u32 position, const char** title, bool* default_on, bool* on) {
    u32 index = 0u, defaults = 0u;
    const char* text = NULL;
    const char* name = g_option != NULL ? g_option(position, &index, &defaults, &text) : NULL;
    if (name == NULL)
        return NULL;
    if (title)
        *title = text;
    if (default_on)
        *default_on = defaults != 0u;
    if (on)
        *on = option_on(index);
    return name;
}

// Instant text: what Better Wind Waker does to the message data on the disc,
// done to the copy the game has loaded. Every message draws its whole box at
// once (initial draw type 1), and its timed waits (1A 07 00 00 07 nn nn) and
// timed waits before a prompt (1A 07 00 00 03 nn nn) wait no time.
static u32 g_bmg;
static u64 g_retrace;

typedef struct BmgGuestWrite {
    CPUState* cpu;
    u32 base;
} BmgGuestWrite;

static void write_bmg_byte(void* user, size_t offset, uint8_t value) {
    BmgGuestWrite* write = user;
    mem_write8(write->cpu, write->base + (u32)offset, value);
}

static bool patch_bmg(CPUState* cpu, u32 bmg, u32* messages_out, u32* waits_out) {
    const u32 offset = bmg - 0x80000000u;
    if (cpu == NULL || cpu->ram == NULL || offset >= cpu->ram_size)
        return false;
    BmgGuestWrite write = {cpu, bmg};
    return bluewake_bmg_patch(cpu->ram + offset, cpu->ram_size - offset,
                             write_bmg_byte, &write, messages_out, waits_out);
}

void bluewake_game_options_retrace(CPUState* cpu) {
    if (!option_on(g_instant_text) || cpu->ram == NULL || (++g_retrace % 60u) != 0u)
        return;
    if (g_bmg != 0u && mem_read32(cpu, g_bmg) == 0x4D455347u && mem_read32(cpu, g_bmg + 4u) == 0x626D6731u)
        return;
    // Not found yet, or the copy moved: find the loaded message data
    // ("MESGbmg1") and patch every copy.
    g_bmg = 0u;
    static u32 scans;
    if (++scans == 1u || scans % 60u == 0u)
        fprintf(stderr, "[options] instant text: looking for the messages (scan %u, %u bytes)\n", scans,
                cpu->ram_size);
    const u8* ram = cpu->ram;
    for (u32 offset = 0; offset + 8u <= cpu->ram_size;) {
        const u8* hit = memmem(ram + offset, cpu->ram_size - offset, "MESGbmg1", 8);
        if (hit == NULL)
            break;
        const u32 address = 0x80000000u + (u32)(hit - ram);
        u32 messages = 0u, waits = 0u;
        if (patch_bmg(cpu, address, &messages, &waits)) {
            fprintf(stderr, "[options] instant text: %u messages at 0x%08X, %u timed waits removed\n",
                    messages, address, waits);
            g_bmg = address;
        }
        offset = (u32)(hit - ram) + 8u;
    }
}
