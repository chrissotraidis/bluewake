#include "hd_look.h"

#include <aurora/aurora.h>
#include <gxruntime/aurora_backend.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool bluewake_hd_distant_islands;
static int g_clouds = -1;  // cloud shadows: 1, 0, or -1 left to the renderer
static bool g_clouds_shown;
static int g_sun = -1;     // sun shadows, the same
static bool g_sun_shown;
static int g_edge = -1;    // edge light, the same
static bool g_edge_shown;
static int g_torch = -1;   // torch light, the same (on every stage)
static bool g_torch_shown;
static bool g_islands_were_on;
static int g_palm_cull = -1; // BLUEWAKE_HD_PALM_CULL: 1, 0, or -1 for the high shadow quality's default
static bool g_palms;       // palm trees culled by their shadow's reach (with the room culling, or the switch)
static bool g_palms_were_on;
static int g_room_cull = -1;  // BLUEWAKE_HD_ROOM_CULL: 1, 0, or -1 for the high shadow quality's default
bool bluewake_hd_room_clip;
static float g_sun_view[3] = {0.0f, 1.0f, 0.0f};  // toward the sun, in the camera's space (room culling)
static float g_sun_up = 1.0f;  // the sun's direction's height (sine of its elevation)
static bool g_trace;
// BLUEWAKE_HD_FIXED_SPLITS=1 (testing): the HD shadows keep their fixed cascade splits, not HD's from the camera.
static bool g_fixed_splits;
static bool g_sky;  // HD's sky and light palettes (BLUEWAKE_HD_SKY)
static bool g_monotone;  // HD's monotone in Frozen Hyrule (BLUEWAKE_HD_MONOTONE, on with the look; monotone())
static bool g_sky_models;  // with them, HD's sky models (BLUEWAKE_HD_SKY_MODELS=1; off by default)
static bool g_sky_linear;  // and HD's blend in linear light (BLUEWAKE_HD_SKY_LINEAR=1; on with an imported pack)
static bool g_fog_curve = true;  // and HD's fog curve (BLUEWAKE_HD_FOG_CURVE=0: the GameCube's, to compare)
static void sky_load(void);

// HD's sunlight as the player chose it (BLUEWAKE_HD_SUNLIGHT, read at each
// reload): the power its factor for the hour is taken to (sky_brightness()).
static float g_sunlight = 0.4545f;
// HD's factor for the hour as last worked out (sky_brightness()), so a new
// choice of sunlight shows at once, the game paused under the menu too.
static float g_sky_factor;
static void sky_sunlight_changed(void);

// A number setting, or `fallback` when it is unset or not a number in [lo, hi].
static float setting_number(const char* key, float fallback, float lo, float hi) {
    const char* value = getenv(key);
    if (value == NULL || value[0] == '\0')
        return fallback;
    char* end = NULL;
    const float number = strtof(value, &end);
    return end != value && number >= lo && number <= hi ? number : fallback;
}

static int setting(const char* key) {  // 1, 0, or -1 when unset
    const char* value = getenv(key);
    if (value == NULL || value[0] == '\0')
        return -1;
    return value[0] == '1';
}

// A part of the HD look: its own switch, else the look's. Turning the look off
// (F2, or BLUEWAKE_HD_LOOK=0) takes every part with it, whatever its switch
// says; with the look unset, a part's own variable alone still turns it on.
static int part(const char* key, int all) {
    if (all == 0)
        return 0;
    return setting(key) >= 0 ? setting(key) : all;
}

// The HD look's strengths, softer than Aurora's own starting values: at full
// strength the look came out darker than the GameCube's, with a dark smudge
// under Link. Aurora reads these once when it starts, so they are set before
// that (main.c), whether or not the look is on yet; a value already set wins.
// The far blur leaves the sky (no depth) sharp: blurred, its clouds looked out
// of focus and the wind's lines faded. Torch light at Aurora's 0.35 washed the
// Dragon Roost Cavern's walls pale yellow beside each torch.
void bluewake_hd_look_defaults(void) {
    setenv("DOL_AURORA_AO_STRENGTH", "0.3", 0);
    setenv("DOL_AURORA_CLOUD_SHADOWS_STRENGTH", "0.15", 0);
    setenv("DOL_AURORA_BLOOM_STRENGTH", "0.3", 0);
    setenv("DOL_AURORA_FAR_BLUR_AMOUNT", "0.4", 0);
    setenv("DOL_AURORA_FAR_BLUR_BACKGROUND", "0", 0);
    setenv("DOL_AURORA_TORCH_LIGHT_STRENGTH", "0.2", 0);
    // HD's sky clouds (BLUEWAKE_HD_CLOUDS, off by default, not on with the
    // look): the importer keeps them in the pack's optional/hd-clouds, which
    // Aurora loads over the pack only when asked, so they come only with the
    // HD textures and the look both on. Asked for here when BlueWake starts
    // with both on; bluewake_hd_look_reload() then loads or drops them as the
    // look is turned on and off.
    const char* pack = getenv("DOL_AURORA_TEXTURE_PACK");
    if (setting("BLUEWAKE_HD_CLOUDS") == 1 && setting("BLUEWAKE_HD_LOOK") == 1 && pack != NULL && pack[0] != '\0')
        setenv("DOL_AURORA_TEXTURE_PACK_OPTIONAL", "hd-clouds", 0);
    // Wind Waker HD's own versions of the look's parts (RecompCore 0206-0213,
    // each behind a switch of its own, off in Aurora), as the Windows test
    // launcher sets them, with the look on when BlueWake starts and a pack
    // imported from the player's HD disc: they follow HD's own normals and
    // colours, which only the importer writes (optional/hd-normals). A pack of
    // textures alone keeps the look as it was. Each acts only under its part
    // (sun shadows, bloom, AO, cloud shadows), so the parts' own defaults are
    // unchanged. A value already set wins; an empty one (the settings file's)
    // reads as unset. Aurora reads these once, so the look turned on later
    // (F2) goes without them until BlueWake starts again.
    char normals[4096];
    snprintf(normals, sizeof normals, "%s/optional/hd-normals/objects.bin", pack != NULL ? pack : "");
    FILE* imported = pack != NULL && pack[0] != '\0' ? fopen(normals, "rb") : NULL;
    if (imported != NULL)
        fclose(imported);
    if (setting("BLUEWAKE_HD_LOOK") == 1 && imported != NULL) {
        static const char* const kHdLight[][2] = {
            {"BLUEWAKE_HD_SUN_RULE", "1"},
            {"DOL_AURORA_SUN_SHADOW_QUALITY", "hd"},
            {"DOL_AURORA_SUN_SHADOW_SHAPE", "0"},
            {"DOL_AURORA_SUN_SHADOW_MAP_HD_BIAS", "1"},
            {"DOL_AURORA_SUN_SHADOW_MAP_CASTER_BIAS", "80,3"},
            {"DOL_AURORA_SUN_SHADOW_HD_MIX", "1"},
            {"DOL_AURORA_SUN_SHADOW_SKINNED_SCENERY", "1"},
            {"DOL_AURORA_SUN_SHADOW_SMOOTH_AMBIENT", "1"},
            {"DOL_AURORA_SUN_SHADOW_HD_TERRAIN_LIGHT", "1"},
            {"DOL_AURORA_SUN_SHADOW_TEMPORAL", "1"},
            {"DOL_AURORA_SUN_SHADOW_TEMPORAL_RANGE", "100,400,0.9"},
            {"DOL_AURORA_BLOOM_HD", "1"},
            {"DOL_AURORA_BLOOM_HD_MASK", "1"},
            {"DOL_AURORA_BLOOM_HD_STRENGTH", "0.6"},
            {"DOL_AURORA_EXPOSURE_HIGHLIGHTS", "0.99,1.0"},
            {"DOL_AURORA_AO_HD", "1"},
            {"DOL_AURORA_CLOUD_SHADOWS_HD", "1"},
            {"DOL_AURORA_SUN_SHADOW_FACING", "1"},
            {"DOL_AURORA_SUN_SHADOW_FACING_MUL", "1.0"},
            {"DOL_AURORA_SUN_SHADOW_FACING_CREASE", "45"},
            // HD's rim light takes the edge light's place whatever the edge
            // light's switch says, so it needs that switch on.
            {"DOL_AURORA_RIM_HD", "1"},
            // HD's light on characters (its toon shader; needs HD's sky and light for the palette).
            {"DOL_AURORA_ACTOR_LIGHT_HD", "1"},
            // HD's far blur (its DOF presets wwHD_basic and wwHD_night): sharp to 25,000, then HD's linear ramp to
            // 50,000 times its level for the hour (far_blur_level()), under the look's amount.
            {"DOL_AURORA_FAR_BLUR_HD", "1"},
            {"DOL_AURORA_FAR_BLUR_START", "25000"},
            {"DOL_AURORA_FAR_BLUR_END", "50000"},
            // HD's palettes blended in linear light, as HD blends them (its dawn and dusk: the cirrus at 07:30,
            // the lavender at 17:30), with HD's sky and light on (it acts only under them).
            {"BLUEWAKE_HD_SKY_LINEAR", "1"},
            // HD's light buffer: the game's point and effect lights on HD's rule (point_lights()), added to the
            // characters' light (x 1.75) and the lit scenery's (x 1.25) in their own draws, in place of the torch
            // glow and of the GameCube's effect-light stage on the characters. It rides on HD's character light.
            {"DOL_AURORA_POINT_LIGHTS_HD", "1"},
        };
        const bool edge_light = setting("BLUEWAKE_HD_EDGE_LIGHT") != 0;
        const bool hd_sky = setting("BLUEWAKE_HD_SKY") != 0;
        for (size_t i = 0; i < sizeof kHdLight / sizeof kHdLight[0]; ++i) {
            const char* key = kHdLight[i][0];
            if (!edge_light && strcmp(key, "DOL_AURORA_RIM_HD") == 0)
                continue;
            if (!hd_sky && strcmp(key, "BLUEWAKE_HD_SKY_LINEAR") == 0)
                continue;
            const char* value = getenv(key);
            if (value == NULL || value[0] == '\0')
                setenv(key, kHdLight[i][1], 1);
        }
    }
}

void bluewake_hd_look_reload(void) {
    const int all = setting("BLUEWAKE_HD_LOOK");
    bluewake_hd_distant_islands = part("BLUEWAKE_HD_DISTANT_ISLANDS", all) == 1;
    g_trace = setting("BLUEWAKE_HD_TRACE") == 1;
    g_fixed_splits = setting("BLUEWAKE_HD_FIXED_SPLITS") == 1;
    // The renderer's parts (Aurora's post_fx). Left to DOL_AURORA_BLOOM and
    // DOL_AURORA_FAR_BLUR and DOL_AURORA_AO while neither the switch nor the part is set.
    const int bloom = part("BLUEWAKE_HD_BLOOM", all);
    const int far_blur = part("BLUEWAKE_HD_FAR_BLUR", all);
    const int ao = part("BLUEWAKE_HD_AO", all);
    // Cloud shadows stay off unless BLUEWAKE_HD_CLOUD_SHADOWS=1: at Outset at
    // 10:00 they darkened 61% of the picture by about 13%, the sea included,
    // where Wind Waker HD has no cloud shadow at all (its clouds' shadow is its
    // cloud texture over a floor, nothing where the sky is clear). Off, the
    // bridge, its shade and the sea there matched HD's within a few levels.
    g_clouds = all == 0 ? 0
               : setting("BLUEWAKE_HD_CLOUD_SHADOWS") >= 0 ? setting("BLUEWAKE_HD_CLOUD_SHADOWS") : all >= 0 ? 0 : -1;
    // Sun shadows are worked out from what is on screen, and on open ground
    // they drew long dark streaks, so the HD look leaves them off; only
    // BLUEWAKE_HD_SUN_SHADOWS=1 turns them on. They are part of the HD look,
    // so turning the look off (F2, or BLUEWAKE_HD_LOOK=0) takes them too.
    g_sun = all == 0 ? 0
            : setting("BLUEWAKE_HD_SUN_SHADOWS") >= 0 ? setting("BLUEWAKE_HD_SUN_SHADOWS") : all >= 0 ? 0 : -1;
    // Sun shadows come from a shadow map (Aurora's post_fx), so what is off
    // screen still casts. DOL_AURORA_SUN_SHADOW_MAP=0 keeps the screen's
    // version alone, for comparing the two.
    const int sun_map = setting("DOL_AURORA_SUN_SHADOW_MAP") >= 0 ? setting("DOL_AURORA_SUN_SHADOW_MAP") : g_sun;
    g_palm_cull = setting("BLUEWAKE_HD_PALM_CULL");
    g_room_cull = setting("BLUEWAKE_HD_ROOM_CULL");
    g_edge = part("BLUEWAKE_HD_EDGE_LIGHT", all);
    g_torch = part("BLUEWAKE_HD_TORCH_LIGHT", all);
    g_sky = all == 1 && setting("BLUEWAKE_HD_SKY") == 1;
    g_monotone = part("BLUEWAKE_HD_MONOTONE", all) == 1;
    // Off until turned on, the HD look on or not (they are new).
    g_sky_models = setting("BLUEWAKE_HD_SKY_MODELS") == 1;
    g_sky_linear = setting("BLUEWAKE_HD_SKY_LINEAR") == 1;
    g_fog_curve = setting("BLUEWAKE_HD_FOG_CURVE") != 0;
    if (g_sky)
        sky_load();
    // HD's sky clouds (the pack's optional/hd-clouds) go with the look at once: loaded over the pack while the look
    // and BLUEWAKE_HD_CLOUDS are on, dropped when either goes (F2), so the GameCube's own clouds, the bank on the
    // horizon included, come back. They loaded only at start before (bluewake_hd_look_defaults), and stayed.
    {
        const char* pack = getenv("DOL_AURORA_TEXTURE_PACK");
        dol_aurora_set_texture_pack_optional(
            "hd-clouds", all == 1 && setting("BLUEWAKE_HD_CLOUDS") == 1 && pack != NULL && pack[0] != '\0');
    }
    if (bloom >= 0)
        aurora_set_bloom(bloom == 1);
    if (far_blur >= 0)
        aurora_set_far_blur(far_blur == 1);
    if (ao >= 0)
        aurora_set_ambient_occlusion(ao == 1);
    if (sun_map >= 0)
        aurora_set_sun_shadow_map(sun_map == 1);
    // The shadow quality (DOL_AURORA_SUN_SHADOW_QUALITY, for testing; the menu
    // and the settings file keep none), handed over when it changes: the
    // renderer makes its map again at once.
    {
        static int s_quality = -1;
        const char* value = getenv("DOL_AURORA_SUN_SHADOW_QUALITY");
        // hd: Wind Waker HD's own shadow pass (its map size, cascades and pre-pass; local test).
        const int quality = value == NULL || value[0] == '\0' ? 2 : strcmp(value, "low") == 0 ? 0
                            : strcmp(value, "medium") == 0 ? 1 : strcmp(value, "hd") == 0 ? 3 : 2;
        if (quality != s_quality) {
            aurora_set_sun_shadow_quality(quality);
            s_quality = quality;
        }
    }
    // How dark the sun shadows are and how strong HD's sunlight is, as the
    // player chose in the menu (HD's own by default).
    aurora_set_sun_shadow_strength(setting_number("BLUEWAKE_HD_SHADOW_STRENGTH", 1.0f, 0.0f, 1.0f));
    const float sunlight = setting_number("BLUEWAKE_HD_SUNLIGHT", 0.4545f, 0.05f, 1.0f);
    if (sunlight != g_sunlight) {
        g_sunlight = sunlight;
        sky_sunlight_changed();
    }
    fprintf(stderr, "[hd-look] distant islands %s, ambient occlusion %s, sun shadows %s%s, edge light %s, torch light %s, cloud shadows %s, far blur %s, bloom %s\n",
            bluewake_hd_distant_islands ? "on" : "off", ao < 0 ? "unset" : ao ? "on" : "off",
            g_sun == 1 ? "on" : "off", g_sun == 1 && sun_map == 1 ? " (shadow map)" : "", g_edge == 1 ? "on" : "off", g_torch == 1 ? "on" : "off",
            g_clouds == 1 ? "on" : "off",
            far_blur < 0 ? "unset" : far_blur ? "on" : "off", bloom < 0 ? "unset" : bloom ? "on" : "off");
}

// GZLE01 revision 0. Every actor's create tag is on g_fopAcTg_Queue
// (node_list_class: head first); a tag is node_class (next at +0x08) with its
// actor at +0x0C. An actor's process name is at +0x08 and its scale at +0x214.
enum {
    kActorQueue = 0x80372028u,
    kNodeNext = 0x08u,
    kTagActor = 0x0Cu,
    kProcName = 0x08u,
    kScaleX = 0x214u,
    kScaleZ = 0x21Cu,
    kActorHeap = 0xF0u,
    kLodBg = 0x01C3u,  // fpcNm_LODBG_e, d_a_lod_bg
    kModelData = 0x2ACu, // daLodbg_c::mModelData
    kDataSize = 0x2BCu,  // mDataSize, then mDataSize2 (a second model on Windfall and the Forsaken Fortress)
    kDataSize2 = 0x2C0u,
    kMaxActors = 4096u,
};

// d_a_lod_bg's constructor makes scale.x the distance within which the island's
// far model is shown (its stage scale times 20,000) and scale.z, 20,000 more,
// the distance past which it is unloaded; it reads both every frame. HD shows
// the models 100,000 units further out. A changed actor keeps 20,001 between
// the two, so it is changed once and can be changed back, also in a save state.
static const float kExtra = 100000.0f;
static const float kGap = 20000.0f;
static const float kChangedGap = 20001.0f;

static bool guest_pointer(u32 address) { return address >= 0x80000000u && address < 0x81800000u; }

static float read_float(CPUState* cpu, u32 address) {
    const u32 bits = mem_read32(cpu, address);
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static void write_float(CPUState* cpu, u32 address, float value) {
    u32 bits;
    memcpy(&bits, &value, sizeof bits);
    mem_write32(cpu, address, bits);
}

// The island models' own heap. d_a_lod_bg keeps every far model in one
// 128 KB heap (JKRCreateExpHeap(0x20000, the archive heap)), which the game's
// distances fill only so far; a model that does not fit there takes the largest
// free block of the archive heap instead (loadModelData), and a long sail with
// more models shown left that heap in pieces, until a big archive such as
// Puppet Ganon's no longer found room and the game waited for it forever (#308).
// So the islands shown further out are only as many as fit: the game's own
// first, then the nearest of the others, up to kModelsBudget models loaded; and
// if a model is ever found outside that heap, the budget drops below the count
// at which it happened, and the farthest further-out islands are let go.
enum {
    kModelHeap = 0x2C4u,      // daLodbg_c::mDataHeap (JKRSolidHeap*)
    kHeapTreeList = 0x50u,    // JKRHeap::mChildTree's link: the parent heap's child list
    kHeapTree = 0x40u,        // ... which is the parent's mChildTree
    kHeapSize = 0x38u,        // JKRHeap::mSize
    kModelsBudget = 15u,      // 17 fitted where the most were loaded (sea room 20, October 12)
    kMaxIslands = 64u,
};
static unsigned g_models_budget = kModelsBudget;

// The heap a loaded model's data heap was made in.
static u32 parent_heap(CPUState* cpu, u32 heap) {
    if (!guest_pointer(heap))
        return 0u;
    const u32 list = mem_read32(cpu, heap + kHeapTreeList);
    return guest_pointer(list) ? list - kHeapTree : 0u;
}

static bool player_position(CPUState* cpu, float* x, float* z) {
    u32 node = mem_read32(cpu, kActorQueue);
    for (u32 n = 0; n < kMaxActors && guest_pointer(node); ++n, node = mem_read32(cpu, node + kNodeNext)) {
        const u32 actor = mem_read32(cpu, node + kTagActor);
        if (guest_pointer(actor) && mem_read16(cpu, actor + kProcName) == 0x00A9u) {  // fpcNm_PLAYER_e
            *x = read_float(cpu, actor + 0x1F8u);
            *z = read_float(cpu, actor + 0x200u);
            return true;
        }
    }
    return false;
}

static void islands(CPUState* cpu, bool on, unsigned long long retrace) {
    struct Island {
        u32 actor;
        float base, distance;
        bool extended, loaded;
        u32 parent;
    } island[kMaxIslands];
    unsigned count = 0, loaded = 0, spilled = 0, changed = 0;
    u32 bytes = 0;
    float px = 0.0f, pz = 0.0f;
    const bool player = player_position(cpu, &px, &pz);
    u32 node = mem_read32(cpu, kActorQueue);
    for (u32 n = 0; n < kMaxActors && guest_pointer(node) && count < kMaxIslands;
         ++n, node = mem_read32(cpu, node + kNodeNext)) {
        const u32 actor = mem_read32(cpu, node + kTagActor);
        if (!guest_pointer(actor) || mem_read16(cpu, actor + kProcName) != kLodBg)
            continue;
        struct Island* it = &island[count++];
        const float x = read_float(cpu, actor + kScaleX);
        const float gap = read_float(cpu, actor + kScaleZ) - x;
        it->actor = actor;
        it->extended = gap == kChangedGap;
        it->base = it->extended ? x - kExtra : x;
        const float dx = read_float(cpu, actor + 0x1F8u) - px, dz = read_float(cpu, actor + 0x200u) - pz;
        it->distance = player ? sqrtf(dx * dx + dz * dz) : 0.0f;
        it->loaded = mem_read32(cpu, actor + kModelData) != 0u;
        it->parent = it->loaded ? parent_heap(cpu, mem_read32(cpu, actor + kModelHeap)) : 0u;
        if (it->loaded) {
            loaded++;
            bytes += mem_read32(cpu, actor + kDataSize) + mem_read32(cpu, actor + kDataSize2);
        }
    }
    // The island heap is the 128 KB one; a model whose heap was made elsewhere spilled.
    for (unsigned i = 0; i < count; ++i) {
        if (island[i].loaded && island[i].parent != 0u &&
            mem_read32(cpu, island[i].parent + kHeapSize) > 0x20000u)
            spilled++;
    }
    if (on && spilled != 0u && loaded > spilled && loaded - spilled - 1u < g_models_budget) {
        g_models_budget = loaded - spilled - 1u;
        fprintf(stderr, "[hd-look] %u island model(s) outside the island heap with %u loaded: up to %u from now on\n",
                spilled, loaded, g_models_budget);
    }
    // The game's own islands first (within their own distance), then the
    // nearest of the others, while the budget lasts.
    unsigned budget_left = g_models_budget;
    for (unsigned i = 0; i < count; ++i)
        if (island[i].distance < island[i].base + kGap && budget_left > 0u)
            budget_left--;
    for (;;) {
        int best = -1;
        for (unsigned i = 0; i < count; ++i) {
            const struct Island* it = &island[i];
            if (it->distance < it->base + kGap || it->distance >= it->base + kExtra + kChangedGap || it->base < 0.0f)
                continue;  // the game's own, or too far even for HD
            if (it->actor == 0u)
                continue;  // already placed in this pass
            if (best < 0 || it->distance < island[best].distance)
                best = (int)i;
        }
        if (best < 0)
            break;
        struct Island* it = &island[best];
        const bool want = on && player && budget_left > 0u;
        if (want)
            budget_left--;
        if (want != it->extended) {
            write_float(cpu, it->actor + kScaleX, want ? it->base + kExtra : it->base);
            write_float(cpu, it->actor + kScaleZ, want ? it->base + kExtra + kChangedGap : it->base + kGap);
            changed++;
        }
        it->actor = 0u;
    }
    // The rest: within the game's own distances an island keeps the one it
    // has (taking it away there would hide a model the player sees); off, or
    // past even the further distance, it goes back to the game's.
    for (unsigned i = 0; i < count; ++i) {
        struct Island* it = &island[i];
        if (it->actor != 0u && it->extended && (!on || it->distance >= it->base + kExtra + kChangedGap)) {
            write_float(cpu, it->actor + kScaleX, it->base);
            write_float(cpu, it->actor + kScaleZ, it->base + kGap);
            changed++;
        }
    }
    static unsigned last_count = ~0u, last_loaded = ~0u, last_spilled = ~0u;
    if (g_trace && (count != last_count || loaded != last_loaded || spilled != last_spilled || changed)) {
        fprintf(stderr,
                "[hd-look] retrace=%llu island models: %u, loaded %u (%u bytes of model data, %u outside the island "
                "heap), changed now %u, budget %u\n",
                retrace, count, loaded, bytes, spilled, changed, g_models_budget);
        last_count = count;
        last_loaded = loaded;
        last_spilled = spilled;
    }
}

// BLUEWAKE_HD_TRACE=1: every 300 retraces, each island model within its show
// distance or loaded: its room, Link's distance, the distance, whether it is
// loaded and its fade (0 to 255).
static void trace_islands(CPUState* cpu, unsigned long long retrace) {
    float px = 0.0f, pz = 0.0f;
    bool player = false;
    u32 node = mem_read32(cpu, kActorQueue);
    for (u32 n = 0; n < kMaxActors && guest_pointer(node) && !player; ++n, node = mem_read32(cpu, node + kNodeNext)) {
        const u32 actor = mem_read32(cpu, node + kTagActor);
        if (guest_pointer(actor) && mem_read16(cpu, actor + kProcName) == 0x00A9u) {  // fpcNm_PLAYER_e
            px = read_float(cpu, actor + 0x1F8u);
            pz = read_float(cpu, actor + 0x200u);
            player = true;
        }
    }
    if (!player)
        return;
    node = mem_read32(cpu, kActorQueue);
    for (u32 n = 0; n < kMaxActors && guest_pointer(node); ++n, node = mem_read32(cpu, node + kNodeNext)) {
        const u32 actor = mem_read32(cpu, node + kTagActor);
        if (!guest_pointer(actor) || mem_read16(cpu, actor + kProcName) != kLodBg)
            continue;
        const float dx = read_float(cpu, actor + 0x1F8u) - px, dz = read_float(cpu, actor + 0x200u) - pz;
        const float dist = sqrtf(dx * dx + dz * dz), shown = read_float(cpu, actor + kScaleX);
        const u32 model = mem_read32(cpu, actor + 0x29Cu);
        if (dist < shown || model != 0u)
            fprintf(stderr, "[hd-look] retrace=%llu room %u: %.0f away, shown within %.0f, %s, fade %u, pause %u mount 0x%08X step 0x%08X heap 0x%08X\n", retrace,
                    mem_read32(cpu, actor + 0xB0u), dist, shown, model ? "loaded" : "not loaded",
                    mem_read8(cpu, actor + 0x2A8u), mem_read8(cpu, actor + 0x0Bu), mem_read32(cpu, actor + 0x2B4u),
                    mem_read32(cpu, actor + 0x298u), mem_read32(cpu, actor + 0x2C4u));
    }
}

// Sun shadows, edge light and cloud shadows: outdoors (the stage's type, from
// its STAG info: outdoors 0 or the sea 7), with the game's camera.
// GZLE01: g_dComIfG_gameInfo.play is at 0x803C5EA8; its mCurStage (the stage's
// name first) at +0x3E94 and mCurrentView (view_class*) at +0x4A58. In the
// view: mNear +0xC8, mFar +0xCC, mFovy (degrees) +0xD0, mAspect +0xD4, mInvViewMtx +0x170.
enum {
    kCurrentView = 0x803CA900u,
    kViewNear = 0xC8u,
    kViewFar = 0xCCu,
    kViewFovy = 0xD0u,
    kViewAspect = 0xD4u,
    kViewInvMtx = 0x170u,
    kStagInfo = 0x803C9DA0u,  // g_dComIfG_gameInfo.play.mStageData.mpStagInfo
    kSunPos = 0x803E5568u,  // g_env_light.mSunPos: the camera's eye plus the way to the sun
    kPointLights = 0x803E4EACu,  // g_env_light.mpPLights[200] (LIGHT_INFLUENCE*: pos, color s16, power)
};

// Wind Waker HD's shadow cascade splits for this frame (LayerShadow's ranges from its camera, calculateShadowRanges in
// the WWHD decompilation's hd_layer_shadow.cpp; its tuning, sLayerShadowTuning at 0x101F8994 in cking.rpx). HD's camera
// fields are the game's dCamera_c (camera_process_class + 0x244): mCenter +0x10, mEye +0x1C, mFovy +0x38, mCurMode
// +0x13C, the shakes mCenterShake +0x568, mEyeShake +0x574, mFovYShake +0x580. Its middle split is 2.5 times the
// camera's distance to what it looks at, less 4 x clamp(-2 x (fovy - 70), -100, 900), kept within 300 to 1000; its
// far split the middle plus 2.5 times (middle - 30), within 500 to 2500. Cameras in modes 4, 10 and 11 take 200 and
// 700. Left out: its event and cutscene ranges (fixed ones for a few events and Demo02). False when the view isn't
// the game's camera.
static bool hd_cascade_splits(CPUState* cpu, u32 view, float* middle, float* far_split) {
    const u32 cam = view + 0x244u;
    float c[3], e[3];
    for (u32 i = 0; i < 3; ++i) {
        c[i] = read_float(cpu, cam + 0x10u + i * 4u) + read_float(cpu, cam + 0x568u + i * 4u);
        e[i] = read_float(cpu, cam + 0x1Cu + i * 4u) + read_float(cpu, cam + 0x574u + i * 4u);
    }
    const float fovy = read_float(cpu, cam + 0x38u) + read_float(cpu, cam + 0x580u);
    const u32 mode = mem_read32(cpu, cam + 0x13Cu);
    const float dx = c[0] - e[0], dy = c[1] - e[1], dz = c[2] - e[2];
    const float distance = sqrtf(dx * dx + dy * dy + dz * dz);
    // Not the game's camera (the view's own field of view disagrees, or nothing sensible is there).
    if (!(distance >= 0.f && distance < 100000.f && fovy > 1.f && fovy < 170.f) ||
        fabsf(fovy - read_float(cpu, view + kViewFovy)) > 20.f)
        return false;
    if (mode == 4u || mode == 10u || mode == 11u) {
        *middle = 200.f;
        *far_split = 700.f;
        return true;
    }
    float adjust = -(fovy - 70.f) * 2.f;
    adjust = adjust < -100.f ? -100.f : adjust > 900.f ? 900.f : adjust;
    float m = distance * 2.5f - 4.f * adjust;
    m = m < 300.f ? 300.f : m > 1000.f ? 1000.f : m;
    float reach = (m - 30.f) * 2.5f;
    reach = reach < 500.f ? 500.f : reach > 2500.f ? 2500.f : reach;
    *middle = m;
    *far_split = m + reach;
    return true;
}

// Wind Waker HD's light buffer (DOL_AURORA_POINT_LIGHTS_HD=1, which the look sets with a pack imported from the HD
// disc; read once). HD sends every light to its light buffer each frame (dKy_setLight, its d_kankyo_5.cpp:564-607):
// the 200 point lights (mpPLights) with r = 2.3 x power (1.25 x in Obshop, the Bomb Shop), and the 10 effect lights
// (mpEfLights: bombs, chests, the sword-hit flash, the thunder) with r = power; colour the S10 colour / 255 without a
// clamp (a lamp's 600 is 2.35), exponent 1.5. Off, the lights go as before to Aurora's torch glow.
static bool point_lights_hd(void) {
    static int s_on = -1;
    if (s_on < 0)
        s_on = setting("DOL_AURORA_POINT_LIGHTS_HD") == 1;
    return s_on == 1;
}

enum {
    kEfLights = 0x803E51CCu,        // g_env_light.mpEfLights[10] (g_env_light 0x803E4AB4 + 0x718)
    kSwordProc = 0x803E5218u,       // mEfLightProc (+0x764): mSwordState u8, mSwordFrame u8, mSwordLightType int at +4
    kSwordLight = kSwordProc + 8u,  // its LIGHT_INFLUENCE mSwordLight
    kThunderTimer = 0x803E54ECu,    // mThunderEff (+0xA30).mFlashTimer (+0x08)
    kThunderLight = 0x803E54FCu,    // mThunderEff.mLightInfluence (+0x18)
    kMaxPointLights = 32u,
};

// Wind Waker HD's sword-hit flash colours, one a frame from the start (its frameFlushColors and frameFlushColors2,
// read from Cemu: drafts/hd-look-remaining/CAPTURES-NOV3.md, shots 4 and 4b); after the last, black. Type 0 is
// blue from the first frame (the GameCube's flush_col starts white), type 1 orange; HD's power is 700 (the
// GameCube's 1000).
static const unsigned char kSwordFlush[2][12][3] = {
    {{21, 197, 255}, {11, 174, 255}, {0, 150, 255}, {0, 114, 207}, {0, 77, 158}, {0, 40, 109}, {0, 34, 91},
     {0, 27, 73}, {0, 20, 55}, {0, 14, 37}, {0, 7, 19}, {0, 0, 0}},
    {{255, 197, 67}, {255, 174, 34}, {255, 150, 0}, {211, 114, 0}, {166, 77, 0}, {120, 40, 0}, {100, 34, 0},
     {81, 27, 0}, {60, 20, 0}, {41, 14, 0}, {20, 7, 0}, {0, 0, 0}},
};

// Wind Waker HD's own powers for three of the game's flames (its actors set them; the light buffer then takes
// HD's 2.3 x power): a torch (d_a_ep) and a Bokoblin's torch stick (d_a_boko) at 15/23 of the GameCube's in stage
// types 1 and 4 (the dungeons: Dragon Roost Cavern's wall torches 690 -> 450, as HD's capture has them), and a
// lantern's flame (d_a_kantera) at 225 for the GameCube's 375. The light's owner is the actor it sits in (its
// mLight at 0x510, 0x300 and 0x2F0), told by the actor's process name. HD's torches also flicker (x 0.9 to 1.1),
// which is left out. 1 for any other light.
static float hd_light_power_scale(CPUState* cpu, u32 light, u32 stage_type) {
    static const struct {
        u32 offset;
        u16 proc;
        bool dungeon_only;
        float scale;
    } kOwners[] = {
        {0x510u, 0x00BAu, true, 15.f / 23.f},   // ep_class::mLight, fpcNm_EP_e
        {0x300u, 0x01D5u, true, 15.f / 23.f},   // daBoko_c::mLight, fpcNm_BOKO_e
        {0x2F0u, 0x00C2u, false, 225.f / 375.f}, // kantera_class::mPlight, fpcNm_KANTERA_e
    };
    const bool dungeon = stage_type == 1u || stage_type == 4u;
    for (size_t i = 0; i < sizeof kOwners / sizeof kOwners[0]; ++i) {
        if (kOwners[i].dungeon_only && !dungeon)
            continue;
        const u32 actor = light - kOwners[i].offset;
        if (light < kOwners[i].offset || !guest_pointer(actor))
            continue;
        if (mem_read16(cpu, actor + kProcName) == kOwners[i].proc)
            return kOwners[i].scale;
    }
    return 1.f;
}

// The game's lights nearest the camera for the renderer: x, y, z, radius, r, g, b, flicker each (up to 16, or 32 on
// HD's rule), nearest by the distance to the eye less the radius.
static void point_lights(CPUState* cpu, const float eye[3]) {
    float found[kMaxPointLights * 8];
    float distance[kMaxPointLights];
    const bool hd = point_lights_hd();
    const unsigned cap = hd ? kMaxPointLights : 16u;
    float scale = 1.f;  // point lights' radius per unit of power
    u32 stage_type = 2u;
    if (hd) {
        char stage[8];
        for (u32 i = 0; i < 8u; ++i)
            stage[i] = (char)mem_read8(cpu, 0x803C9D3Cu + i);  // play.mCurStage.mName
        scale = strncmp(stage, "Obshop", 7) == 0 ? 1.25f : 2.3f;
        const u32 info = mem_read32(cpu, kStagInfo);
        stage_type = guest_pointer(info) ? (mem_read32(cpu, info + 0x0Cu) >> 16) & 7u : 2u;
    }
    unsigned count = 0;
    const u32 lists = hd ? 2u : 1u;
    for (u32 list = 0; list < lists; ++list) {
        const bool effect = list == 1u;
        const u32 entries = effect ? 10u : 200u;
        for (u32 i = 0; i < entries; ++i) {
            const u32 light = mem_read32(cpu, (effect ? kEfLights : kPointLights) + i * 4u);
            if (!guest_pointer(light))
                continue;
            const float x = read_float(cpu, light), y = read_float(cpu, light + 4u), z = read_float(cpu, light + 8u);
            float power = read_float(cpu, light + 0x14u);
            // The global light is 99,999 among the point lights; the thunder's 90,000 is an effect light.
            if (!(power > (hd ? 0.01f : 1.f) && power < (effect ? 1.0e6f : 10000.f)))
                continue;
            float colour[3];
            for (u32 c = 0; c < 3u; ++c)
                colour[c] = (float)(s16)mem_read16(cpu, light + 0x0Cu + c * 2u) / 255.f;
            if (effect && light == kSwordLight) {
                // The sword-hit flash on HD's table and power (from the GameCube's frame count; its state 2 counts).
                const u32 type = mem_read32(cpu, kSwordProc + 4u) != 0u ? 1u : 0u;
                const u32 frame = mem_read8(cpu, kSwordProc + 1u);
                const unsigned char* rgb = kSwordFlush[type][frame < 11u ? frame : 11u];
                for (u32 c = 0; c < 3u; ++c)
                    colour[c] = rgb[c] / 255.f;
                power = 700.f;
            } else if (effect && light == kThunderLight) {
                // The thunder flash: HD's colour (0.1, 0.4, 0.7) x 190 x the flash timer, (19, 77, 134) at the peak
                // (Cemu, shot 9); the GameCube's is about (36, 47, 51).
                float t = read_float(cpu, kThunderTimer);
                t = t > 0.f ? (t < 1.f ? t : 1.f) : 0.f;
                colour[0] = 0.1f * 190.f * t / 255.f;
                colour[1] = 0.4f * 190.f * t / 255.f;
                colour[2] = 0.7f * 190.f * t / 255.f;
            }
            const float radius = hd && !effect ? power * scale * hd_light_power_scale(cpu, light, stage_type) : power;
            if (colour[0] <= 0.f && colour[1] <= 0.f && colour[2] <= 0.f)
                continue;
            const float dx = x - eye[0], dy = y - eye[1], dz = z - eye[2];
            const float d = sqrtf(dx * dx + dy * dy + dz * dz) - radius;
            if (d > (hd ? 20000.f : 6000.f))
                continue;
            unsigned slot = count < cap ? count++ : cap;
            if (slot == cap) {  // full: replace the farthest if this one is nearer
                unsigned far = 0;
                for (unsigned k = 1; k < cap; ++k)
                    if (distance[k] > distance[far])
                        far = k;
                if (d >= distance[far])
                    continue;
                slot = far;
            }
            distance[slot] = d;
            float* out = found + slot * 8u;
            out[0] = x;
            out[1] = y;
            out[2] = z;
            out[3] = radius;
            for (u32 c = 0; c < 3u; ++c)
                out[4 + c] = hd ? (colour[c] < 0.f ? 0.f : colour[c])
                                : (colour[c] < 0.f ? 0.f : colour[c] > 1.f ? 1.f : colour[c]);
            out[7] = read_float(cpu, light + 0x18u) != 0.f ? 1.f : 0.f;
        }
    }
    aurora_set_point_lights(found, count);
    static unsigned traced;
    if (g_trace && traced++ % 300u == 0u) {
        fprintf(stderr, "[hd-look] point lights near the camera%s: %u", hd ? " (HD's rule)" : "", count);
        for (unsigned k = 0; k < count && k < 4u; ++k)
            fprintf(stderr, " (%.0f %.0f %.0f r%.0f c%.2f,%.2f,%.2f)", found[k * 8], found[k * 8 + 1], found[k * 8 + 2],
                    found[k * 8 + 3], found[k * 8 + 4], found[k * 8 + 5], found[k * 8 + 6]);
        fprintf(stderr, "\n");
    }
}

// Wind Waker HD's light for the sun shadows (BLUEWAKE_HD_SUN_RULE=1, local test; HD's setLightPosHD in its
// d_kankyo_3.cpp): from the camera's eye, by day (06:30-19:30) the sun at (sin a, -cos a, -0.6 cos a) x 80,000
// with a the time less 15 degrees, its height held at 40,000 before 09:00 and after 17:00; by night the
// opposite side, held at 40,000. Crossing 06:30 or 19:30, the light stays where it was while a fade rises
// (0.01 a game frame), changes sides at the top and the fade falls: the renderer takes the lit side down to
// the shade by it (aurora_set_sun_hd_light). Out: the direction toward the light.
// HD's day/night fade (mDayNightFade, EnvLight::setLightPosHD in its d_kankyo_3.cpp): crossing 06:30 or 19:30
// it rises 0.01 a game frame (addCalc 0.05, 0.01, 0.01: 100 frames), switches sides at 1 and falls again. While
// it is up HD takes every lit colour toward its shade (K0 to C0, the shadow colour to C0: fadeK0 and
// d_kankyo_6.cpp's TEV structs), and its shadows out. Worked out once a retrace, from hd_sun_rule (the light's
// freeze and side) and sky() (the colours), whichever comes first; a save state's load or a jump of the clock
// starts it again (no fade: the side is the clock's).
static struct {
    bool started, night;
    float fade, target, time;
    unsigned long long last, done;
} g_daynight;
static unsigned long long g_retrace;

static float hd_daynight_fade(CPUState* cpu) {
    if (g_daynight.started && g_daynight.done == g_retrace)
        return g_daynight.fade;
    g_daynight.done = g_retrace;
    const float t = read_float(cpu, 0x803E56ECu);  // g_env_light.mCurTime
    const bool day = t > 97.5f && t < 292.5f;
    unsigned long long frames =
        g_daynight.started && g_retrace > g_daynight.last ? (g_retrace - g_daynight.last) / 2u : 0u;
    if (frames > 0u || !g_daynight.started)
        g_daynight.last = g_retrace;
    if (frames > 200u)  // a long time unseen: the fade is over either way
        frames = 200u;
    // The clock jumping (a stage or file loading, a warp, the Song of Passing) starts it again, as a load: the
    // clock moves 0.01 degree a retrace, so a few degrees between two looks is no crossing HD would fade at.
    float moved = fabsf(t - g_daynight.time);
    moved = moved > 180.0f ? 360.0f - moved : moved;
    g_daynight.time = t;
    if (g_daynight.started && moved > 5.0f)
        g_daynight.started = false;
    if (!g_daynight.started) {
        g_daynight.started = true;
        g_daynight.night = !day;
        g_daynight.fade = g_daynight.target = 0.0f;
    }
    if (day == g_daynight.night) {  // crossed 06:30 or 19:30
        g_daynight.night = !day;
        g_daynight.target = 1.0f;
    }
    for (; frames > 0u; --frames) {
        float fade = g_daynight.fade + (g_daynight.target > g_daynight.fade ? 0.01f : -0.01f);
        g_daynight.fade = fade < 0.0f ? 0.0f : fade > 1.0f ? 1.0f : fade;
        if (g_daynight.fade >= 1.0f)
            g_daynight.target = 0.0f;
    }
    return g_daynight.fade;
}

static void hd_sun_rule(CPUState* cpu, float out[3]) {
    static float light[3];
    const float fade = hd_daynight_fade(cpu);
    const bool night = g_daynight.night;
    const float t = read_float(cpu, 0x803E56ECu);  // g_env_light.mCurTime
    const bool day = t > 97.5f && t < 292.5f;
    if (g_daynight.target <= 0.0f) {
        const float a = (t > 15.0f ? t - 15.0f : t + 345.0f) * 3.14159265f / 180.0f;
        const float x = sinf(a) * 80000.0f, z = cosf(a) * -48000.0f;
        if (day) {
            const float y = (t < 135.0f || t > 255.0f) ? cosf(240.0f * 3.14159265f / 180.0f) * 80000.0f : cosf(a) * 80000.0f;
            light[0] = x, light[1] = -y, light[2] = z;
        } else {
            light[0] = -x, light[1] = cosf(300.0f * 3.14159265f / 180.0f) * 80000.0f, light[2] = -z;
        }
    }
    out[0] = light[0], out[1] = light[1], out[2] = light[2];
    aurora_set_sun_hd_light(1, night ? 1 : 0, fade);
}

// Wind Waker HD's own normals for the stage in play, for the sun shadows' facing term, and its rooms' vertex
// colours for HD's light: the HD pack's optional/hd-normals/<stage>.bin and optional/hd-colours/<stage>.bin (made
// by the importer), handed to the renderer as the stage changes.
static void hd_normals_stage(CPUState* cpu) {
    static char s_stage[9];
    char stage[9] = {0};
    for (u32 i = 0; i < 8u; ++i)
        stage[i] = (char)mem_read8(cpu, 0x803C9D3Cu + i);  // play.mCurStage.mName
    if (strcmp(stage, s_stage) == 0)
        return;
    memcpy(s_stage, stage, sizeof s_stage);
    const char* pack = getenv("DOL_AURORA_TEXTURE_PACK");
    char path[4096];
    if (stage[0] == '\0' || pack == NULL || pack[0] == '\0') {
        aurora_set_hd_normals(NULL);
        aurora_set_hd_stage_colours(NULL);
        return;
    }
    snprintf(path, sizeof path, "%s/optional/hd-normals/%s.bin", pack, stage);
    FILE* f = fopen(path, "rb");
    if (f != NULL)
        fclose(f);
    aurora_set_hd_normals(f != NULL ? path : NULL);
    if (g_trace)
        fprintf(stderr, "[hd-look] HD normals for %s: %s\n", stage, f != NULL ? path : "none");
    // And its rooms' own vertex colours (HD remade them), optional/hd-colours/<stage>.bin.
    snprintf(path, sizeof path, "%s/optional/hd-colours/%s.bin", pack, stage);
    f = fopen(path, "rb");
    if (f != NULL)
        fclose(f);
    aurora_set_hd_stage_colours(f != NULL ? path : NULL);
}

// Wind Waker HD's cloud shadow texture for the cloud shadows on HD's rule: the HD pack's
// optional/hd-cloud-shadow/ww_shadow_clouds.dds (made by the importer), handed to the renderer once when the pack
// has it (without it the renderer keeps its own noise, or DOL_AURORA_CLOUD_SHADOWS_HD_TEXTURE's file).
static void hd_cloud_texture(void) {
    static bool s_done;
    if (s_done)
        return;
    s_done = true;
    const char* pack = getenv("DOL_AURORA_TEXTURE_PACK");
    if (pack == NULL || pack[0] == '\0')
        return;
    char path[4096];
    snprintf(path, sizeof path, "%s/optional/hd-cloud-shadow/ww_shadow_clouds.dds", pack);
    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return;
    fclose(f);
    aurora_set_hd_cloud_texture(path);
    if (g_trace)
        fprintf(stderr, "[hd-look] HD cloud shadow texture: %s\n", path);
}

// Wind Waker HD's normals and vertex colours for the game's object models (palms, rocks, trees): the HD pack's
// optional/hd-normals/objects.bin and optional/hd-colours/objects.bin (made by the importer), handed to the renderer
// once. BLUEWAKE_HD_OBJECT_NORMALS=0 leaves them out (testing).
static void hd_object_tables(void) {
    static bool s_done;
    if (s_done)
        return;
    s_done = true;
    const char* pack = getenv("DOL_AURORA_TEXTURE_PACK");
    if (pack == NULL || pack[0] == '\0' || setting("BLUEWAKE_HD_OBJECT_NORMALS") == 0)
        return;
    char normals[4096], colours[4096];
    snprintf(normals, sizeof normals, "%s/optional/hd-normals/objects.bin", pack);
    snprintf(colours, sizeof colours, "%s/optional/hd-colours/objects.bin", pack);
    aurora_set_hd_object_tables(normals, colours);
    if (g_trace)
        fprintf(stderr, "[hd-look] HD object normals and colours from %s/optional\n", pack);
}

// Wind Waker HD's cloud shadow floor for the hour (cCloudShadowPower, its setSunpos in d_kankyo_3.cpp): from the
// game's clock, 1 - max(0.25, -0.9 cos(time - 15 degrees)), 0.75 at 07:30 and 18:30 and through the night, 0.364 at
// 10:00, 0.131 at noon. The renderer's own is worked out from the shadow light's height, which HD's light rule holds
// at 40,000 high before 09:00 and after 17:00 (0.596 at dawn and dusk: a cloud took 1.6 times HD's light from Link
// and the scenery there). Sent while the cloud shadows show, else the renderer's own again.
static void cloud_floor(CPUState* cpu, bool on) {
    static float s_sent = -1.0f;
    float floor = -1.0f;
    if (on) {
        const float t = read_float(cpu, 0x803E56ECu);  // g_env_light.mCurTime
        if (t >= 0.0f && t <= 360.0f) {
            const float sun = -0.9f * cosf((t - 15.0f) * 3.14159265f / 180.0f);
            floor = 1.0f - (sun > 0.25f ? sun : 0.25f);
        }
    }
    if (fabsf(floor - s_sent) > 0.0005f) {
        aurora_set_cloud_hd_floor(floor);
        s_sent = floor;
    }
}

// Wind Waker HD's room lights (the HD pack's optional/hd-sky/room-lights.bin, made by the importer from each
// stage's baseLight.bin): for each stage, 64 pitches then 64 yaws (big-endian s16), one a room. HD's
// setLightPosHD (kankyo3_roomLight, d_kankyo_3.cpp) lights the room Link is in from (cos p sin y, sin p,
// cos p cos y) when its angles are set, and from the sun by the clock when they aren't; never in M2tower.
// Golden values: Dragon Roost Cavern's room 0 (-0.477, 0.811, 0.338), its shadow pass's light in Cemu.
typedef struct RoomLights {
    char name[16];
    u8 angles[256];
} RoomLights;
static RoomLights* g_room_lights;
static u32 g_room_light_count;

static void room_lights_load(void) {
    static bool s_tried;
    if (s_tried)
        return;
    s_tried = true;
    const char* pack = getenv("DOL_AURORA_TEXTURE_PACK");
    if (pack == NULL || pack[0] == '\0')
        return;
    char path[4096];
    snprintf(path, sizeof path, "%s/optional/hd-sky/room-lights.bin", pack);
    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return;
    u8 head[8];
    if (fread(head, 1, 8, f) == 8 && memcmp(head, "BWRL", 4) == 0) {
        const u32 count = (u32)head[4] << 24 | (u32)head[5] << 16 | (u32)head[6] << 8 | head[7];
        if (count > 0u && count <= 1024u && (g_room_lights = calloc(count, sizeof *g_room_lights)) != NULL) {
            for (; g_room_light_count < count; ++g_room_light_count) {
                RoomLights* r = &g_room_lights[g_room_light_count];
                if (fread(r->name, 1, 16, f) != 16 || fread(r->angles, 1, 256, f) != 256)
                    break;
                r->name[15] = '\0';
            }
        }
    }
    fclose(f);
    if (g_trace)
        fprintf(stderr, "[hd-look] HD room lights: %u stages from %s\n", g_room_light_count, path);
}

// The light HD lights the room Link is in from (the direction toward it), or false for HD's sun by the clock.
// A room change (Link through a door): the scenery's light, which the host takes from the room Link is in
// (mStayNo: the palettes of sky_blend_state's scenery side, HD's brightness for them, the room's light), fades
// from the room he left to the new one over kRoomFadeRetraces, as the game blends its own light from the floor's
// palettes when he crosses (its mColPatBlend, 42 retraces in kindan). Changed at once, all of the picture jumped
// when Forbidden Woods' door into its clearing (room 1 to 2) opened: HD's brightness 1.00 to 1.65, the shade's
// ground and lit colours, the room light's direction. In HD each room's scenery keeps its own light throughout
// (its tevstr), so there is no change at all there; the host has one light for the picture. The room light's
// direction still changes at once (room_light): turned through the fade, the shadows swept across the clearing.
enum { kRoomFadeRetraces = 42 };
static struct {
    char stage[8];
    int from, to;  // the room left (-1: no fade) and the one Link is in
    unsigned long long start;
} g_room_fade = {{0}, -1, -1, 0};

static void room_fade_update(CPUState* cpu) {
    char stage[8];
    for (u32 i = 0; i < 8u; ++i)
        stage[i] = (char)mem_read8(cpu, 0x803C9D3Cu + i);  // play.mCurStage.mName
    const int room = (s8)mem_read8(cpu, 0x803F6A78u);   // dStage_roomControl_c::mStayNo
    if (memcmp(stage, g_room_fade.stage, sizeof stage) != 0 || room < 0 || g_room_fade.to < 0) {
        memcpy(g_room_fade.stage, stage, sizeof stage);
        g_room_fade.from = -1;
        g_room_fade.to = room;
        return;
    }
    if (room == g_room_fade.to)
        return;
    // Back through the door before the fade ended: the way back from where it got to.
    const unsigned long long done = g_retrace - g_room_fade.start;
    const unsigned long long back = room == g_room_fade.from && done < kRoomFadeRetraces ? kRoomFadeRetraces - done : 0u;
    g_room_fade.from = g_room_fade.to;
    g_room_fade.to = room;
    g_room_fade.start = g_retrace - back;
}

// How far the fade into the room Link is in has got (1: done, *from -1), and the room it comes from;
// none while held (the rooms' own palette entries, sky_linear).
static bool g_room_fade_held;
static float room_fade(int* from) {
    *from = -1;
    if (g_room_fade.from < 0 || g_room_fade_held)
        return 1.0f;
    const float t = (float)(g_retrace - g_room_fade.start) / (float)kRoomFadeRetraces;
    if (t >= 1.0f) {
        g_room_fade.from = -1;
        return 1.0f;
    }
    *from = g_room_fade.from;
    return t < 0.0f ? 0.0f : t;
}

static bool room_light(CPUState* cpu, float out[3]) {
    room_lights_load();
    char stage[9] = {0};
    for (u32 i = 0; i < 8u; ++i)
        stage[i] = (char)mem_read8(cpu, 0x803C9D3Cu + i);  // play.mCurStage.mName
    const int room = (s8)mem_read8(cpu, 0x803F6A78u);       // dStage_roomControl_c::mStayNo
    if (room < 0 || room >= 64 || strcmp(stage, "M2tower") == 0)
        return false;
    for (u32 i = 0; i < g_room_light_count; ++i) {
        const RoomLights* r = &g_room_lights[i];
        if (strncmp(r->name, stage, 16) != 0)
            continue;
        const int pitch = (s16)((u32)r->angles[room * 2] << 8 | r->angles[room * 2 + 1]);
        const int yaw = (s16)((u32)r->angles[128 + room * 2] << 8 | r->angles[128 + room * 2 + 1]);
        if (pitch == 0 && yaw == 0)
            return false;
        const float p = (float)(u16)pitch * (6.28318531f / 65536.0f), y = (float)(u16)yaw * (6.28318531f / 65536.0f);
        out[0] = cosf(p) * sinf(y);
        out[1] = fabsf(sinf(p));  // getShadowDirection turns a light from below up
        out[2] = cosf(p) * cosf(y);
        return true;
    }
    return false;
}

// HD lights its dungeons' rooms as its outdoor scenery, without the clouds and the edge light (its room shader,
// captures-nov3 s01/s02: hemisphere ambient, N.L to the room's light, the room light's shadow map and SSAO): the
// stage types HD's setLightPosHD takes for dungeons (1 dungeon, 3 boss, 4 subdungeon, 6 miniboss), while HD's
// palettes are in (its colours for the room). The casters are culled as HD culls them there
// (aurora_set_sun_caster_cull), so a closed room's ceiling doesn't shade its floor, and HD's torches come from its
// light buffer. BLUEWAKE_HD_ROOM_SHADOWS=0 keeps the rooms as before (the sun shadows outdoors only).
static bool sky_palettes_shown(void);
static bool hd_room(u32 type) {
    static int s_on = -1;
    if (s_on < 0) {
        const char* value = getenv("BLUEWAKE_HD_ROOM_SHADOWS");
        s_on = value == NULL || value[0] != '0';
    }
    return s_on && (type == 1u || type == 3u || type == 4u || type == 6u) && sky_palettes_shown();
}

// The game's glow around torches, lanterns and Bokoblins' sticks: spheres into the frame's alpha, then its colour
// over them (dDlst_alphaModel_c, at most 64 a frame). HD draws none (d_a_ep, d_a_kantera, d_a_boko, d_a_bpw: no
// alpha model); its light buffer lights the walls instead, so with HD's rule for it the game gets no room for them,
// and the walls aren't lit twice. Its room back (64) when the light buffer is off.
static void alpha_model_glows(CPUState* cpu, bool on) {
    const u32 model = mem_read32(cpu, 0x803CAB68u);  // g_dComIfG_gameInfo.drawlist (+0x5D1C).mpAlphaModel (+0x244)
    if (!guest_pointer(model))
        return;
    const u16 capacity = on ? 0x40u : 0u;
    if (mem_read16(cpu, model + 4u) != capacity) {  // mCapacity
        mem_write16(cpu, model + 4u, capacity);
        mem_write16(cpu, model + 6u, 0u);  // mNum
    }
}

static void clouds(CPUState* cpu, unsigned long long retrace) {
    hd_normals_stage(cpu);
    hd_cloud_texture();
    hd_object_tables();
    bool show = false, sun_show = false;
    bool torch_view = false;
    if (g_clouds == 1 || g_sun == 1 || g_edge == 1 || g_torch == 1) {
        const u32 info = mem_read32(cpu, kStagInfo);
        const u32 type = guest_pointer(info) ? (mem_read32(cpu, info + 0x0Cu) >> 16) & 7u : 2u;
        const u32 view = mem_read32(cpu, kCurrentView);
        const bool outdoors = type == 0u || type == 7u;
        const bool room = !outdoors && hd_room(type);
        // HD's casters culled as their materials are, wherever its sun shadows are (its maps in Dragon Roost
        // Cavern, at Outset and in the Forest Haven: back faces culled but for a double-sided material), so what
        // faces away from the light casts nothing: a closed room's ceiling, or the Forest Haven's (Omori, an
        // outdoor stage type), which shaded all of its floor with both sides cast.
        aurora_set_sun_caster_cull(outdoors || room ? 1 : 0);
        if (g_trace && retrace % 300u == 0u)
            fprintf(stderr, "[hd-look] stage type %u (%s)\n", type,
                    outdoors ? "outdoors" : room ? "indoors, HD's lit room" : "indoors");
        if ((outdoors || room || g_torch == 1) && guest_pointer(view)) {
            float inv[12];
            for (u32 i = 0; i < 12; ++i)
                inv[i] = read_float(cpu, view + kViewInvMtx + i * 4u);
            const float fovy = read_float(cpu, view + kViewFovy);
            const float aspect = read_float(cpu, view + kViewAspect);
            const float near_plane = read_float(cpu, view + kViewNear);
            if (fovy > 1.f && fovy < 170.f && aspect > 0.1f && near_plane > 0.f) {
                if (g_trace && retrace % 300u == 0u)
                    fprintf(stderr, "[hd-look] camera eye %.0f %.0f %.0f fovy %.1f aspect %.3f near %.2f far %.0f\n",
                            inv[3], inv[7], inv[11], fovy, aspect, near_plane, read_float(cpu, view + kViewFar));
                // The sea is at 0 outdoors; indoors there is none, and a floor near
                // 0 would read as sea to the passes that leave the sea alone (HD's
                // brightness for the hour), so the sea goes far below.
                aurora_set_world_view(inv, tanf(fovy * 0.5f * 3.14159265f / 180.f), aspect, near_plane,
                                      outdoors ? 0.f : -1.0e9f, (float)retrace / 59.94f);
                // With the far plane the sun shadow map reads distances from the
                // depth exactly: taken as infinitely far, an island 8,000 units
                // off read hundreds of units too far and shaded itself in blocks.
                const float far_plane = read_float(cpu, view + kViewFar);
                aurora_set_world_view_far(far_plane > near_plane ? far_plane : 0.f);
                // HD's shadow cascades follow its camera (BlueWake's zoom included).
                float split_middle = 0.f, split_far = 0.f;
                if (g_fixed_splits || !hd_cascade_splits(cpu, view, &split_middle, &split_far))
                    split_middle = split_far = 0.f;
                aurora_set_sun_cascade_splits(split_middle, split_far);
                const float sx = read_float(cpu, kSunPos) - inv[3];
                const float sy = read_float(cpu, kSunPos + 4u) - inv[7];
                const float sz = read_float(cpu, kSunPos + 8u) - inv[11];
                static int s_hd_rule = -1;
                if (s_hd_rule < 0) {
                    const char* rule = getenv("BLUEWAKE_HD_SUN_RULE");
                    s_hd_rule = rule != NULL && rule[0] == '1';
                }
                // HD's rule is for its sun shadows; with them off the edge light keeps the game's sun.
                float light[3];
                bool room_lit = false;
                if (s_hd_rule && g_sun == 1) {
                    room_lit = room && room_light(cpu, light);
                    if (room_lit) {
                        aurora_set_sun_direction(light[0], light[1], light[2]);
                        aurora_set_sun_hd_light(1, 0, 0.0f);  // a room's light: no night side, no day/night fade
                        if (g_trace && retrace % 300u == 0u)
                            fprintf(stderr, "[hd-look] HD's room light %.3f %.3f %.3f\n", light[0], light[1], light[2]);
                    } else {
                        hd_sun_rule(cpu, light);
                        aurora_set_sun_direction(light[0], light[1], light[2]);
                    }
                } else {
                    aurora_set_sun_direction(sx, sy, sz);
                }
                // The same in the camera's space (its axes are inv's columns),
                // for the room culling below; in a room lit by its own light, that light.
                const float lx = room_lit ? light[0] : sx, ly = room_lit ? light[1] : sy, lz = room_lit ? light[2] : sz;
                const float sl = sqrtf(lx * lx + ly * ly + lz * lz);
                if (sl > 0.0f) {
                    for (int a = 0; a < 3; ++a)
                        g_sun_view[a] = (inv[a] * lx + inv[4 + a] * ly + inv[8 + a] * lz) / sl;
                    g_sun_up = ly / sl;
                }
                if (g_torch == 1) {
                    const float eye[3] = {inv[3], inv[7], inv[11]};
                    point_lights(cpu, eye);
                    torch_view = true;
                }
                if (g_trace && retrace % 300u == 0u)
                    fprintf(stderr, "[hd-look] sun from the eye %.0f %.0f %.0f, HD's cascade splits %.0f %.0f\n", sx,
                            sy, sz, split_middle, split_far);
                show = outdoors;
                sun_show = outdoors || room;
            }
        }
    }
    const bool clouds_on = show && g_clouds == 1, sun_on = sun_show && g_sun == 1, edge_on = show && g_edge == 1;
    const bool torch_on = torch_view && g_torch == 1;
    if (torch_on != g_torch_shown && g_torch >= 0) {
        aurora_set_torch_light(torch_on);
        g_torch_shown = torch_on;
    }
    alpha_model_glows(cpu, !(torch_on && point_lights_hd()));
    if (clouds_on != g_clouds_shown && g_clouds >= 0) {
        aurora_set_cloud_shadows(clouds_on);
        g_clouds_shown = clouds_on;
    }
    cloud_floor(cpu, clouds_on);
    // The room model's widened culling goes with the sun shadows on the high
    // shadow quality (DOL_AURORA_SUN_SHADOW_QUALITY unset or high).
    const char* quality = getenv("DOL_AURORA_SUN_SHADOW_QUALITY");
    const bool high = quality == NULL || quality[0] == '\0' || strcmp(quality, "high") == 0 ||
                      strcmp(quality, "hd") == 0;
    bluewake_hd_room_clip = sun_on && (g_room_cull >= 0 ? g_room_cull == 1 : high);
    // The palms' shadow-sized cull boxes go with it (BLUEWAKE_HD_PALM_CULL
    // overrides; turned on, it applies whatever the sun).
    g_palms = g_palm_cull >= 0 ? g_palm_cull == 1 : sun_on && high;
    if (sun_on != g_sun_shown && g_sun >= 0) {
        aurora_set_sun_shadows(sun_on);
        g_sun_shown = sun_on;
    }
    if (edge_on != g_edge_shown && g_edge >= 0) {
        aurora_set_edge_light(edge_on);
        g_edge_shown = edge_on;
    }
}

// Palm trees' cull boxes, widened by their shadows' reach. A palm stops
// drawing once its box (350 units out from the trunk, 1300 tall) leaves the
// view, so with the sun behind Link its long shadow on the ground ahead
// vanished from the sun shadow map as soon as the trunk went behind the
// camera. The box is widened across by kPalmReach: 5,000 units, twice a
// 1300-unit palm's shadow with the sun 30 degrees up (2,500, then 3,750, fell
// short in play).
// A widened box carries a quarter unit more
// on its top as the mark, so it is changed once and can be changed back, also
// in a save state. fopAc_ac_c::cull.box is at +0x230: min x, y, z, max x, y, z.
enum {
    kLpalm = 0x004Bu,  // fpcNm_Obj_Lpalm_e, d_a_obj_lpalm
    kVyasi = 0x0087u,  // fpcNm_Obj_Vyasi_e, d_a_obj_vyasi (Gale Isle's palms in the wind)
    kCullBox = 0x230u,
};
static const float kPalmReach = 5000.0f;
static const float kPalmMark = 0.25f;

static void palms(CPUState* cpu, bool on) {
    unsigned changed = 0;
    u32 node = mem_read32(cpu, kActorQueue);
    for (u32 n = 0; n < kMaxActors && guest_pointer(node); ++n, node = mem_read32(cpu, node + kNodeNext)) {
        const u32 actor = mem_read32(cpu, node + kTagActor);
        if (!guest_pointer(actor))
            continue;
        const u16 name = mem_read16(cpu, actor + kProcName);
        if (name != kLpalm && name != kVyasi)
            continue;
        const u32 box = actor + kCullBox;
        const float top = read_float(cpu, box + 0x10u);
        const bool widened = fmodf(fabsf(top), 1.0f) == kPalmMark;
        if (name == kLpalm) {
            // The common palm's box is known (d_a_obj_lpalm: 350 across, -50 to
            // 1300), so it is set outright each retrace, as wide as a 1300-unit
            // palm's shadow is long now (half again and 500 spare, as its fronds reach
            // past the box; 2,500 to 20,000): a fixed
            // width had an edge where the shadow, longer at a low sun, popped
            // with the smallest turn of the camera.
            const float up = fmaxf(g_sun_up, 0.02f);
            // From its top down to the sea (y 0), whichever is more: on a cliff
            // its shadow falls on to the water far below it.
            const float top = fmaxf(read_float(cpu, actor + 0x1FCu) + 1300.0f, 1300.0f);
            const float shadow = top * sqrtf(fmaxf(1.0f - up * up, 0.0f)) / up;
            const float half = on ? 350.0f + fminf(fmaxf(shadow * 1.5f + 500.0f, 2500.0f), 20000.0f) : 350.0f;
            if (!on && !widened)
                continue;
            if (on && widened && read_float(cpu, box + 0x0Cu) == half)
                continue;
            write_float(cpu, box + 0x00u, -half);
            write_float(cpu, box + 0x08u, -half);
            write_float(cpu, box + 0x0Cu, half);
            write_float(cpu, box + 0x14u, half);
            write_float(cpu, box + 0x04u, -50.0f);
            write_float(cpu, box + 0x10u, on ? 1300.0f + kPalmMark : 1300.0f);
            changed += widened != on;
            continue;
        }
        if (widened == on)
            continue;
        const float d = on ? kPalmReach : -kPalmReach;
        write_float(cpu, box + 0x00u, read_float(cpu, box + 0x00u) - d);
        write_float(cpu, box + 0x08u, read_float(cpu, box + 0x08u) - d);
        write_float(cpu, box + 0x0Cu, read_float(cpu, box + 0x0Cu) + d);
        write_float(cpu, box + 0x14u, read_float(cpu, box + 0x14u) + d);
        write_float(cpu, box + 0x10u, top + (on ? kPalmMark : -kPalmMark));
        changed++;
    }
    if (changed != 0u && g_trace)
        fprintf(stderr, "[hd-look] palms: %u cull boxes %s\n", changed, on ? "widened" : "back to the game's");
}

// The room model's culling (daBg_c::draw), done here with each joint's box
// swept along its shadow. The room hides every joint whose box is outside the
// camera's view (J3DUClipper::clipByBox, 0x80256CB8, called only from the room
// draw), and a small island in a sea room is one joint: behind the camera its
// shadow left the sun shadow map while a palm on it still cast. Here a joint is
// shown when its box, or the box moved away from the sun by its shadow's
// length (the box's height over tan of the sun's elevation, a fifth spare,
// at most kRoomReach, as the palms), reaches into the view: the region it keeps widens away from the
// camera toward the sun, as the shadows it is for do. The test is the game's
// own (a box is out when all its corners are outside one plane), over the 16
// corners; show and hide are its own (J3DShape::mFlags bit 0). Then the call
// returns to the room draw, with the hidden shapes counted as the game does.
// GZLE01 layouts: J3DUClipper (r3) planes +0x04, near +0x54, far +0x58;
// J3DModel (r4) data +0x04, node matrices +0x8C; J3DModelData joint count
// +0x28, joints +0x2C; J3DJoint kind +0x1A, min +0x40, max +0x4C, mesh +0x60;
// J3DMaterial next +0x04, shape +0x08; J3DShape flags +0x0C; j3dSys's view
// matrix at 0x803EDA58.
enum {
    kClipByBox = 0x80256CB8u,
    kJ3DSysView = 0x803EDA58u,
};
static const float kRoomReach = 20000.0f;  // as the palms' (a small island was lost before its palms)

static void mtx_read(CPUState* cpu, u32 at, float m[12]) {
    for (u32 i = 0; i < 12u; ++i)
        m[i] = read_float(cpu, at + i * 4u);
}

static void mtx_concat(const float a[12], const float b[12], float out[12]) {
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 4; ++c)
            out[r * 4 + c] = a[r * 4] * b[c] + a[r * 4 + 1] * b[4 + c] + a[r * 4 + 2] * b[8 + c] + (c == 3 ? a[r * 4 + 3] : 0.0f);
}

void bluewake_hd_look_room_clip(CPUState* cpu, u32 address) {
    if (address != kClipByBox)
        return;
    const u32 clipper = cpu->gpr[3], model = cpu->gpr[4];
    const u32 data = guest_pointer(model) ? mem_read32(cpu, model + 0x04u) : 0u;
    const u32 nodes = guest_pointer(model) ? mem_read32(cpu, model + 0x8Cu) : 0u;
    if (!guest_pointer(clipper) || !guest_pointer(data) || !guest_pointer(nodes))
        return;  // the game's own clipByBox runs
    float plane[4][3];
    for (u32 i = 0; i < 4u; ++i)
        for (u32 k = 0; k < 3u; ++k)
            plane[i][k] = read_float(cpu, clipper + 0x04u + i * 12u + k * 4u);
    const float clip_near = read_float(cpu, clipper + 0x54u), clip_far = read_float(cpu, clipper + 0x58u);
    float view[12];
    mtx_read(cpu, kJ3DSysView, view);
    // Away from the sun, in the camera's space; the shadow's length per unit of height.
    const float away[3] = {-g_sun_view[0], -g_sun_view[1], -g_sun_view[2]};
    const float up = fmaxf(g_sun_up, 0.02f);
    const float per_height = sqrtf(fmaxf(1.0f - up * up, 0.0f)) / up;
    const u32 count = mem_read16(cpu, data + 0x28u);
    const u32 joints = mem_read32(cpu, data + 0x2Cu);
    u32 hidden = 0u;
    for (u32 i = 0; i < count && guest_pointer(joints); ++i) {
        const u32 joint = mem_read32(cpu, joints + i * 4u);
        if (!guest_pointer(joint) || (mem_read8(cpu, joint + 0x1Au) & 0x0Fu) != 0u)
            continue;
        float anm[12], m[12];
        mtx_read(cpu, nodes + i * 48u, anm);
        mtx_concat(view, anm, m);
        float lo[3], hi[3];
        for (u32 k = 0; k < 3u; ++k) {
            lo[k] = read_float(cpu, joint + 0x40u + k * 4u);
            hi[k] = read_float(cpu, joint + 0x4Cu + k * 4u);
        }
        // The joint's height, or its top's height over the sea (y 0) when that
        // is more: a cliff top's shadow falls all the way to the water.
        float top = -1e30f;
        for (int c = 0; c < 8; ++c)
            top = fmaxf(top, anm[4] * ((c & 1) ? hi[0] : lo[0]) + anm[5] * ((c & 2) ? hi[1] : lo[1]) +
                                 anm[6] * ((c & 4) ? hi[2] : lo[2]) + anm[7]);
        const float height = fmaxf(hi[1] - lo[1], top);
        const float length = fminf(height * per_height * 1.5f + 500.0f, kRoomReach);
        int outside[6] = {0, 0, 0, 0, 0, 0};
        bool in_view = false;
        for (int c = 0; c < 16 && !in_view; ++c) {
            const float q[3] = {(c & 1) ? hi[0] : lo[0], (c & 2) ? hi[1] : lo[1], (c & 4) ? hi[2] : lo[2]};
            float p[3];
            for (int r = 0; r < 3; ++r)
                p[r] = m[r * 4] * q[0] + m[r * 4 + 1] * q[1] + m[r * 4 + 2] * q[2] + m[r * 4 + 3] +
                       (c >= 8 ? away[r] * length : 0.0f);
            bool any = false;
            if (-p[2] < clip_near) outside[4]++, any = true;
            if (-p[2] > clip_far) outside[5]++, any = true;
            for (int k = 0; k < 4; ++k)
                if (p[0] * plane[k][0] + p[1] * plane[k][1] + p[2] * plane[k][2] > 0.0f)
                    outside[k]++, any = true;
            if (!any)
                in_view = true;
        }
        bool out = false;
        if (!in_view)
            for (int k = 0; k < 6; ++k)
                out = out || outside[k] == 16;
        for (u32 mat = mem_read32(cpu, joint + 0x60u); guest_pointer(mat); mat = mem_read32(cpu, mat + 0x04u)) {
            const u32 shape = mem_read32(cpu, mat + 0x08u);
            if (!guest_pointer(shape))
                continue;
            const u32 flags = mem_read32(cpu, shape + 0x0Cu);
            mem_write32(cpu, shape + 0x0Cu, out ? (flags | 1u) : (flags & ~1u));
            if (out)
                hidden++;
        }
    }
    cpu->gpr[3] = hidden;
    cpu->pc = cpu->lr & ~3u;
}

// HD's sky and light palettes (BLUEWAKE_HD_SKY, off by default, needs the
// look): HD retuned the sea stage's colour tables (stage dzs Pale and Virt),
// the sky toward cyan with an aqua horizon, the fog sky blue. The importer
// keeps them, in the GameCube's layout and gamma, in the HD pack's
// optional/hd-sky/sea.bin with the GameCube's own beside them
// (hd-sky/make_sky.py). The game reads the tables where the stage file was
// loaded, every frame (its counts at +0x30 and +0x3C stay 0 in GZLE01, so
// the tables are known by their bytes), so while they hold the GameCube's colours the host
// writes HD's over them, and puts them back when the switch goes off.
// g_dComIfG_gameInfo.play.mStageData (dStage_stageDt_c) is at 0x803C9D58.
enum {
    kPaletInfo = 0x803C9D74u,     // mpPaletInfo (stage_palet_info_class, 0x2C each)
    kVrboxInfo = 0x803C9D80u,     // mpVrboxInfo (stage_vrbox_info_class, 0x24 each)
};
// Every stage HD retuned (optional/hd-sky/stages.bin, else sea.bin alone): a
// stage's GC Pale, HD Pale, GC Virt, HD Virt, HD's Pale factors (version 3:
// each Pale entry's shadow colour, then its factors; 2: the factors), and in
// version 4 HD's five floats for each Virt entry. The stage in
// play is known by its tables holding one stage's GameCube colours; its data
// is then g_sky_data (Virt can be none: an interior has no sky).
typedef struct SkyStage {
    char name[16];
    u8* data;
    u32 pale, virt;
    u32 extra;  // bytes a Pale entry's factors take: 28 with the shadow colour first (version 3), else 24
    const u8* floats;  // HD's Virt floats, 20 bytes an entry (version 4), or NULL
} SkyStage;
static SkyStage* g_skies;
static u32 g_sky_count;
static u8* g_sky_data;  // the stage in play's, or NULL
static u32 g_sky_pale, g_sky_virt;
static u32 g_sky_extra = 24u;  // the stage in play's (SkyStage.extra)
static const u8* g_sky_floats;  // the stage in play's (SkyStage.floats)
static bool g_sky_shown;  // the tables hold HD's (as last seen)
static bool sky_palettes_shown(void) { return g_sky_shown; }
static float g_sky_exposure;  // what aurora_set_exposure was last given

// A new choice of HD sunlight, at once: the brightness is otherwise worked out
// each game frame, and the menu pauses the game.
static void sky_sunlight_changed(void) {
    const char* override = getenv("BLUEWAKE_HD_SKY_GAMMA");
    if (!g_sky_shown || g_sky_factor <= 0.0f || (override != NULL && atof(override) > 0.0))
        return;
    g_sky_exposure = powf(g_sky_factor, g_sunlight);
    aurora_set_exposure(g_sky_exposure);
}

static void sky_load(void) {
    static bool tried;
    if (tried)
        return;
    tried = true;
    const char* pack = getenv("DOL_AURORA_TEXTURE_PACK");
    if (pack == NULL || pack[0] == '\0')
        return;
    char path[4096];
    snprintf(path, sizeof path, "%s/optional/hd-sky/stages.bin", pack);
    FILE* f = fopen(path, "rb");
    u32 count = 1u;
    bool all = false;
    u8 head[16];
    if (f != NULL && fread(head, 1, 8, f) == 8 && memcmp(head, "BWSA", 4) == 0) {
        count = (u32)head[4] << 24 | (u32)head[5] << 16 | (u32)head[6] << 8 | head[7];
        all = true;
    } else {
        if (f != NULL)
            fclose(f);
        snprintf(path, sizeof path, "%s/optional/hd-sky/sea.bin", pack);
        f = fopen(path, "rb");
    }
    if (f == NULL || count == 0u || count > 512u || (g_skies = calloc(count, sizeof *g_skies)) == NULL) {
        if (f != NULL)
            fclose(f);
        fprintf(stderr, "[hd-look] HD sky palettes: none in the pack\n");
        return;
    }
    for (u32 i = 0; i < count; ++i) {
        SkyStage* stage = &g_skies[g_sky_count];
        if (all) {
            u8 name[20];
            if (fread(name, 1, 20, f) != 20)
                break;
            memcpy(stage->name, name, 15);
        } else {
            memcpy(stage->name, "sea", 4);
        }
        if (fread(head, 1, 16, f) != 16 || memcmp(head, "BWSK", 4) != 0 || head[7] < 2u || head[7] > 4u)
            break;
        stage->pale = (u32)head[8] << 24 | (u32)head[9] << 16 | (u32)head[10] << 8 | head[11];
        stage->virt = (u32)head[12] << 24 | (u32)head[13] << 16 | (u32)head[14] << 8 | head[15];
        stage->extra = head[7] >= 3u ? 28u : 24u;
        const size_t floats = 2u * (stage->pale * 0x2Cu + stage->virt * 0x24u) + stage->pale * stage->extra;
        const size_t size = floats + (head[7] >= 4u ? stage->virt * 20u : 0u);
        if (stage->pale == 0u || stage->pale >= 256u || stage->virt >= 256u || (stage->data = malloc(size)) == NULL)
            break;
        stage->floats = head[7] >= 4u ? stage->data + floats : NULL;
        if (fread(stage->data, 1, size, f) != size) {
            free(stage->data);
            break;
        }
        ++g_sky_count;
    }
    fclose(f);
    // A pack imported before version 4 works without HD's haze brightness at dusk; importing it again adds it.
    fprintf(stderr, "[hd-look] HD sky palettes: %u stages from %s%s\n", g_sky_count, path,
            g_sky_count != 0u && g_skies[0].floats == NULL ? " (no haze brightness: import the pack again for it)" : "");
}

static bool guest_equals(CPUState* cpu, u32 address, const u8* bytes, u32 size);

// The bytes of an entry the host rewrites in place while it keeps a table (bit
// n: byte n): a Pale entry's colours (0x00-0x20, the linear blend) and a Virt
// entry's (kVirtColours below: the sky's gain, and the blend). Each retrace it
// rewrites the entries the game is blending: the light's four and, with HD's
// light on characters, the scenery's four (s_written_pale; the characters'
// colours, 0x00-0x05, are among the colours).
#define SKY_PALE_LOOSE 0x1FFFFFFFFull
#define SKY_VIRT_LOOSE 0x1FF770000ull
#define SKY_LOOSE_ENTRIES 8u
// Whether a table that holds HD's already is taken as the stage's
// (BLUEWAKE_STATE_REAPPLY=0 after a load turns it off).
static bool s_sky_adopt = true;

// Whether the game's table at `table` (count entries of `entry` bytes) holds
// `want`: every entry to the byte, but for at most `loose_entries` entries
// that may differ in the bytes `loose` marks. A save state taken while the
// host kept a table holds it so, whatever the table's size.
static bool guest_table_is(CPUState* cpu, u32 table, const u8* want, u32 count, u32 entry, u64 loose,
                           u32 loose_entries) {
    u32 differ = 0u;
    for (u32 i = 0; i < count; ++i) {
        bool same = true;
        for (u32 b = 0; b < entry; ++b) {
            if (mem_read8(cpu, table + i * entry + b) == want[i * entry + b])
                continue;
            if (b >= 64u || (loose & (1ull << b)) == 0u)
                return false;
            same = false;
        }
        if (!same && ++differ > loose_entries)
            return false;
    }
    return true;
}

// The stage whose GameCube Pale table the game's holds, made the one in play;
// false if none. A save state made with HD's palettes in holds HD's instead,
// with the host's changes in a few entries (guest_table_is), and is the stage's
// too. Several stages share the GameCube's tables where HD retuned them apart
// (the fairy fountains, the title's and the epilogue's sea against the sea's,
// two Triforce caves), so the stage of that name is taken first
// (play.mCurStage, 0x803C9D3C), then one that matches to the byte.
static bool sky_pick(CPUState* cpu, u32 table) {
    char current[9] = {0};
    for (u32 i = 0; i < 8u; ++i)
        current[i] = (char)mem_read8(cpu, 0x803C9D3Cu + i);
    int found = -1;
    bool found_exact = false;
    // Frozen Hyrule is sepia in Wind Waker HD, and that look isn't in its
    // palettes: they are tuned to be seen through HD's monotone (monotone()).
    // With HD's alone under the GameCube's grey, the room stayed grey and
    // Link's hair turned green-yellow. Without HD's monotone the GameCube's are
    // kept in the stages the game freezes (d_s_play.cpp: Hyrule, Hyroom,
    // kenroom, and the X stages, Ganon's Tower's refights).
    if (!(g_monotone && aurora_monotone_hd_supported()) &&
        (strncmp(current, "Hyroom", 8) == 0 || strncmp(current, "Hyrule", 8) == 0 ||
         strncmp(current, "kenroom", 8) == 0 || current[0] == 'X'))
        return false;
    for (u32 i = 0; i < g_sky_count; ++i) {
        const SkyStage* stage = &g_skies[i];
        const u8* hd = stage->data + stage->pale * 0x2Cu;
        const bool exact = guest_table_is(cpu, table, stage->data, stage->pale, 0x2Cu, 0u, 0u) ||
                           (s_sky_adopt && guest_table_is(cpu, table, hd, stage->pale, 0x2Cu, 0u, 0u));
        if (!exact && !(s_sky_adopt && guest_table_is(cpu, table, hd, stage->pale, 0x2Cu, SKY_PALE_LOOSE,
                                                      SKY_LOOSE_ENTRIES)))
            continue;
        const bool named = strncmp(stage->name, current, 8) == 0;
        if (found < 0 || named || (exact && !found_exact)) {
            found = (int)i;
            found_exact = exact;
        }
        if (named)
            break;
    }
    if (found < 0)
        return false;
    const SkyStage* stage = &g_skies[found];
    if (g_sky_data != stage->data && g_trace)
        fprintf(stderr, "[hd-look] sky palettes for stage %s\n", stage->name);
    g_sky_data = stage->data;
    g_sky_pale = stage->pale;
    g_sky_virt = stage->virt;
    g_sky_extra = stage->extra;
    g_sky_floats = stage->floats;
    return true;
}

static bool guest_equals(CPUState* cpu, u32 address, const u8* bytes, u32 size) {
    for (u32 i = 0; i < size; ++i)
        if (mem_read8(cpu, address + i) != bytes[i])
            return false;
    return true;
}

// One table: when it holds `from`, write `to` over it.
// One table (Pale or Virt). While it holds the GameCube's colours and HD's
// are wanted, HD's go over it, and the host keeps it (`*owned`): the linear
// blend below then changes its entries, so it is no longer HD's to the byte.
// With HD's no longer wanted the GameCube's go back. A stage that loads again
// over it puts the GameCube's back by itself, and is taken again. Returns the
// table while the host keeps it, else 0.
// A table that holds HD's already, which the host does not keep yet, is from a
// save state made with HD's palettes in: the host keeps it too, with HD's own
// colours back in the entries the state holds blended (or, with HD's no longer
// wanted, the GameCube's go back as above).
static u32 sky_table(CPUState* cpu, u32 pointer, u32 count, u32 entry, const u8* gc, const u8* hd, u64 loose,
                     u32* owned) {
    const u32 table = mem_read32(cpu, pointer);
    if (!guest_pointer(table)) {
        *owned = 0u;
        return 0u;
    }
    const bool is_gc = guest_equals(cpu, table, gc, entry) && guest_equals(cpu, table, gc, count * entry);
    if (s_sky_adopt && !is_gc && *owned != table &&
        guest_table_is(cpu, table, hd, count, entry, loose, SKY_LOOSE_ENTRIES)) {
        if (g_sky)
            for (u32 i = 0; i < count * entry; ++i)
                mem_write8(cpu, table + i, hd[i]);
        *owned = table;
        if (g_trace)
            fprintf(stderr, "[hd-look] sky table at 0x%08X: HD already (a save state)\n", table);
    }
    if (g_sky && is_gc) {
        for (u32 i = 0; i < count * entry; ++i)
            mem_write8(cpu, table + i, hd[i]);
        *owned = table;
        if (g_trace)
            fprintf(stderr, "[hd-look] sky table at 0x%08X: HD\n", table);
    } else if (!g_sky && *owned == table && !is_gc) {
        for (u32 i = 0; i < count * entry; ++i)
            mem_write8(cpu, table + i, gc[i]);
        *owned = 0u;
        if (g_trace)
            fprintf(stderr, "[hd-look] sky table at 0x%08X: GameCube\n", table);
    } else if (*owned != table) {
        *owned = 0u;
    }
    return *owned;
}

// HD's brightness now: its factor for the sun's light on scenery (BG0 K0) in
// each palette the game is blending, blended as the game blends their colours
// (dScnKy_env_light_c::setLight_palno_get and setLight): the hour's two
// palettes of the last weather and of the new one, by the hour, then by the
// weather's change. In g_env_light (0x803E4AB4). As the scene's brightness,
// taken to gamma's 1/2.2 (0.4545): 3 at noon is 1.65, 2 in the morning 1.37,
// HD's sunlight in the game's gamma. It once looked too strong on the islands'
// sand and rock and was taken to 0.37, while the cloud shadows darkened the
// whole picture and the rock's faces turned from the sun weren't shaded; with
// those fixed, 1/2.2 matched HD's bridge at Outset at 10:00.
enum {
    kEnvLight = 0x803E4AB4u,
    kEnvPselect = kEnvLight + 0x008u,   // mpPselectInfo (0xC each: palette ids[8], change rate)
    kEnvEnvr = kEnvLight + 0x00Cu,      // mpEnvrInfo (8 each: pselect ids)
    kEnvSchedule = kEnvLight + 0x014u,  // mpSchejule (11 of 0xC: end time, begin time, palette slots 0 and 1)
    kEnvColPatBlend = kEnvLight + 0xBE0u,
    kEnvFogStart = kEnvLight + 0xBE8u,  // mFogStartZ, then mFogEndZ (after the palette blend, clamped at 0)
    kEnvFogEnd = kEnvLight + 0xBECu,
    kEnvTime = kEnvLight + 0xC38u,
    kEnvColpatPrev = kEnvLight + 0xC84u,
    kEnvColpatCurr = kEnvLight + 0xC85u,
    kEnvEnvrPrev = kEnvLight + 0xC88u,
    kEnvEnvrCurr = kEnvLight + 0xC89u,
    // The event tints (sky_tint_read).
    kEnvActorAddAmb = kEnvLight + 0xB70u,  // mActorAddColAmb, then mActorAddColDif, mBgAddColAmb, mBgAddColDif (s16 x3 in 8)
    kEnvAllRatio = kEnvLight + 0xBFCu,     // mAllColRatio, then mActColRatio, mBgColRatio
    kEnvColChgStrength = kEnvLight + 0xC34u,
    kEnvColChgFlag = kEnvLight + 0xC93u,
    kEnvColChgItem = kEnvLight + 0xC98u,
    kEnvThunder = kEnvLight + 0xA30u,  // mThunderEff: mState (u8) at +1, mFlashTimer (f32) at +8
};

// HD's day ends sooner: its light schedule (dKyd_schejule_getp's table, found in
// HD's cking.rpx with the GameCube's values but these) holds the noon palette
// to 17:00, not 18:00, goes to the evening's by 18:00 and takes two hours, not
// one, to dusk's at 20:00. Dawn, morning and night are the GameCube's. Each
// entry: end, begin (degrees, 15 an hour), palette slots 0 and 1. Written over
// the game's table (mpSchejule, 11 of 0xC) while it holds the GameCube's, with
// the palettes, and put back with them. Wind Waker HD's sunset at 17:30 on
// Windfall's dock was ours at 18:30 before.
static const float kScheduleGc[3][2] = {{150.0f, 270.0f}, {270.0f, 285.0f}, {285.0f, 300.0f}};
static const float kScheduleHd[3][2] = {{150.0f, 255.0f}, {255.0f, 270.0f}, {270.0f, 300.0f}};

static bool sky_schedule(CPUState* cpu, bool hd) {
    const u32 table = mem_read32(cpu, kEnvSchedule);
    if (!guest_pointer(table))
        return false;
    const float (*from)[2] = hd ? kScheduleGc : kScheduleHd;
    const float (*to)[2] = hd ? kScheduleHd : kScheduleGc;
    bool has_to = true, has_from = true;
    for (u32 i = 0; i < 3u; ++i)
        for (u32 k = 0; k < 2u; ++k) {
            const float v = read_float(cpu, table + (4u + i) * 0xCu + k * 4u);
            has_to = has_to && v == to[i][k];
            has_from = has_from && v == from[i][k];
        }
    // The rest of the field schedule, so the boss's or another table is never touched.
    if ((!has_to && !has_from) || read_float(cpu, table + 4u) != 90.0f || read_float(cpu, table + 3u * 0xCu + 4u) != 150.0f)
        return false;
    if (has_from)
        for (u32 i = 0; i < 3u; ++i)
            for (u32 k = 0; k < 2u; ++k)
                write_float(cpu, table + (4u + i) * 0xCu + k * 4u, to[i][k]);
    return true;
}

// HD's sky models are the GameCube's four, taller: the horizon's haze, the
// band that carries the dawn's and dusk's lavender and noon's aqua, fades out
// 17.7 degrees up in HD's vr_sky, 6.4 in ours; the front haze (vr_kasumi_mae)
// at 7.9, not 4.9; the three cloud layers (vr_back_cloud) reach 11.3, 21.5
// and 7.75 degrees, not 9.5, 10.3 and 4.4 (measured in HD's sea_Stage and the
// GameCube's sea Stage.arc). Their joints' heights (J3DJoint's transform
// scale y, +0x20) are set to match while HD's palettes are in, and put back
// with them; only a joint holding the GameCube's or that height is touched.
// The sky actors make the models again from their joints every frame
// (mDoExt_modelUpdateDL). daVrbox (0x1BA in GZLE01, one below the decomp's
// fpcNm list): vr_sky at +0x294; daVrbox2 (0x1BB): vr_back_cloud at +0x294,
// vr_kasumi_mae at +0x29C.
static const struct {
    u16 proc;
    u16 model;  // the actor's model pointer
    u16 joints, joint;  // the model's joint count, and the joint
    float gc, hd;  // its height (scale y)
} kSkyJoints[] = {
    {0x1BAu, 0x294u, 4u, 2u, 1.0f, 2.80f},      // vr_sky: the haze band
    {0x1BBu, 0x29Cu, 3u, 2u, 0.951808f, 1.535f}, // vr_kasumi_mae
    {0x1BBu, 0x294u, 5u, 2u, 1.0f, 1.20f},      // vr_back_cloud: the near mae layer
    {0x1BBu, 0x294u, 5u, 3u, 1.0f, 2.16f},      // the naka layer, highest in HD
    {0x1BBu, 0x294u, 5u, 4u, 0.45f, 0.80f},     // the low mae layer
};

// HD draws each cloud layer as one sheet of its texture, scrolling with the
// wind. The GameCube's layer material (vr_back_cloud's three, 4 TEV stages)
// samples its colour and alpha textures twice, once still and once scrolling
// 0.2 along, and screens the colours and multiplies the alphas: clouds that
// slowly change shape, and with HD's art only the overlaps left, scattered
// blobs instead of HD's cirrus bands. With HD's palettes in, stage 1 takes
// the scrolling sample's colour alone and stage 3 its alpha alone (J3DTevStage
// bytes in the material's TEV block: model data +0x60 the materials, a
// material +0x2C its TEV block, then the stages, 8 bytes each).
// The material's own display list (J3DMaterial +0x48, J3DDisplayListObj: two
// buffers, then the size) is what is drawn; it carries the same stages as GX
// BP writes (0x61, the register, three bytes), made once at load. Patched in
// both buffers: stage 1's colour (register 0xC2) and stage 3's alpha (0xC7).
static void sky_layer_dl(CPUState* cpu, u32 material, bool hd) {
    const u32 dl = mem_read32(cpu, material + 0x48u);
    if (!guest_pointer(dl))
        return;
    const u32 size = mem_read32(cpu, dl + 0x08u);
    for (u32 b = 0; b < 2u && size > 0u && size < 0x1000u; ++b) {
        const u32 data = mem_read32(cpu, dl + b * 4u);
        if (!guest_pointer(data))
            continue;
        for (u32 i = 0; i + 5u <= size; ++i) {
            if (mem_read8(cpu, data + i) != 0x61u)
                continue;
            const u8 reg = mem_read8(cpu, data + i + 1u), x = mem_read8(cpu, data + i + 3u), y = mem_read8(cpu, data + i + 4u);
            if (reg == 0xC2u) {
                if (hd && x == 0x0Fu && y == 0x88u) {
                    mem_write8(cpu, data + i + 3u, 0xFFu);
                    mem_write8(cpu, data + i + 4u, 0xF8u);
                } else if (!hd && x == 0xFFu && y == 0xF8u) {
                    mem_write8(cpu, data + i + 3u, 0x0Fu);
                    mem_write8(cpu, data + i + 4u, 0x88u);
                }
            } else if (reg == 0xC7u) {
                if (hd && x == 0xE2u && (y & 0xF0u) == 0x70u) {
                    mem_write8(cpu, data + i + 3u, 0xFFu);
                    mem_write8(cpu, data + i + 4u, (u8)(0xC0u | (y & 0x0Fu)));
                } else if (!hd && x == 0xFFu && (y & 0xF0u) == 0xC0u) {
                    mem_write8(cpu, data + i + 3u, 0xE2u);
                    mem_write8(cpu, data + i + 4u, (u8)(0x70u | (y & 0x0Fu)));
                }
            }
        }
    }
}

// The scrolling sample also starts 0.2 along (texture matrix 1's translation
// x, +0x1C), for the GameCube's two copies to differ; one copy that far along
// shows another part of the texture than HD's model, whose layers start at
// 0 (72 degrees around the sky for the naka layer). With HD's palettes in it
// starts at 0; the wind's scroll goes on adding to it. Which matrices are
// shifted is kept, to put the 0.2 back.
static u32 s_shifted[8];  // cleared while no sky actor is there (a stage change makes the models again)

// A layer whose material holds HD's stages already (`was_hd`) but whose
// matrix isn't kept was shifted by a run before (a save state made with HD's
// sky models): it is kept as it is, not shifted again.
static void sky_layer_shift(CPUState* cpu, u32 mtx, bool hd, bool was_hd) {
    int at = -1, free_slot = -1;
    for (int i = 0; i < 8; ++i) {
        if (s_shifted[i] == mtx)
            at = i;
        else if (s_shifted[i] == 0u && free_slot < 0)
            free_slot = i;
    }
    if (hd && at < 0 && free_slot >= 0) {
        if (!was_hd)
            write_float(cpu, mtx + 0x1Cu, read_float(cpu, mtx + 0x1Cu) - 0.2f);
        s_shifted[free_slot] = mtx;
    } else if (!hd && at >= 0) {
        write_float(cpu, mtx + 0x1Cu, read_float(cpu, mtx + 0x1Cu) + 0.2f);
        s_shifted[at] = 0u;
    }
}

// HD wraps its highest layer's texture (naka, the cirrus band) once around the
// sky, the GameCube's four times: HD's streaks are four times as long, long
// cirrus and cirrostratus, where four repeats left short patches. The mae
// layers wrap alike (6 and 8). So the naka material's scrolling texture
// matrix (texture matrix 1; J3DTexMtx: centre at +0x04, scale x at +0x10,
// rebuilt each frame from these) is scaled to a quarter across. The material's
// texture matrices are found among its texgen block's pointers by their
// centre (0.5, 0.5).
static void sky_layer_wrap(CPUState* cpu, u32 material, u32 m, bool hd, bool was_hd) {
    const u32 texgen = mem_read32(cpu, material + 0x28u);
    if (!guest_pointer(texgen))
        return;
    u32 found = 0u;
    for (u32 o = 0x04u; o < 0x80u; o += 4u) {
        const u32 mtx = mem_read32(cpu, texgen + o);
        if (!guest_pointer(mtx) || read_float(cpu, mtx + 0x04u) != 0.5f || read_float(cpu, mtx + 0x08u) != 0.5f)
            continue;
        if (found++ != 1u)
            continue;
        sky_layer_shift(cpu, mtx, hd, was_hd);
        if (m != 1u)
            return;
        const float now = read_float(cpu, mtx + 0x10u), want = hd ? 0.25f : 1.0f;
        if (now != want && (now == 1.0f || now == 0.25f))
            write_float(cpu, mtx + 0x10u, want);
        return;
    }
}

static void sky_layer_material(CPUState* cpu, u32 data, bool hd) {
    if (mem_read16(cpu, data + 0x5Cu) != 3u)
        return;
    const u32 materials = mem_read32(cpu, data + 0x60u);
    for (u32 m = 0; guest_pointer(materials) && m < 3u; ++m) {
        const u32 material = mem_read32(cpu, materials + m * 4u);
        const u32 tev = guest_pointer(material) ? mem_read32(cpu, material + 0x2Cu) : 0u;
        if (!guest_pointer(tev))
            continue;
        // The stages, found by their register bytes (0xC0 stage 0's colour,
        // 0xC1 its alpha, and on): the block's class sets where they start
        // (+0x38 for this model's, with 8 TEV orders before them).
        u32 s0 = 0u;
        for (u32 o = 0x10u; o < 0x80u && s0 == 0u; o += 2u)
            if (mem_read8(cpu, tev + o) == 0xC0u && mem_read8(cpu, tev + o + 4u) == 0xC1u &&
                mem_read8(cpu, tev + o + 8u) == 0xC2u && mem_read8(cpu, tev + o + 28u) == 0xC7u)
                s0 = tev + o;
        if (s0 == 0u)
            continue;
        const u32 s1 = s0 + 8u, s3 = s0 + 24u;
        // Stage 1 colour: a, b | c, d. GameCube: CPREV, ZERO | TEXC, TEXC (screen); HD: ZERO, ZERO | ZERO, TEXC.
        const u8 ab = mem_read8(cpu, s1 + 2u), cd = mem_read8(cpu, s1 + 3u);
        // Stage 3 alpha: a<<5 | b<<2 | c>>1, then (c&1)<<7 | d<<4 | swap. GameCube: ZERO, APREV, TEXA, ZERO
        // (a product); HD: ZERO, ZERO, ZERO, TEXA.
        const u8 aab = mem_read8(cpu, s3 + 6u), sw = mem_read8(cpu, s3 + 7u);
        const bool gc = ab == 0x0Fu && cd == 0x88u && aab == 0xE2u && (sw & 0xF0u) == 0x70u;
        const bool is_hd = ab == 0xFFu && cd == 0xF8u && aab == 0xFFu && (sw & 0xF0u) == 0xC0u;
        sky_layer_dl(cpu, material, hd);
        sky_layer_wrap(cpu, material, m, hd, is_hd);
        if (g_trace && !gc && !is_hd) {
            static unsigned s_told;
            if (s_told++ < 3u)
                fprintf(stderr, "[hd-look] cloud layer material %u: stages unlike the GameCube's (%02X %02X %02X %02X)\n",
                        m, ab, cd, aab, sw);
        }
        if (hd && gc) {
            mem_write8(cpu, s1 + 2u, 0xFFu);
            mem_write8(cpu, s1 + 3u, 0xF8u);
            mem_write8(cpu, s3 + 6u, 0xFFu);
            mem_write8(cpu, s3 + 7u, (u8)(0xC0u | (sw & 0x0Fu)));
        } else if (!hd && is_hd) {
            mem_write8(cpu, s1 + 2u, 0x0Fu);
            mem_write8(cpu, s1 + 3u, 0x88u);
            mem_write8(cpu, s3 + 6u, 0xE2u);
            mem_write8(cpu, s3 + 7u, (u8)(0x70u | (sw & 0x0Fu)));
        }
    }
}

// HD's floating clouds (the sky's puffs, drawVrkumo) are larger than the
// GameCube's. Their size is 0.45 + 0.55 times the weather's cloud strength,
// two constants the game reads from its small data (0x803F8E10 and 0x803F8E14,
// read by drawVrkumo alone); with HD's sky models they are scaled together by
// BLUEWAKE_HD_SKY_PUFFS (1.4 by default), and put back with them.
static void sky_puffs(CPUState* cpu, bool hd) {
    static float s_scale = -1.0f;
    if (s_scale < 0.0f) {
        const char* value = getenv("BLUEWAKE_HD_SKY_PUFFS");
        s_scale = value != NULL && atof(value) > 0.0 ? (float)atof(value) : 1.4f;
    }
    const float base = read_float(cpu, 0x803F8E10u), grow = read_float(cpu, 0x803F8E14u);
    const float want_base = hd ? 0.45f * s_scale : 0.45f, want_grow = hd ? 0.55f * s_scale : 0.55f;
    const bool known = (base == 0.45f && grow == 0.55f) || (base == 0.45f * s_scale && grow == 0.55f * s_scale);
    if (known && (base != want_base || grow != want_grow)) {
        write_float(cpu, 0x803F8E10u, want_base);
        write_float(cpu, 0x803F8E14u, want_grow);
    }
}

static void sky_models(CPUState* cpu, bool hd) {
    const bool models_on = g_sky_models;
    sky_puffs(cpu, hd && models_on);
    bool clouds_seen = false;
    u32 node = mem_read32(cpu, kActorQueue);
    for (u32 n = 0; n < kMaxActors && guest_pointer(node); ++n, node = mem_read32(cpu, node + kNodeNext)) {
        const u32 actor = mem_read32(cpu, node + kTagActor);
        if (!guest_pointer(actor))
            continue;
        const u16 proc = mem_read16(cpu, actor + kProcName);
        if (proc != 0x1BAu && proc != 0x1BBu)
            continue;
        if (proc == 0x1BBu) {
            clouds_seen = true;
            const u32 clouds = mem_read32(cpu, actor + 0x294u);
            const u32 data = guest_pointer(clouds) ? mem_read32(cpu, clouds + 0x04u) : 0u;
            if (guest_pointer(data) && mem_read16(cpu, data + 0x28u) == 5u)
                sky_layer_material(cpu, data, hd && models_on);
        }
        for (u32 k = 0; k < sizeof kSkyJoints / sizeof kSkyJoints[0]; ++k) {
            if (kSkyJoints[k].proc != proc)
                continue;
            const u32 model = mem_read32(cpu, actor + kSkyJoints[k].model);
            const u32 data = guest_pointer(model) ? mem_read32(cpu, model + 0x04u) : 0u;
            if (!guest_pointer(data) || mem_read16(cpu, data + 0x28u) != kSkyJoints[k].joints)
                continue;
            const u32 joints = mem_read32(cpu, data + 0x2Cu);
            const u32 joint = guest_pointer(joints) ? mem_read32(cpu, joints + kSkyJoints[k].joint * 4u) : 0u;
            if (!guest_pointer(joint))
                continue;
            const float now = read_float(cpu, joint + 0x20u);
            const float want = hd && models_on ? kSkyJoints[k].hd : kSkyJoints[k].gc;
            if (now != want && (now == kSkyJoints[k].gc || now == kSkyJoints[k].hd))
                write_float(cpu, joint + 0x20u, want);
        }
    }
    if (!clouds_seen)
        memset(s_shifted, 0, sizeof s_shifted);
}

// The palette (Pale index) the game takes for a weather's slot, or -1.
static int sky_palette(CPUState* cpu, u32 envr_idx, u32 colpat, u32 slot) {
    const u32 envr = mem_read32(cpu, kEnvEnvr), pselect = mem_read32(cpu, kEnvPselect);
    if (!guest_pointer(envr) || !guest_pointer(pselect) || colpat > 7u || slot > 5u)
        return -1;
    const u32 psel = mem_read8(cpu, envr + envr_idx * 8u + colpat);
    const u32 pale = mem_read8(cpu, pselect + psel * 0xCu + slot);
    return pale < g_sky_pale ? (int)pale : -1;
}

// The room Link is in (dStage_roomControl_c::mStayNo), negative while none is.
enum { kStayRoom = 0x803F6A78u };

// The four palettes the game is blending now and how (setLight_palno_get):
// the hour's two (slots) of the last weather, the same of the new one, by the
// hour (ab), then by the weather's change (cb). False outside the schedule.
// The game's light (g_env_light) takes its palettes from the floor under Link
// (his polygon's colour, settingTevStruct_colget_player) and lights Link, the
// sky and the fog with them; the scenery takes its room's (setLight_bg: a
// room's palettes are those of its number). With `scenery`, the room's: under
// Windfall's arch the floor names a darker palette (envr 50) that dims Link
// while the walls and the street keep the town's (room 11), in Wind Waker HD
// as on the GameCube.
static bool sky_blend_state(CPUState* cpu, bool scenery, int pale[4], float* ab, float* cb) {
    const u32 schedule = mem_read32(cpu, kEnvSchedule);
    if (!guest_pointer(schedule))
        return false;
    const float now = read_float(cpu, kEnvTime);
    for (u32 i = 0; i < 11u; ++i) {
        const u32 at = schedule + i * 0xCu;
        const float end = read_float(cpu, at), begin = read_float(cpu, at + 4u);
        if (!(now >= end && now <= begin))
            continue;
        const u32 slot0 = mem_read8(cpu, at + 8u), slot1 = mem_read8(cpu, at + 9u);
        *ab = 1.0f;  // get_parcent(begin, end, now)
        if (begin - end != 0.0f) {
            *ab = 1.0f - (begin - now) / (begin - end);
            if (*ab >= 1.0f)
                *ab = 1.0f;
        }
        u32 envr0 = mem_read8(cpu, kEnvEnvrPrev), envr1 = mem_read8(cpu, kEnvEnvrCurr);
        u32 pat0 = mem_read8(cpu, kEnvColpatPrev);
        const u32 pat1 = mem_read8(cpu, kEnvColpatCurr);
        const float blend = read_float(cpu, kEnvColPatBlend);
        *cb = blend < 0.0f ? 0.0f : blend > 1.0f ? 1.0f : blend;
        const s8 room = (s8)mem_read8(cpu, kStayRoom);
        if (scenery && room >= 0) {
            // The room's palettes, faded in from the last room's through a room change (room_fade).
            int from;
            *cb = room_fade(&from);
            envr0 = from >= 0 ? (u32)from : (u32)room;
            envr1 = (u32)room;
            pat0 = pat1;
        }
        pale[0] = sky_palette(cpu, envr0, pat0, slot0);
        pale[1] = sky_palette(cpu, envr0, pat0, slot1);
        pale[2] = sky_palette(cpu, envr1, pat1, slot0);
        pale[3] = sky_palette(cpu, envr1, pat1, slot1);
        return true;
    }
    return false;
}

// A Pale entry's extra bytes (its shadow colour in version 3, then its factors).
static const u8* sky_extra_of(int pale) {
    return g_sky_data + 2u * (g_sky_pale * 0x2Cu + g_sky_virt * 0x24u) + (u32)pale * g_sky_extra;
}

// HD's factor `index` for a Pale entry (0 actor C0, 1 actor K0, 2 BG0 C0, 3 BG0 K0, 4 BG1 C0, 5 BG1 K0).
static float sky_factor_at(int pale, u32 index) {
    if (pale < 0)
        return 1.0f;
    const u8* f = sky_extra_of(pale) + (g_sky_extra == 28u ? 4u : 0u) + index * 4u;
    const u32 bits = (u32)f[0] << 24 | (u32)f[1] << 16 | (u32)f[2] << 8 | f[3];
    float value;
    memcpy(&value, &bits, 4);
    return value > 0.0f && value < 10.0f ? value : 1.0f;
}

// The power HD's factors are taken to: the menu's HD sunlight (0.4545, HD's), or
// BLUEWAKE_HD_SKY_GAMMA over it for testing.
static float sky_power(void) {
    static float s_override = -1.0f;
    if (s_override < 0.0f) {
        const char* value = getenv("BLUEWAKE_HD_SKY_GAMMA");
        s_override = value != NULL && atof(value) > 0.0 ? (float)atof(value) : 0.0f;
    }
    return s_override > 0.0f ? s_override : g_sunlight;
}

// The game's event tints this retrace (g_env_light's add colours and ratios, the copies exeKankyo made, read
// after the game's frame), for the host's own HD colours: HD tints those as it tints its TEV colours,
// clamp((blend + add) x mAllColRatio x ratio) on its linear bytes (kankyo_color_ratio_set; the characters' shade
// x mActColRatio squared, the shadow colour with the scenery's ambient add and ratio) and its intensities x
// mAllColRatio (its float_kankyo_color_ratio_set). Lightning, bombs, the chest's dim, staring at the sun and
// the demos write them alike in both games. An item get and the arrows' colours are HD's own (its
// d_kankyo_4.cpp): an item get dims the picture (the characters by 1 - s/2 and their shade toward black, the
// rest by 1 - 3s/4) where the GameCube lights Link up, and an arrow takes each lit colour and shade toward
// HD's fire, ice or light by s. The game's own TEV colours keep the GameCube's, from its gamma bytes.
typedef struct SkyTint {
    float act_amb[3], act_dif[3], bg_amb[3], bg_dif[3];  // adds, in bytes
    float all, act, bg;                                  // ratios
    float s;                                             // an item get's or an arrow's strength, else 0
    bool item, arrow;
    bool thunder;                      // a lightning flash is lighting the picture (mThunderEff)
    float arrow_amb[3], arrow_dif[3];  // an arrow's colours (linear bytes)
} SkyTint;
static SkyTint g_tint = {.all = 1.0f, .act = 1.0f, .bg = 1.0f};
static float g_sky_fade;  // HD's day/night fade this retrace while its sky is shown (hd_daynight_fade)

static float tint_ratio(CPUState* cpu, u32 at) {
    const float v = read_float(cpu, at);
    return v >= 0.0f && v <= 4.0f ? v : 1.0f;
}

static void sky_tint_read(CPUState* cpu) {
    SkyTint t = {.all = 1.0f, .act = 1.0f, .bg = 1.0f};
    float* adds[4] = {t.act_amb, t.act_dif, t.bg_amb, t.bg_dif};
    for (u32 k = 0; k < 4u; ++k)
        for (u32 c = 0; c < 3u; ++c) {
            const int v = (s16)mem_read16(cpu, kEnvActorAddAmb + k * 8u + c * 2u);
            adds[k][c] = v >= -1024 && v <= 1024 ? (float)v : 0.0f;
        }
    t.all = tint_ratio(cpu, kEnvAllRatio);
    t.act = tint_ratio(cpu, kEnvAllRatio + 4u);
    t.bg = tint_ratio(cpu, kEnvAllRatio + 8u);
    {
        const u8 state = mem_read8(cpu, kEnvThunder + 1u);
        const float timer = read_float(cpu, kEnvThunder + 8u);
        t.thunder = state != 0u && timer > 0.0f && timer <= 1.0f;
    }
    const u8 flag = mem_read8(cpu, kEnvColChgFlag);
    float strength = read_float(cpu, kEnvColChgStrength);
    strength = !(strength > 0.0f) ? 0.0f : strength > 1.0f ? 1.0f : strength;
    if ((flag & 7u) != 0u && mem_read8(cpu, kEnvColChgItem) != 0u) {
        t.item = true;
        t.s = strength;
        t.act = 1.0f - 0.5f * strength;
        t.bg = 1.0f - 0.75f * strength;
        for (u32 c = 0; c < 3u; ++c)
            t.act_amb[c] = t.act_dif[c] = 0.0f;  // the shade's add (toward black) is worked out from it
    } else if (flag >= 0x10u && flag <= 0x60u && (flag & 0x0Fu) == 0u) {
        static const float kArrow[3][3] = {{255.0f, 80.0f, 20.0f}, {30.0f, 160.0f, 255.0f}, {225.0f, 255.0f, 60.0f}};
        const u32 kind = ((flag >> 4) - 1u) % 3u;  // fire, ice, light (0x10-0x30 in, 0x40-0x60 out)
        char stage[8];
        for (u32 i = 0; i < 8u; ++i)
            stage[i] = (char)mem_read8(cpu, 0x803C9D3Cu + i);  // play.mCurStage.mName
        const bool tower = kind == 2u && memcmp(stage, "GTower", 7) == 0;
        static const float kTowerAmb[3] = {69.0f, 27.0f, 0.0f}, kTowerDif[3] = {116.0f, 101.0f, 16.0f};
        t.arrow = true;
        t.s = strength;
        for (u32 c = 0; c < 3u; ++c) {
            t.arrow_amb[c] = tower ? kTowerAmb[c] : kArrow[kind][c];
            t.arrow_dif[c] = tower ? kTowerDif[c] : kArrow[kind][c];
        }
    }
    g_tint = t;
}

// HD's factor for the hour, the scenery's lit intensity (BG0 K0's), taken toward its shade's (BG0 C0's) by the
// day/night fade as HD's is. The event tints' mAllColRatio is not in it: the game's own TEV colours, which the
// brightness scales, take it already, on their gamma bytes (x 0.4 there is about HD's 0.4 x 0.4 in linear light).
static float sky_brightness(CPUState* cpu) {
    int pale[4];
    float ab, cb;
    if (!sky_blend_state(cpu, true, pale, &ab, &cb))
        return 0.0f;
    float f[2];
    for (u32 k = 0; k < 2u; ++k) {
        const float a = sky_factor_at(pale[0], 3u - k), b = sky_factor_at(pale[1], 3u - k);
        const float c = sky_factor_at(pale[2], 3u - k), d = sky_factor_at(pale[3], 3u - k);
        const float prev = a + ab * (b - a), curr = c + ab * (d - c);
        f[k] = prev + cb * (curr - prev);
    }
    const float factor = f[0] + g_sky_fade * (f[1] - f[0]);
    g_sky_factor = factor;
    return powf(factor, sky_power());
}

// HD scales each light colour by its own factor: a character's light (actor K0)
// by 2.75 at noon and its shade (actor C0) by 1, against the scenery's 3 (read
// on Link in Cemu at Windfall's dock at 14:00: K0 x 2.75, C0 x 1, linear). The
// brightness above takes the whole picture up by the scenery's, so a character's
// shade came out three times HD's (Link's back lit, his shirt washed out, his
// hair glowing). The characters' two colours in the Pale entries written here are
// taken down by their factor against the scenery's, to the same power, so after
// the brightness they are HD's. BLUEWAKE_HD_ACTOR_LIGHT=0 leaves them alone.
static bool sky_actor_light(void) {
    static int s_on = -1;
    if (s_on < 0) {
        const char* value = getenv("BLUEWAKE_HD_ACTOR_LIGHT");
        s_on = value == NULL || value[0] != '0';
    }
    return s_on && g_sky_factor > 0.0f;
}

// A Pale entry's character colours (out[0..5]: actor C0, actor K0) scaled by the
// characters' factors (`c0`, `k0`, blended as the entry is) against the scenery's.
static void sky_actor_scale(u8 out[6], float c0, float k0) {
    if (!sky_actor_light())
        return;
    const float p = sky_power();
    const float s[2] = {powf(c0 / g_sky_factor, p), powf(k0 / g_sky_factor, p)};
    for (u32 c = 0; c < 6u; ++c) {
        const float v = out[c] * s[c / 3u] + 0.5f;
        out[c] = (u8)(v > 255.0f ? 255.0f : v);
    }
}

// HD blends its palettes in linear light (it keeps them linear), the game in
// its gamma: halfway from noon's cyan sky to evening's orange is a grey teal
// in the game's blend and HD's lavender in linear light (Windfall's dock at
// 17:30), and dawn and dusk pass through purples and pinks in HD. The game
// blends the four palettes sky_blend_state names with fixed weights, so the
// host writes the linear blend of their colours into all four (their Pale
// entries and the Virt entries those name): whatever the weights, the game's
// blend is then HD's. Each retrace the entries written last time get HD's
// own colours back first. Only with BLUEWAKE_HD_SKY_LINEAR=1 (the default with
// a pack imported from the HD disc, bluewake_hd_look_defaults).
static float sky_lin(u8 v) { return powf(v / 255.0f, 2.2f); }
static u8 sky_gam(float v) {
    const float g = powf(v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v, 1.0f / 2.2f) * 255.0f + 0.5f;
    return (u8)(g > 255.0f ? 255.0f : g);
}

// The four palettes' blend of a colour (3 bytes at `offset` in HD's Pale entries), in linear light, as bytes.
static void sky_blend_bytes(const int pale[4], const float w[4], u32 offset, float out[3]) {
    const u8* hd_pale = g_sky_data + g_sky_pale * 0x2Cu;
    for (u32 c = 0; c < 3u; ++c) {
        out[c] = 0.0f;
        for (int i = 0; i < 4; ++i)
            out[c] += w[i] * sky_lin(hd_pale[(u32)pale[i] * 0x2Cu + offset + c]) * 255.0f;
    }
}

static float sky_blend_factor(const int pale[4], const float w[4], u32 index) {
    float v = 0.0f;
    for (int i = 0; i < 4; ++i)
        v += w[i] * sky_factor_at(pale[i], index);
    return v;
}

// A blended colour (linear bytes) with its add and ratio, HD's way (kankyo_color_ratio_set), 0-1.
static float sky_tinted(float blend, float add, float ratio) {
    const float v = (blend + add) * g_tint.all * ratio;
    return (v < 0.0f ? 0.0f : v > 255.0f ? 255.0f : v) / 255.0f;
}

// One light's shade and lit colour (C0, K0: linear bytes) tinted, then faded (K0 toward C0) by the day/night
// fade, with their intensities (x mAllColRatio, the lit one faded too), as 0-1 times the intensity. `amb` and
// `dif` are the adds, `c0_ratio` and `k0_ratio` the ratios. Out: the shade's colour before its intensity.
static void sky_light(const float c0[3], const float k0[3], const float amb[3], const float dif[3], float c0_ratio,
                      float k0_ratio, float fc, float fk, float shade[3], float lit[3], float c0_bytes[3]) {
    fk = (fk + g_sky_fade * (fc - fk)) * g_tint.all;
    fc *= g_tint.all;
    for (u32 c = 0; c < 3u; ++c) {
        float add_c0 = amb[c], add_k0 = dif[c];
        if (g_tint.arrow) {  // toward the arrow's colour from the light as it is (HD's mActorC0 and the rest)
            add_c0 = (g_tint.arrow_amb[c] - sky_tinted(c0[c], 0.0f, c0_ratio) * 255.0f) * g_tint.s;
            add_k0 = (g_tint.arrow_dif[c] - sky_tinted(k0[c], 0.0f, k0_ratio) * 255.0f) * g_tint.s;
        }
        const float s0 = sky_tinted(c0[c], add_c0, c0_ratio);
        float k = sky_tinted(k0[c], add_k0, k0_ratio);
        k += g_sky_fade * (s0 - k);
        c0_bytes[c] = s0;
        shade[c] = s0 * fc;
        lit[c] = k * fk;
    }
}

// HD's shadow colour (mBgAmbientColorHD, its Pale entry's twelfth colour): one for the whole picture, blended
// from the palettes the floor under Link picks (its global setLight), with the scenery's ambient add and ratio.
// Not a lightning flash's add: in Cemu the shadow colour stays as it is through a flash (27, 98, 181 at the peak
// and plain alike) while the lit colours go white, so the flash's blue add would turn the whole shade blue.
static void sky_shadow_colour(const int pale[4], const float w[4], float out[3]) {
    float sh[3] = {0.0f, 0.0f, 0.0f}, bg0[3];
    for (int i = 0; i < 4; ++i) {
        const u8* extra = sky_extra_of(pale[i]);
        for (u32 c = 0; c < 3u; ++c)
            sh[c] += w[i] * extra[c];
    }
    sky_blend_bytes(pale, w, 0x06u, bg0);
    for (u32 c = 0; c < 3u; ++c) {
        const float add = g_tint.arrow ? (g_tint.arrow_amb[c] - sky_tinted(bg0[c], 0.0f, g_tint.bg) * 255.0f) * g_tint.s
                                       : g_tint.thunder ? 0.0f : g_tint.bg_amb[c];
        out[c] = sky_tinted(sh[c], add, g_tint.bg);
    }
}

static bool sky_weights(CPUState* cpu, bool scenery, int pale[4], float w[4]);

// HD's light for the sun shadows (aurora_set_sun_palette), as HD's terrain shaders take it (read in Cemu: a
// flat surface in shade takes the shadow colour, a wall half of it and half BG0 C0, and a lit one BG0 K0 times
// its factor, 3 at noon): the light from the sky HD's shade takes from above (the shadow colour, kept linear),
// the scenery's ambient, which its shade takes from the side (BG0 C0 times its factor), and the sun's light on
// scenery (BG0 K0 times its factor). BG0's from the room's palettes, the shadow colour from those under Link
// (in HD it is one for the whole picture: under Windfall's arch the street's shade follows Link's floor). The
// bytes and the factors are blended apart and multiplied after, as HD's are, tinted by the game's events
// (sky_tint_read), and taken to the shade by the day/night fade (BG0 K0 and the shadow colour to BG0 C0, as HD's
// scenery TEV structs). Only with stages.bin version 3 (the shadow colour in it).
static void sky_sun_palette(CPUState* cpu, bool on) {
    static bool s_sent;
    int pale[4], light_pale[4];
    float w[4], light_w[4];
    const bool ok = on && g_sky_data != NULL && g_sky_extra == 28u && sky_weights(cpu, true, pale, w);
    if (!ok) {
        if (s_sent)
            aurora_set_sun_palette(0, NULL, NULL, NULL);
        s_sent = false;
        return;
    }
    if (!sky_weights(cpu, false, light_pale, light_w)) {
        memcpy(light_pale, pale, sizeof pale);
        memcpy(light_w, w, sizeof w);
    }
    float c0[3], k0[3], sky[3], ground[3], lit[3], c0_bytes[3];
    sky_blend_bytes(pale, w, 0x06u, c0);
    sky_blend_bytes(pale, w, 0x09u, k0);
    sky_light(c0, k0, g_tint.bg_amb, g_tint.bg_dif, g_tint.bg, g_tint.bg, sky_blend_factor(pale, w, 2u),
              sky_blend_factor(pale, w, 3u), ground, lit, c0_bytes);
    sky_shadow_colour(light_pale, light_w, sky);
    for (u32 c = 0; c < 3u; ++c)
        sky[c] += g_sky_fade * (c0_bytes[c] - sky[c]);
    aurora_set_sun_palette(1, sky, ground, lit);
    s_sent = true;
}

// HD's light on characters (aurora_set_actor_light_hd), from the palettes the game's own light is blending
// (the floor under Link picks them, as for actors in the game, not each room's): the shade's ambient (actor C0
// times its factor), the sky HD's shade blends toward from above (the shadow colour, kept linear) and the lit
// colour (actor K0 times its factor, 2.75 at noon). Read from HD's toon shader in Cemu: a character's shade is
// a hemisphere from actor C0 below to the shadow colour above, its lit side K. Blended, tinted and faded as the
// sun's palette above (the shade x mActColRatio squared, the lit colour x mActColRatio, as HD's actor TEV
// structs; the shadow colour toward actor C0's bytes, its intensity kept). Only with stages.bin version 3.
static void sky_actor_palette(CPUState* cpu, bool on) {
    static bool s_sent;
    int pale[4];
    float w[4];
    const bool ok = on && g_sky_data != NULL && g_sky_extra == 28u && sky_weights(cpu, false, pale, w);
    if (!ok) {
        if (s_sent) {
            aurora_set_actor_light_hd(0, NULL, NULL, NULL);
            aurora_set_actor_light_hd_own(0, NULL, NULL);
        }
        s_sent = false;
        return;
    }
    float c0[3], k0[3], amb[3], shade[3], lit[3], shadow[3], c0_bytes[3];
    sky_blend_bytes(pale, w, 0x00u, c0);
    sky_blend_bytes(pale, w, 0x03u, k0);
    const float act2 = g_tint.act * g_tint.act;
    for (u32 c = 0; c < 3u; ++c)  // an item get takes the characters' shade toward black (HD's add of -C0 x s)
        amb[c] = g_tint.item ? -sky_tinted(c0[c], 0.0f, act2) * 255.0f * g_tint.s : g_tint.act_amb[c];
    sky_light(c0, k0, amb, g_tint.act_dif, act2, g_tint.act, sky_blend_factor(pale, w, 0u),
              sky_blend_factor(pale, w, 1u), shade, lit, c0_bytes);
    sky_shadow_colour(pale, w, shadow);
    for (u32 c = 0; c < 3u; ++c)
        shadow[c] += g_sky_fade * (c0_bytes[c] - shadow[c]);
    aurora_set_actor_light_hd(1, shade, shadow, lit);
    // The game's own global actor colours (g_env_light.mActorC0 and mActorK0: Link's, from the floor under him), that
    // each character's own toon registers are set against: HD lights a character by its own tevstr's colours (the
    // palettes of the floor under it, as the game's) times the global intensities (setLightTevColorType), so an
    // islander in the sun keeps the sun's light while Link stands where the floor names a darker palette (Windfall's
    // stalls and arch, envr 50).
    float own_c0[3], own_k0[3];
    for (u32 c = 0; c < 3u; ++c) {
        const float vc = (float)(s16)mem_read16(cpu, kEnvLight + 0xB18u + 2u * c);
        const float vk = (float)(s16)mem_read16(cpu, kEnvLight + 0xB20u + 2u * c);
        own_c0[c] = powf((vc < 0.0f ? 0.0f : vc > 255.0f ? 255.0f : vc) / 255.0f, 2.2f);
        own_k0[c] = powf((vk < 0.0f ? 0.0f : vk > 255.0f ? 255.0f : vk) / 255.0f, 2.2f);
    }
    aurora_set_actor_light_hd_own(setting("BLUEWAKE_HD_ACTOR_OWN") != 0, own_c0, own_k0);
    s_sent = true;
    static unsigned s_traced;
    if (g_trace && (s_traced++ % 60u == 0u || g_sky_fade > 0.0f || g_tint.s > 0.0f || g_tint.all != 1.0f ||
                    g_tint.bg_amb[0] != 0.0f || g_tint.act_amb[0] != 0.0f))
        fprintf(stderr,
                "[hd-look] actor light (retrace %llu) at %.2f: C0 %.3f %.3f %.3f shadow %.3f %.3f %.3f K %.3f %.3f %.3f; fade %.2f, "
                "all %.2f act %.2f bg %.2f, adds act %.0f/%.0f bg %.0f,%.0f,%.0f/%.0f%s%s s %.2f\n",
                g_retrace, read_float(cpu, kEnvTime), shade[0], shade[1], shade[2], shadow[0], shadow[1], shadow[2], lit[0], lit[1],
                lit[2], g_sky_fade, g_tint.all, g_tint.act, g_tint.bg, g_tint.act_amb[0], g_tint.act_dif[0],
                g_tint.bg_amb[0], g_tint.bg_amb[1], g_tint.bg_amb[2], g_tint.bg_dif[0], g_tint.item ? " item" : "",
                g_tint.arrow ? " arrow" : "", g_tint.s);
}

static const u8 kVirtColours[] = {0x10, 0x11, 0x12, 0x14, 0x15, 0x16, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};

// HD's float `index` for a Virt entry (0 cloud, 1 cloud centre, 2 sky, 3 haze, 4 false sea's alpha), from stages.bin
// version 4; 1 without them.
static float sky_float_at(int virt, u32 index) {
    if (g_sky_floats == NULL || virt < 0)
        return 1.0f;
    const u8* f = g_sky_floats + (u32)virt * 20u + index * 4u;
    const u32 bits = (u32)f[0] << 24 | (u32)f[1] << 16 | (u32)f[2] << 8 | f[3];
    float value;
    memcpy(&value, &bits, 4);
    return value >= 0.0f && value < 10.0f ? value : 1.0f;
}

// The Pale entries written last (the game's light's four, then the scenery's)
// and the Virt entries.
static int s_written_pale[8] = {-1, -1, -1, -1, -1, -1, -1, -1}, s_written_virt[4] = {-1, -1, -1, -1};

static void sky_linear_forget(void) {
    for (int i = 0; i < 8; ++i)
        s_written_pale[i] = -1;
    for (int i = 0; i < 4; ++i)
        s_written_virt[i] = -1;
}

// HD's noon sky is brighter than its palette's colour (0,210,255 on Windfall's
// dock against the palette's 00A5FF): the sky's colour in the Virt entries the
// game is blending goes up by BLUEWAKE_HD_SKY_GAIN (1.27), as far as HD's
// brightness for the hour is up from 1 toward noon's 1.65. Only the sky's
// colour: brightening all that has no depth (the sky pass) took the floating
// clouds to a clipped white where HD's keep their cream.
static float sky_gain(float brightness) {
    static float s_gain = -1.0f;
    if (s_gain < 0.0f) {
        const char* value = getenv("BLUEWAKE_HD_SKY_GAIN");
        s_gain = value != NULL && atof(value) > 0.0 ? (float)atof(value) : 1.27f;
    }
    const float up = (brightness - 1.0f) / 0.65f;
    return 1.0f + (s_gain - 1.0f) * (up < 0.0f ? 0.0f : up > 1.0f ? 1.0f : up);
}

// One set of four palettes' linear blend (sky_blend_state's), written into
// all four Pale entries, recorded from `written` on.
static void sky_linear_pale(CPUState* cpu, u32 pale_table, const int pale[4], const float w[4], int* written) {
    const u8* hd_pale = g_sky_data + g_sky_pale * 0x2Cu;
    u8 pale_out[0x21];
    float lin[0x21];
    for (u32 c = 0; c < 0x21u; ++c) {
        float v = 0.0f;
        for (int i = 0; i < 4; ++i)
            v += w[i] * sky_lin(hd_pale[(u32)pale[i] * 0x2Cu + c]);
        lin[c] = v;
        pale_out[c] = sky_gam(v);
    }
    // HD's day/night fade: the characters' and the scenery's lit colours (actor K0, BG0 K0) toward their shade.
    if (g_sky_fade > 0.0f)
        for (u32 c = 0; c < 3u; ++c) {
            pale_out[0x03u + c] = sky_gam(lin[0x03u + c] + g_sky_fade * (lin[0x00u + c] - lin[0x03u + c]));
            pale_out[0x09u + c] = sky_gam(lin[0x09u + c] + g_sky_fade * (lin[0x06u + c] - lin[0x09u + c]));
        }
    float c0 = 0.0f, k0 = 0.0f;
    for (int i = 0; i < 4; ++i) {
        c0 += w[i] * sky_factor_at(pale[i], 0u);
        k0 += w[i] * sky_factor_at(pale[i], 1u);
    }
    sky_actor_scale(pale_out, c0, k0 + g_sky_fade * (c0 - k0));
    for (int i = 0; i < 4; ++i) {
        for (u32 c = 0; c < 0x21u; ++c)
            mem_write8(cpu, pale_table + (u32)pale[i] * 0x2Cu + c, pale_out[c]);
        written[i] = pale[i];
    }
}

// Without the linear blend: each of the four Pale entries keeps its own colours, its character colours scaled
// (sky_actor_scale) and, while HD's day/night fade is up, its lit colours (actor K0, BG0 K0) taken toward its
// shade (in linear light), recorded from `written` on.
static void sky_actor_pale(CPUState* cpu, u32 pale_table, const int pale[4], int* written) {
    if (!sky_actor_light() && g_sky_fade <= 0.0f)
        return;
    const u8* hd_pale = g_sky_data + g_sky_pale * 0x2Cu;
    const u32 bytes = g_sky_fade > 0.0f ? 12u : 6u;
    for (int i = 0; i < 4; ++i) {
        u8 out[12];
        memcpy(out, hd_pale + (u32)pale[i] * 0x2Cu, sizeof out);
        float c0 = sky_factor_at(pale[i], 0u), k0 = sky_factor_at(pale[i], 1u);
        if (g_sky_fade > 0.0f) {
            for (u32 c = 0; c < 3u; ++c) {
                out[0x03u + c] = sky_gam(sky_lin(out[0x03u + c]) + g_sky_fade * (sky_lin(out[0x00u + c]) - sky_lin(out[0x03u + c])));
                out[0x09u + c] = sky_gam(sky_lin(out[0x09u + c]) + g_sky_fade * (sky_lin(out[0x06u + c]) - sky_lin(out[0x09u + c])));
            }
            k0 += g_sky_fade * (c0 - k0);
        }
        sky_actor_scale(out, c0, k0);
        for (u32 c = 0; c < bytes; ++c)
            mem_write8(cpu, pale_table + (u32)pale[i] * 0x2Cu + c, out[c]);
        written[i] = pale[i];
    }
}

static bool sky_weights(CPUState* cpu, bool scenery, int pale[4], float w[4]) {
    float ab, cb;
    if (!sky_blend_state(cpu, scenery, pale, &ab, &cb) || pale[0] < 0 || pale[1] < 0 || pale[2] < 0 || pale[3] < 0)
        return false;
    w[0] = (1.0f - ab) * (1.0f - cb);
    w[1] = ab * (1.0f - cb);
    w[2] = (1.0f - ab) * cb;
    w[3] = ab * cb;
    return true;
}

static void sky_linear(CPUState* cpu, u32 pale_table, u32 virt_table, bool on, float gain) {
    if (g_sky_data == NULL) {
        sky_linear_forget();
        return;
    }
    const bool linear_on = g_sky_linear;
    const u8* hd_pale = g_sky_data + g_sky_pale * 0x2Cu;
    const u8* hd_virt = hd_pale + g_sky_pale * 0x2Cu + g_sky_virt * 0x24u;
    // HD's own colours back where they were blended.
    for (int i = 0; i < 8; ++i) {
        if (on && s_written_pale[i] >= 0)
            for (u32 c = 0; c < 0x21u; ++c)
                mem_write8(cpu, pale_table + (u32)s_written_pale[i] * 0x2Cu + c, hd_pale[(u32)s_written_pale[i] * 0x2Cu + c]);
        s_written_pale[i] = -1;
    }
    for (int i = 0; i < 4; ++i) {
        if (on && virt_table != 0u && s_written_virt[i] >= 0)
            for (u32 c = 0; c < sizeof kVirtColours; ++c)
                mem_write8(cpu, virt_table + (u32)s_written_virt[i] * 0x24u + kVirtColours[c],
                           hd_virt[(u32)s_written_virt[i] * 0x24u + kVirtColours[c]]);
        s_written_virt[i] = -1;
    }
    int pale[4], room_pale[4];
    float w[4], room_w[4];
    if (!on || !sky_weights(cpu, false, pale, w))
        return;
    // The game's light's palettes, then the scenery's where its room names
    // others (they win an entry both name: the scenery fills the screen).
    // Each room's own entries, without a room change's fade: that is only for the host's one light (the sun's
    // palette, HD's brightness); faded here, the clearing past Forbidden Woods' door took the dark room's colours
    // into its own entries and went darker than either end halfway.
    g_room_fade_held = true;
    const bool room = sky_weights(cpu, true, room_pale, room_w) && memcmp(room_pale, pale, sizeof pale) != 0;
    g_room_fade_held = false;
    if (linear_on) {
        sky_linear_pale(cpu, pale_table, pale, w, s_written_pale);
        if (room)
            sky_linear_pale(cpu, pale_table, room_pale, room_w, s_written_pale + 4);
    } else {
        sky_actor_pale(cpu, pale_table, pale, s_written_pale);
        if (room)
            sky_actor_pale(cpu, pale_table, room_pale, s_written_pale + 4);
    }
    // The sky goes with the game's light.
    int virt[4];
    for (int i = 0; i < 4; ++i)
        virt[i] = virt_table != 0u && hd_pale[(u32)pale[i] * 0x2Cu + 0x21u] < g_sky_virt ? hd_pale[(u32)pale[i] * 0x2Cu + 0x21u] : -1;
    if (virt[0] < 0 || virt[1] < 0 || virt[2] < 0 || virt[3] < 0)
        return;
    // HD's haze (kasumi-mae, kVirtColours 12-14) is its colour times Virt float 3 (1.636 at dusk on the sea),
    // blended as the colours are and times mAllColRatio (float_kankyo_color_ratio_set), in linear light. HD's
    // target clamps it at 1 before the blend, as the bytes clamp at 255. A pack from before version 4 has no floats.
    float haze[4] = {1.0f, 1.0f, 1.0f, 1.0f}, haze_blend = 0.0f;
    for (int i = 0; i < 4; ++i) {
        if (g_sky_floats != NULL)
            haze[i] = sky_float_at(virt[i], 3u) * g_tint.all;
        haze_blend += w[i] * haze[i];
    }
    u8 virt_out[sizeof kVirtColours];
    for (u32 c = 0; c < sizeof kVirtColours; ++c) {
        float v = 0.0f;
        for (int i = 0; i < 4; ++i)
            v += w[i] * sky_lin(hd_virt[(u32)virt[i] * 0x24u + kVirtColours[c]]);
        virt_out[c] = sky_gam(c >= 12u ? v * haze_blend : v);
    }
    for (int i = 0; i < 4; ++i) {
        // The blend, or without it each entry's own colours (the game's blend
        // of them carries the sky's gain alike); the sky's colour (0x18-0x1A,
        // kVirtColours 6-8) times the gain.
        for (u32 c = 0; c < sizeof kVirtColours; ++c) {
            float v = linear_on ? virt_out[c] : hd_virt[(u32)virt[i] * 0x24u + kVirtColours[c]];
            if (!linear_on && c >= 12u && haze[i] != 1.0f)
                v = sky_gam(sky_lin((u8)v) * haze[i]);
            if (c >= 6u && c <= 8u)
                v = v * gain + 0.5f > 255.0f ? 255.0f : v * gain + 0.5f;
            mem_write8(cpu, virt_table + (u32)virt[i] * 0x24u + kVirtColours[c], (u8)v);
        }
        s_written_virt[i] = virt[i];
    }
    static unsigned s_haze_traced;
    if (g_trace && s_haze_traced++ % 60u == 0u) {
        const u32 at = virt_table + (u32)virt[0] * 0x24u + 0x1Eu;
        fprintf(stderr, "[hd-look] haze at %.1f: x%.3f, entry %d holds %u %u %u\n", read_float(cpu, kEnvTime), haze_blend,
                virt[0], mem_read8(cpu, at), mem_read8(cpu, at + 1u), mem_read8(cpu, at + 2u));
    }
}

// A kept Pale table is still the stage's while each entry's bytes after its
// colours (its Virt index and fog distances, which the blend never writes)
// are HD's: a stage loaded again at the same place brings the GameCube's.
static bool sky_kept(CPUState* cpu, u32 table) {
    const u8* hd_pale = g_sky_data + g_sky_pale * 0x2Cu;
    for (u32 i = 0; i < g_sky_pale; ++i)
        if (!guest_equals(cpu, table + i * 0x2Cu + 0x21u, hd_pale + i * 0x2Cu + 0x21u, 0x2Cu - 0x21u))
            return false;
    return true;
}

static u32 s_pale, s_virt;  // the tables the host keeps

// Wind Waker HD reads no floor palettes outside the dungeons: its dBgS::GetPolyColor returns 0xFF ("none") unless
// the stage type is 1 (wwhd d_bg_s.cpp; the GameCube's, 0x800A06A4, has no such check), so outdoors and in houses,
// boss and sub-dungeon stages Link and the characters stay on their room's palettes (Windfall's stalls and arch:
// the GameCube's envr 50, HD's room 11). With HD's palettes in, the collision's floor colours (bits 19-26 of each
// poly info's first word, which only GetPolyColor reads) are set to 0xFF on such stages, each original kept and
// written back when the look or the palettes go, or on a type 1 stage. Only collision still registered with the
// game's dBgS is touched. BLUEWAKE_HD_FLOOR_PALETTES=1 keeps the GameCube's floor palettes everywhere.
enum {
    kBgS = 0x803C5EA8u,     // g_dComIfG_gameInfo.play.mBgS (dBgS: m_chk_element[256], 0x14 each)
    kBgChkCount = 256u,
    kBgwBgd = 0x94u,        // cBgW::pm_bgd (cBgD_t)
    kBgdTiNum = 0x28u,      // cBgD_t::m_ti_num
    kBgdTiTbl = 0x2Cu,      // cBgD_t::m_ti_tbl (cBgD_Ti_t, 0x10 each; m_info0 first)
    kFloorColourMask = 0x07F80000u,
};
typedef struct FloorColours {
    u32 table, count;
    u8* saved;  // each entry's original colour (bits 19-26)
} FloorColours;
static FloorColours g_floor[kBgChkCount];

static void floor_colours_restore(CPUState* cpu, u32 slot, bool write) {
    FloorColours* f = &g_floor[slot];
    if (f->saved != NULL && write)
        for (u32 i = 0; i < f->count; ++i) {
            const u32 a = f->table + i * 0x10u;
            const u32 w = mem_read32(cpu, a);
            mem_write32(cpu, a, (w & ~kFloorColourMask) | ((u32)f->saved[i] << 19));
        }
    free(f->saved);
    memset(f, 0, sizeof *f);
}

static void floor_colours(CPUState* cpu, bool hd) {
    static int s_keep = -1;
    if (s_keep < 0)
        s_keep = setting("BLUEWAKE_HD_FLOOR_PALETTES") == 1;
    const u32 info = mem_read32(cpu, kStagInfo);
    const u32 type = guest_pointer(info) ? (mem_read32(cpu, info + 0x0Cu) >> 16) & 7u : 1u;
    const bool none = hd && !s_keep && type != 1u;
    for (u32 slot = 0; slot < kBgChkCount; ++slot) {
        const bool used = (mem_read32(cpu, kBgS + slot * 0x14u + 4u) & 1u) != 0u;  // m_flags: ChkUsed
        const u32 bgw = used ? mem_read32(cpu, kBgS + slot * 0x14u) : 0u;
        const u32 bgd = guest_pointer(bgw) ? mem_read32(cpu, bgw + kBgwBgd) : 0u;
        const u32 table = guest_pointer(bgd) ? mem_read32(cpu, bgd + kBgdTiTbl) : 0u;
        const u32 count = guest_pointer(table) ? mem_read32(cpu, bgd + kBgdTiNum) : 0u;
        FloorColours* f = &g_floor[slot];
        const bool same = f->saved != NULL && f->table == table && f->count == count;
        if (f->saved != NULL && (!same || !none))
            floor_colours_restore(cpu, slot, same);  // gone or replaced: its memory isn't the game's collision now
        if (!none || count == 0u || count > 0x10000u || f->saved != NULL)
            continue;
        bool shared = false;  // one model's collision for several (moving BG actors): kept by the first slot only
        for (u32 other = 0; other < kBgChkCount && !shared; ++other)
            shared = other != slot && g_floor[other].saved != NULL && g_floor[other].table == table;
        if (shared)
            continue;
        f->saved = (u8*)malloc(count);
        if (f->saved == NULL)
            continue;
        f->table = table;
        f->count = count;
        for (u32 i = 0; i < count; ++i) {
            const u32 a = table + i * 0x10u;
            const u32 w = mem_read32(cpu, a);
            f->saved[i] = (u8)((w & kFloorColourMask) >> 19);
            mem_write32(cpu, a, w | kFloorColourMask);
        }
        if (g_trace)
            fprintf(stderr, "[hd-look] floor palettes off (stage type %u): collision %u, %u poly infos\n", type, slot,
                    count);
    }
}

static void sky(CPUState* cpu) {
    if (g_sky_count == 0u)
        return;
    const u32 pale_now = mem_read32(cpu, kPaletInfo);
    if (s_pale != 0u && (pale_now != s_pale || g_sky_data == NULL || !sky_kept(cpu, s_pale))) {
        s_pale = s_virt = 0u;
        sky_linear_forget();
    }
    if (s_pale == 0u && !(guest_pointer(pale_now) && sky_pick(cpu, pale_now)))
        g_sky_data = NULL;
    u32 pale = 0u, virt = 0u;
    if (g_sky_data != NULL) {
        const u8* gc_pale = g_sky_data;
        const u8* hd_pale = gc_pale + g_sky_pale * 0x2Cu;
        const u8* gc_virt = hd_pale + g_sky_pale * 0x2Cu;
        const u8* hd_virt = gc_virt + g_sky_virt * 0x24u;
        // Last retrace's linear blend comes out before the GameCube's go back.
        if (!g_sky && s_pale != 0u)
            sky_linear(cpu, s_pale, s_virt, false, 1.0f);
        pale = sky_table(cpu, kPaletInfo, g_sky_pale, 0x2Cu, gc_pale, hd_pale, SKY_PALE_LOOSE, &s_pale);
        if (g_sky_virt != 0u)
            virt = sky_table(cpu, kVrboxInfo, g_sky_virt, 0x24u, gc_virt, hd_virt, SKY_VIRT_LOOSE, &s_virt);
    }
    g_sky_shown = g_sky && pale != 0u;
    g_sky_fade = g_sky_shown ? hd_daynight_fade(cpu) : 0.0f;
    if (g_sky_shown)
        sky_tint_read(cpu);
    // HD's brightness goes with its palettes: only while they are in place.
    const float exposure = g_sky_shown ? sky_brightness(cpu) : 0.0f;
    sky_sun_palette(cpu, g_sky_shown);
    sky_actor_palette(cpu, g_sky_shown);
    sky_linear(cpu, pale, virt, g_sky_shown, sky_gain(exposure));
    // The schedule goes with the palettes (back to the GameCube's when they go).
    sky_schedule(cpu, g_sky_shown);
    sky_models(cpu, g_sky_shown);
    // HD's fog curve goes with its fog distances (the Pale entries' bytes 0x21-0x2B, which HD retuned for the
    // square of the linear factor: on the GameCube's linear curve they gave about twice HD's fog at middle distance).
    static int s_fog = -1;
    const int fog = g_sky_shown && g_fog_curve;
    if (fog != s_fog) {
        aurora_set_fog_hd(fog);
        s_fog = fog;
    }
    // HD's fog starts below 0 (Ganon's Tower -5000, the Tower of the Gods -1000): the game's blend clamps them at 0
    // (float_kankyo_color_ratio_set), so the sun pass takes HD's start from the host: HD's entries' starts blended as
    // the game blends the palettes (its four entries and weights, the light's or the scenery's, whichever blend gives
    // the game's own end), unclamped, with the game's end (aurora_set_fog_hd_starts).
    {
        float pair[2] = {0.0f, 0.0f};
        const float game_start = read_float(cpu, kEnvFogStart), game_end = read_float(cpu, kEnvFogEnd);
        if (fog && g_sky_data != NULL && game_start == 0.0f && game_end > 0.0f) {
            const u8* hd_pale = g_sky_data + g_sky_pale * 0x2Cu;
            for (int scenery = 1; scenery >= 0 && pair[1] == 0.0f; --scenery) {
                int pale[4];
                float w[4], start = 0.0f, end = 0.0f;
                if (!sky_weights(cpu, scenery != 0, pale, w))
                    continue;
                for (int i = 0; i < 4; ++i) {
                    if ((u32)pale[i] >= g_sky_pale)
                        break;
                    const u8* e = hd_pale + (u32)pale[i] * 0x2Cu;
                    float v[2];
                    for (int k = 0; k < 2; ++k) {
                        const u32 bits = (u32)e[0x24 + 4 * k] << 24 | (u32)e[0x25 + 4 * k] << 16 |
                                         (u32)e[0x26 + 4 * k] << 8 | e[0x27 + 4 * k];
                        memcpy(&v[k], &bits, 4);
                    }
                    start += w[i] * v[0];
                    end += w[i] * v[1];
                }
                if (start < 0.0f && fabsf(end - game_end) <= 0.01f * game_end) {
                    pair[0] = start;
                    pair[1] = game_end;
                }
            }
        }
        static float s_pair[2] = {1.0f, 1.0f};
        if (pair[0] != s_pair[0] || pair[1] != s_pair[1]) {
            aurora_set_fog_hd_starts(pair[1] > 0.0f ? pair : NULL, pair[1] > 0.0f ? 1 : 0);
            if (g_trace)
                fprintf(stderr, "[hd-look] fog: game %.0f-%.0f, HD start %.0f\n", game_start, game_end, pair[0]);
            s_pair[0] = pair[0];
            s_pair[1] = pair[1];
        }
    }
    static unsigned s_clock;
    if (g_trace && g_sky_shown && ++s_clock % 60u == 0u)
        fprintf(stderr, "[hd-look] clock %.1f, HD brightness %.3f\n", read_float(cpu, kEnvTime), exposure);
    if (fabsf(exposure - g_sky_exposure) > 0.001f) {
        aurora_set_exposure(exposure);
        if (g_trace && fabsf(exposure - g_sky_exposure) > 0.02f)
            fprintf(stderr, "[hd-look] HD brightness %.3f at %.1f\n", exposure, read_float(cpu, kEnvTime));
        g_sky_exposure = exposure;
    }
}

// A save state was loaded: the tables and layers the host kept are this run's,
// not the state's, so each is found again at the next retrace (sky_pick,
// sky_table and sky_layer_shift know HD's as a state holds it). With HD's sky
// off, a state taken with it on holds HD's palettes: they are read from the
// pack too, so the GameCube's go back over them.
void bluewake_hd_look_state_loaded(bool reapply) {
    s_sky_adopt = reapply;
    if (reapply)
        sky_load();
    s_pale = s_virt = 0u;
    sky_linear_forget();
    memset(s_shifted, 0, sizeof s_shifted);
    g_daynight.started = false;
    g_room_fade.to = -1;  // no fade from the room before the load
    // The loaded RAM has its own collision bytes: forget what was kept, without writing it.
    for (u32 slot = 0; slot < kBgChkCount; ++slot)
        floor_colours_restore(NULL, slot, false);
}

// Wind Waker HD's far blur level for the hour (d_kankyo_4.cpp's preset blend): wwHD_basic's 1.2 from 08:00 to 18:00,
// wwHD_night's 0.65 from 20:00 to 06:00, blended linearly over 06:00-08:00 and 18:00-20:00 (the clock in degrees,
// 15 an hour). Used by Aurora's far blur on HD's ramp (DOL_AURORA_FAR_BLUR_HD=1, with an imported pack).
static void far_blur_level(CPUState* cpu) {
    static float s_level = -1.0f;
    const float day = 1.2f, night = 0.65f;
    const float t = read_float(cpu, kEnvTime);
    float level = night;
    if (t >= 90.0f && t < 120.0f)
        level = night + (day - night) * (t - 90.0f) / 30.0f;
    else if (t >= 120.0f && t < 270.0f)
        level = day;
    else if (t >= 270.0f && t < 300.0f)
        level = day + (night - day) * (t - 270.0f) / 30.0f;
    if (!(fabsf(level - s_level) <= 0.001f)) {
        aurora_set_far_blur_level(level);
        s_level = level;
    }
}

// Wind Waker HD's monotone (Frozen Hyrule), from the game's own state: while
// mDoGph_gInf_c::mMonotone is set (d_s_play.cpp: Hyrule, Hyroom and kenroom
// until the colours come back, the X stages, the warp out of Hyrule), HD draws
// its DOF pass with its monotone presets (d_kankyo_4.cpp), which RecompCore
// draws at the game's grey composite (aurora_set_monotone_hd()). In Hyroom HD
// blends from Monotone (sepia beyond 0 to 100) to MonotoneOff (34,500 to
// 34,800) as the game's rate goes from 400 to -600, so the colours come back
// as a sweep out from the camera: the blend chases 1 - (rate + 600) / 1000 by
// 0.0015 a game frame (HD's frameMonotoneRange 1000 and its step, read from its
// rpx; the extra step it adds is taken as 0), back to 0 at once when the rate
// turns back past -250, and 1 from Demo49's frame 610. Elsewhere HD draws
// MonotoneOut whole while the flag is set and ignores the rate.
enum {
    kFrameBufferTex = 0x803F68A0u,   // mDoGph_gInf_c::mFrameBufferTex (the grey composite's copy)
    kMonotone = 0x803F68B5u,         // mDoGph_gInf_c::mMonotone (u8)
    kMonotoneRate = 0x803F68B6u,     // mMonotoneRate (s16, 400 to -600)
    kMonotoneRateSpeed = 0x803F68B8u, // mMonotoneRateSpeed (s16)
    kDemoArcName = 0x803F6A7Cu,      // dStage_roomControl_c::mDemoArcName (char[8])
    kPlayDemo = 0x803C9430u,         // g_dComIfG_gameInfo.play.mDemo (dDemo_manager_c*), its mFrame at +0xD4
};

static void monotone(CPUState* cpu, unsigned long long retrace) {
    static bool s_sent;
    static float s_blend;
    static const float kMul[3] = {1.4f, 1.5f, 0.6f};
    if (!g_monotone || mem_read8(cpu, kMonotone) == 0u) {
        if (s_sent)
            aurora_set_monotone_hd(0, 0.8f, 0.0f, 0.0f, kMul, 1.0f, 0u);
        s_sent = false;
        s_blend = 0.0f;
        return;
    }
    char stage[9] = {0};
    for (u32 i = 0; i < 8u; ++i)
        stage[i] = (char)mem_read8(cpu, 0x803C9D3Cu + i);  // play.mCurStage.mName
    float start = 0.0f, end = 0.0f;
    if (strncmp(stage, "Hyroom", 8) == 0) {
        const int rate = (s16)mem_read16(cpu, kMonotoneRate);
        const int speed = (s16)mem_read16(cpu, kMonotoneRateSpeed);
        float target = 1.0f - (float)(rate + 600) / 1000.0f;
        target = target < 0.0f ? 0.0f : target > 1.0f ? 1.0f : target;
        if (speed < 0 && rate > -250)
            s_blend = 0.0f;
        else if (retrace % 2u == 0u)  // once a game frame (two retraces)
            s_blend = s_blend < target ? fminf(s_blend + 0.0015f, target) : fmaxf(s_blend - 0.0015f, target);
        char demo[9] = {0};
        for (u32 i = 0; i < 8u; ++i)
            demo[i] = (char)mem_read8(cpu, kDemoArcName + i);
        const u32 manager = mem_read32(cpu, kPlayDemo);
        if (strncmp(demo, "Demo49", 8) == 0 && manager >= 0x80000000u && manager < 0x81800000u &&
            (s32)mem_read32(cpu, manager + 0xD4u) >= 0x262)
            s_blend = 1.0f;
        start = 34500.0f * s_blend;
        end = 100.0f + 34700.0f * s_blend;
    }
    aurora_set_monotone_hd(1, 0.8f, start, end, kMul, 1.0f, mem_read32(cpu, kFrameBufferTex));
    s_sent = true;
    if (g_trace && retrace % 300u == 0u)
        fprintf(stderr, "[hd-look] monotone on in %s: rate %d speed %d blend %.4f, sepia from %.0f to %.0f\n", stage,
                (s16)mem_read16(cpu, kMonotoneRate), (s16)mem_read16(cpu, kMonotoneRateSpeed), s_blend, start, end);
}

void bluewake_hd_look_retrace(CPUState* cpu) {
    const unsigned long long retrace = ++g_retrace;
    if (cpu == NULL || cpu->ram == NULL)
        return;
    room_fade_update(cpu);
    if (bluewake_hd_distant_islands || g_islands_were_on || g_trace)
        islands(cpu, bluewake_hd_distant_islands, retrace);
    g_islands_were_on = bluewake_hd_distant_islands;
    if (g_palms || g_palms_were_on)
        palms(cpu, g_palms);
    g_palms_were_on = g_palms;
    clouds(cpu, retrace);
    sky(cpu);
    floor_colours(cpu, sky_palettes_shown());
    monotone(cpu, retrace);
    far_blur_level(cpu);
    if (g_trace && retrace % 300u == 0u)
        trace_islands(cpu, retrace);
}
