#include "hd_look.h"

#include <aurora/aurora.h>

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
static bool g_sky;  // HD's sky and light palettes (BLUEWAKE_HD_SKY)
static void sky_load(void);

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
    // HD textures and the look both on. Textures load once, so this holds
    // until BlueWake starts again.
    const char* pack = getenv("DOL_AURORA_TEXTURE_PACK");
    if (setting("BLUEWAKE_HD_CLOUDS") == 1 && setting("BLUEWAKE_HD_LOOK") == 1 && pack != NULL && pack[0] != '\0')
        setenv("DOL_AURORA_TEXTURE_PACK_OPTIONAL", "hd-clouds", 0);
}

void bluewake_hd_look_reload(void) {
    const int all = setting("BLUEWAKE_HD_LOOK");
    bluewake_hd_distant_islands = part("BLUEWAKE_HD_DISTANT_ISLANDS", all) == 1;
    g_trace = setting("BLUEWAKE_HD_TRACE") == 1;
    // The renderer's parts (Aurora's post_fx). Left to DOL_AURORA_BLOOM and
    // DOL_AURORA_FAR_BLUR and DOL_AURORA_AO while neither the switch nor the part is set.
    const int bloom = part("BLUEWAKE_HD_BLOOM", all);
    const int far_blur = part("BLUEWAKE_HD_FAR_BLUR", all);
    const int ao = part("BLUEWAKE_HD_AO", all);
    g_clouds = part("BLUEWAKE_HD_CLOUD_SHADOWS", all);
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
    if (g_sky)
        sky_load();
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
        const int quality = value == NULL || value[0] == '\0' ? 2 : strcmp(value, "low") == 0 ? 0
                            : strcmp(value, "medium") == 0 ? 1 : 2;
        if (quality != s_quality) {
            aurora_set_sun_shadow_quality(quality);
            s_quality = quality;
        }
    }
    // How dark the sun shadows are, as the player chose in the menu (HD's own
    // by default).
    aurora_set_sun_shadow_strength(setting_number("BLUEWAKE_HD_SHADOW_STRENGTH", 1.0f, 0.0f, 1.0f));
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

// The game's point lights nearest the camera (torches, fires, lanterns, bombs):
// up to 16 of x, y, z, radius, r, g, b, flicker for the renderer.
static void point_lights(CPUState* cpu, const float eye[3]) {
    float found[16 * 8];
    float distance[16];
    unsigned count = 0;
    for (u32 i = 0; i < 200u; ++i) {
        const u32 light = mem_read32(cpu, kPointLights + i * 4u);
        if (!guest_pointer(light))
            continue;
        const float x = read_float(cpu, light), y = read_float(cpu, light + 4u), z = read_float(cpu, light + 8u);
        const float radius = read_float(cpu, light + 0x14u);
        if (!(radius > 1.f && radius < 10000.f))  // the global light is 99,999
            continue;
        const float dx = x - eye[0], dy = y - eye[1], dz = z - eye[2];
        const float d = sqrtf(dx * dx + dy * dy + dz * dz) - radius;
        if (d > 6000.f)
            continue;
        unsigned slot = count < 16u ? count++ : 16u;
        if (slot == 16u) {  // full: replace the farthest if this one is nearer
            unsigned far = 0;
            for (unsigned k = 1; k < 16u; ++k)
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
        for (u32 c = 0; c < 3u; ++c) {
            const float v = (float)(s16)mem_read16(cpu, light + 0x0Cu + c * 2u) / 255.f;
            out[4 + c] = v < 0.f ? 0.f : v > 1.f ? 1.f : v;
        }
        out[7] = read_float(cpu, light + 0x18u) != 0.f ? 1.f : 0.f;
    }
    aurora_set_point_lights(found, count);
    static unsigned traced;
    if (g_trace && traced++ % 300u == 0u) {
        fprintf(stderr, "[hd-look] point lights near the camera: %u", count);
        for (unsigned k = 0; k < count && k < 4u; ++k)
            fprintf(stderr, " (%.0f %.0f %.0f r%.0f c%.2f,%.2f,%.2f)", found[k * 8], found[k * 8 + 1], found[k * 8 + 2],
                    found[k * 8 + 3], found[k * 8 + 4], found[k * 8 + 5], found[k * 8 + 6]);
        fprintf(stderr, "\n");
    }
}

static void clouds(CPUState* cpu, unsigned long long retrace) {
    bool show = false;
    bool torch_view = false;
    if (g_clouds == 1 || g_sun == 1 || g_edge == 1 || g_torch == 1) {
        const u32 info = mem_read32(cpu, kStagInfo);
        const u32 type = guest_pointer(info) ? (mem_read32(cpu, info + 0x0Cu) >> 16) & 7u : 2u;
        const u32 view = mem_read32(cpu, kCurrentView);
        const bool outdoors = type == 0u || type == 7u;
        if (g_trace && retrace % 300u == 0u)
            fprintf(stderr, "[hd-look] stage type %u (%s)\n", type, outdoors ? "outdoors" : "indoors");
        if ((outdoors || g_torch == 1) && guest_pointer(view)) {
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
                aurora_set_world_view(inv, tanf(fovy * 0.5f * 3.14159265f / 180.f), aspect, near_plane, 0.f,
                                      (float)retrace / 59.94f);
                // With the far plane the sun shadow map reads distances from the
                // depth exactly: taken as infinitely far, an island 8,000 units
                // off read hundreds of units too far and shaded itself in blocks.
                const float far_plane = read_float(cpu, view + kViewFar);
                aurora_set_world_view_far(far_plane > near_plane ? far_plane : 0.f);
                const float sx = read_float(cpu, kSunPos) - inv[3];
                const float sy = read_float(cpu, kSunPos + 4u) - inv[7];
                const float sz = read_float(cpu, kSunPos + 8u) - inv[11];
                aurora_set_sun_direction(sx, sy, sz);
                // The same in the camera's space (its axes are inv's columns),
                // for the room culling below.
                const float sl = sqrtf(sx * sx + sy * sy + sz * sz);
                if (sl > 0.0f) {
                    for (int a = 0; a < 3; ++a)
                        g_sun_view[a] = (inv[a] * sx + inv[4 + a] * sy + inv[8 + a] * sz) / sl;
                    g_sun_up = sy / sl;
                }
                if (g_torch == 1) {
                    const float eye[3] = {inv[3], inv[7], inv[11]};
                    point_lights(cpu, eye);
                    torch_view = true;
                }
                if (g_trace && retrace % 300u == 0u)
                    fprintf(stderr, "[hd-look] sun from the eye %.0f %.0f %.0f\n", sx, sy, sz);
                show = outdoors;
            }
        }
    }
    const bool clouds_on = show && g_clouds == 1, sun_on = show && g_sun == 1, edge_on = show && g_edge == 1;
    const bool torch_on = torch_view && g_torch == 1;
    if (torch_on != g_torch_shown && g_torch >= 0) {
        aurora_set_torch_light(torch_on);
        g_torch_shown = torch_on;
    }
    if (clouds_on != g_clouds_shown && g_clouds >= 0) {
        aurora_set_cloud_shadows(clouds_on);
        g_clouds_shown = clouds_on;
    }
    // The room model's widened culling goes with the sun shadows on the high
    // shadow quality (DOL_AURORA_SUN_SHADOW_QUALITY unset or high).
    const char* quality = getenv("DOL_AURORA_SUN_SHADOW_QUALITY");
    const bool high = quality == NULL || quality[0] == '\0' || strcmp(quality, "high") == 0;
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
static u8* g_sky_data;  // the file: GC Pale, HD Pale, GC Virt, HD Virt, HD's Pale factors
static u32 g_sky_pale, g_sky_virt;
static bool g_sky_shown;  // the tables hold HD's (as last seen)
static float g_sky_exposure;  // what aurora_set_exposure was last given

static void sky_load(void) {
    static bool tried;
    if (tried)
        return;
    tried = true;
    const char* pack = getenv("DOL_AURORA_TEXTURE_PACK");
    if (pack == NULL || pack[0] == '\0')
        return;
    char path[4096];
    snprintf(path, sizeof path, "%s/optional/hd-sky/sea.bin", pack);
    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return;
    u8 head[16];
    if (fread(head, 1, 16, f) == 16 && memcmp(head, "BWSK", 4) == 0 && head[7] == 2u) {
        g_sky_pale = (u32)head[8] << 24 | (u32)head[9] << 16 | (u32)head[10] << 8 | head[11];
        g_sky_virt = (u32)head[12] << 24 | (u32)head[13] << 16 | (u32)head[14] << 8 | head[15];
        const size_t size = 2u * (g_sky_pale * 0x2Cu + g_sky_virt * 0x24u) + g_sky_pale * 24u;
        if (g_sky_pale < 256u && g_sky_virt < 256u && (g_sky_data = malloc(size)) != NULL &&
            fread(g_sky_data, 1, size, f) != size) {
            free(g_sky_data);
            g_sky_data = NULL;
        }
    }
    fclose(f);
    fprintf(stderr, "[hd-look] HD sky palettes: %s\n", g_sky_data ? path : "none in the pack");
}

static bool guest_equals(CPUState* cpu, u32 address, const u8* bytes, u32 size) {
    for (u32 i = 0; i < size; ++i)
        if (mem_read8(cpu, address + i) != bytes[i])
            return false;
    return true;
}

// One table: when it holds `from`, write `to` over it.
// One table: when it holds `from`, write `to` over it. True while it holds `to`.
static bool sky_table(CPUState* cpu, u32 pointer, u32 want, u32 entry, const u8* from, const u8* to) {
    const u32 table = mem_read32(cpu, pointer);
    if (!guest_pointer(table))
        return false;
    if (guest_equals(cpu, table, to, entry))
        return guest_equals(cpu, table, to, want * entry);
    if (!guest_equals(cpu, table, from, entry) || !guest_equals(cpu, table, from, want * entry))
        return false;
    for (u32 i = 0; i < want * entry; ++i)
        mem_write8(cpu, table + i, to[i]);
    if (g_trace)
        fprintf(stderr, "[hd-look] sky table at 0x%08X: %s\n", table, g_sky ? "HD" : "GameCube");
    return true;
}

// HD's brightness now: its factor for the sun's light on scenery (BG0 K0) in
// each palette the game is blending, blended as the game blends their colours
// (dScnKy_env_light_c::setLight_palno_get and setLight): the hour's two
// palettes of the last weather and of the new one, by the hour, then by the
// weather's change. In g_env_light (0x803E4AB4). As the scene's brightness
// after gamma (1/2.2): 3 at noon is 1.65, 2 in the morning 1.37.
enum {
    kEnvLight = 0x803E4AB4u,
    kEnvPselect = kEnvLight + 0x008u,   // mpPselectInfo (0xC each: palette ids[8], change rate)
    kEnvEnvr = kEnvLight + 0x00Cu,      // mpEnvrInfo (8 each: pselect ids)
    kEnvSchedule = kEnvLight + 0x014u,  // mpSchejule (11 of 0xC: end time, begin time, palette slots 0 and 1)
    kEnvColPatBlend = kEnvLight + 0xBE0u,
    kEnvTime = kEnvLight + 0xC38u,
    kEnvColpatPrev = kEnvLight + 0xC84u,
    kEnvColpatCurr = kEnvLight + 0xC85u,
    kEnvEnvrPrev = kEnvLight + 0xC88u,
    kEnvEnvrCurr = kEnvLight + 0xC89u,
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

static void sky_layer_shift(CPUState* cpu, u32 mtx, bool hd) {
    int at = -1, free_slot = -1;
    for (int i = 0; i < 8; ++i) {
        if (s_shifted[i] == mtx)
            at = i;
        else if (s_shifted[i] == 0u && free_slot < 0)
            free_slot = i;
    }
    if (hd && at < 0 && free_slot >= 0) {
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
static void sky_layer_wrap(CPUState* cpu, u32 material, u32 m, bool hd) {
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
        sky_layer_shift(cpu, mtx, hd);
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
        sky_layer_dl(cpu, material, hd);
        sky_layer_wrap(cpu, material, m, hd);
        const u32 s1 = s0 + 8u, s3 = s0 + 24u;
        // Stage 1 colour: a, b | c, d. GameCube: CPREV, ZERO | TEXC, TEXC (screen); HD: ZERO, ZERO | ZERO, TEXC.
        const u8 ab = mem_read8(cpu, s1 + 2u), cd = mem_read8(cpu, s1 + 3u);
        // Stage 3 alpha: a<<5 | b<<2 | c>>1, then (c&1)<<7 | d<<4 | swap. GameCube: ZERO, APREV, TEXA, ZERO
        // (a product); HD: ZERO, ZERO, ZERO, TEXA.
        const u8 aab = mem_read8(cpu, s3 + 6u), sw = mem_read8(cpu, s3 + 7u);
        const bool gc = ab == 0x0Fu && cd == 0x88u && aab == 0xE2u && (sw & 0xF0u) == 0x70u;
        const bool is_hd = ab == 0xFFu && cd == 0xF8u && aab == 0xFFu && (sw & 0xF0u) == 0xC0u;
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

static void sky_models(CPUState* cpu, bool hd) {
    static int s_on = -1;  // BLUEWAKE_HD_SKY_MODELS=0 leaves the models as they are
    if (s_on < 0)
        s_on = setting("BLUEWAKE_HD_SKY_MODELS") != 0;
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
                sky_layer_material(cpu, data, hd && s_on);
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
            const float want = hd && s_on ? kSkyJoints[k].hd : kSkyJoints[k].gc;
            if (now != want && (now == kSkyJoints[k].gc || now == kSkyJoints[k].hd))
                write_float(cpu, joint + 0x20u, want);
        }
    }
    if (!clouds_seen)
        memset(s_shifted, 0, sizeof s_shifted);
}

static float sky_factor(CPUState* cpu, u32 envr_idx, u32 colpat, u32 slot) {
    const u32 envr = mem_read32(cpu, kEnvEnvr), pselect = mem_read32(cpu, kEnvPselect);
    if (!guest_pointer(envr) || !guest_pointer(pselect) || colpat > 7u || slot > 5u)
        return 1.0f;
    const u32 psel = mem_read8(cpu, envr + envr_idx * 8u + colpat);
    const u32 pale = mem_read8(cpu, pselect + psel * 0xCu + slot);
    if (pale >= g_sky_pale)
        return 1.0f;
    const u8* f = g_sky_data + 2u * (g_sky_pale * 0x2Cu + g_sky_virt * 0x24u) + pale * 24u + 3u * 4u;
    const u32 bits = (u32)f[0] << 24 | (u32)f[1] << 16 | (u32)f[2] << 8 | f[3];
    float value;
    memcpy(&value, &bits, 4);
    return value > 0.0f && value < 10.0f ? value : 1.0f;
}

static float sky_brightness(CPUState* cpu) {
    const u32 schedule = mem_read32(cpu, kEnvSchedule);
    if (!guest_pointer(schedule))
        return 0.0f;
    const float now = read_float(cpu, kEnvTime);
    for (u32 i = 0; i < 11u; ++i) {
        const u32 at = schedule + i * 0xCu;
        const float end = read_float(cpu, at), begin = read_float(cpu, at + 4u);
        if (!(now >= end && now <= begin))
            continue;
        const u32 slot0 = mem_read8(cpu, at + 8u), slot1 = mem_read8(cpu, at + 9u);
        float ab = 1.0f;  // get_parcent(begin, end, now)
        if (begin - end != 0.0f) {
            ab = 1.0f - (begin - now) / (begin - end);
            if (ab >= 1.0f)
                ab = 1.0f;
        }
        const u32 envr0 = mem_read8(cpu, kEnvEnvrPrev), envr1 = mem_read8(cpu, kEnvEnvrCurr);
        const u32 pat0 = mem_read8(cpu, kEnvColpatPrev), pat1 = mem_read8(cpu, kEnvColpatCurr);
        const float a = sky_factor(cpu, envr0, pat0, slot0), b = sky_factor(cpu, envr0, pat0, slot1);
        const float c = sky_factor(cpu, envr1, pat1, slot0), d = sky_factor(cpu, envr1, pat1, slot1);
        float blend = read_float(cpu, kEnvColPatBlend);
        blend = blend < 0.0f ? 0.0f : blend > 1.0f ? 1.0f : blend;
        const float prev = a + ab * (b - a), curr = c + ab * (d - c);
        const float factor = prev + blend * (curr - prev);
        static float s_scale = -1.0f;  // BLUEWAKE_HD_SKY_GAMMA: the power the factor is taken to (1/2.2)
        if (s_scale < 0.0f) {
            const char* value = getenv("BLUEWAKE_HD_SKY_GAMMA");
            s_scale = value != NULL && atof(value) > 0.0 ? (float)atof(value) : 1.0f / 2.2f;
        }
        return powf(factor, s_scale);
    }
    return 0.0f;
}

static void sky(CPUState* cpu) {
    if (g_sky_data == NULL)
        return;
    const u8* gc_pale = g_sky_data;
    const u8* hd_pale = gc_pale + g_sky_pale * 0x2Cu;
    const u8* gc_virt = hd_pale + g_sky_pale * 0x2Cu;
    const u8* hd_virt = gc_virt + g_sky_virt * 0x24u;
    const bool pale = sky_table(cpu, kPaletInfo, g_sky_pale, 0x2Cu, g_sky ? gc_pale : hd_pale, g_sky ? hd_pale : gc_pale);
    sky_table(cpu, kVrboxInfo, g_sky_virt, 0x24u, g_sky ? gc_virt : hd_virt, g_sky ? hd_virt : gc_virt);
    g_sky_shown = g_sky && pale;
    // The schedule goes with the palettes (back to the GameCube's when they go).
    sky_schedule(cpu, g_sky_shown);
    sky_models(cpu, g_sky_shown);
    // HD's brightness goes with its palettes: only while they are in place.
    const float exposure = g_sky_shown ? sky_brightness(cpu) : 0.0f;
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

void bluewake_hd_look_retrace(CPUState* cpu) {
    static unsigned long long retrace;
    retrace++;
    if (cpu == NULL || cpu->ram == NULL)
        return;
    if (bluewake_hd_distant_islands || g_islands_were_on || g_trace)
        islands(cpu, bluewake_hd_distant_islands, retrace);
    g_islands_were_on = bluewake_hd_distant_islands;
    if (g_palms || g_palms_were_on)
        palms(cpu, g_palms);
    g_palms_were_on = g_palms;
    clouds(cpu, retrace);
    sky(cpu);
    if (g_trace && retrace % 300u == 0u)
        trace_islands(cpu, retrace);
}
