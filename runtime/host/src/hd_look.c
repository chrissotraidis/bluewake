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

static int setting(const char* key) {  // 1, 0, or -1 when unset
    const char* value = getenv(key);
    if (value == NULL || value[0] == '\0')
        return -1;
    return value[0] == '1';
}

// The HD look's strengths, softer than Aurora's own starting values: at full
// strength the look came out darker than the GameCube's, with a dark smudge
// under Link. Aurora reads these once when it starts, so they are set before
// that (main.c), whether or not the look is on yet; a value already set wins.
void bluewake_hd_look_defaults(void) {
    setenv("DOL_AURORA_AO_STRENGTH", "0.3", 0);
    setenv("DOL_AURORA_CLOUD_SHADOWS_STRENGTH", "0.15", 0);
    setenv("DOL_AURORA_BLOOM_STRENGTH", "0.3", 0);
    setenv("DOL_AURORA_FAR_BLUR_AMOUNT", "0.4", 0);
}

void bluewake_hd_look_reload(void) {
    const int all = setting("BLUEWAKE_HD_LOOK");
    const int islands = setting("BLUEWAKE_HD_DISTANT_ISLANDS");
    bluewake_hd_distant_islands = islands >= 0 ? islands == 1 : all == 1;
    g_trace = setting("BLUEWAKE_HD_TRACE") == 1;
    // The renderer's parts (Aurora's post_fx). Left to DOL_AURORA_BLOOM and
    // DOL_AURORA_FAR_BLUR and DOL_AURORA_AO while neither the switch nor the part is set.
    const int bloom = setting("BLUEWAKE_HD_BLOOM") >= 0 ? setting("BLUEWAKE_HD_BLOOM") : all;
    const int far_blur = setting("BLUEWAKE_HD_FAR_BLUR") >= 0 ? setting("BLUEWAKE_HD_FAR_BLUR") : all;
    const int ao = setting("BLUEWAKE_HD_AO") >= 0 ? setting("BLUEWAKE_HD_AO") : all;
    g_clouds = setting("BLUEWAKE_HD_CLOUD_SHADOWS") >= 0 ? setting("BLUEWAKE_HD_CLOUD_SHADOWS") : all;
    // Sun shadows are worked out from what is on screen, and on open ground
    // they drew long dark streaks, so the HD look leaves them off; only
    // BLUEWAKE_HD_SUN_SHADOWS=1 turns them on.
    g_sun = setting("BLUEWAKE_HD_SUN_SHADOWS") >= 0 ? setting("BLUEWAKE_HD_SUN_SHADOWS") : all >= 0 ? 0 : -1;
    // Sun shadows come from a shadow map (Aurora's post_fx), so what is off
    // screen still casts. DOL_AURORA_SUN_SHADOW_MAP=0 keeps the screen's
    // version alone, for comparing the two.
    const int sun_map = setting("DOL_AURORA_SUN_SHADOW_MAP") >= 0 ? setting("DOL_AURORA_SUN_SHADOW_MAP") : g_sun;
    g_palm_cull = setting("BLUEWAKE_HD_PALM_CULL");
    g_room_cull = setting("BLUEWAKE_HD_ROOM_CULL");
    g_edge = setting("BLUEWAKE_HD_EDGE_LIGHT") >= 0 ? setting("BLUEWAKE_HD_EDGE_LIGHT") : all;
    g_torch = setting("BLUEWAKE_HD_TORCH_LIGHT") >= 0 ? setting("BLUEWAKE_HD_TORCH_LIGHT") : all;
    if (bloom >= 0)
        aurora_set_bloom(bloom == 1);
    if (far_blur >= 0)
        aurora_set_far_blur(far_blur == 1);
    if (ao >= 0)
        aurora_set_ambient_occlusion(ao == 1);
    if (sun_map >= 0)
        aurora_set_sun_shadow_map(sun_map == 1);
    // The shadow quality (DOL_AURORA_SUN_SHADOW_QUALITY, also in the menu),
    // handed over when it changes: the renderer makes its map again at once.
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
// view: mNear +0xC8, mFovy (degrees) +0xD0, mAspect +0xD4, mInvViewMtx +0x170.
enum {
    kCurrentView = 0x803CA900u,
    kViewNear = 0xC8u,
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
                    fprintf(stderr, "[hd-look] camera eye %.0f %.0f %.0f fovy %.1f aspect %.3f near %.2f\n", inv[3],
                            inv[7], inv[11], fovy, aspect, near_plane);
                aurora_set_world_view(inv, tanf(fovy * 0.5f * 3.14159265f / 180.f), aspect, near_plane, 0.f,
                                      (float)retrace / 59.94f);
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
    if (g_trace && retrace % 300u == 0u)
        trace_islands(cpu, retrace);
}
