#ifndef BLUEWAKE_STAGE_SELECT_H
#define BLUEWAKE_STAGE_SELECT_H

#include "core/cpu.h"
#include "stage_select_key.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The developers' stage select (d_s_menu.cpp, fpcNm_MENU_SCENE_e) is in the
// retail game with its data (/res/Menu/Menu1.dat), but nothing goes there.
// BLUEWAKE_STAGE_SELECT=1 makes it reachable: when the file select
// starts play (dScnName_c::changeGameScene asks for the play scene, or the
// opening scene for a new game), the request becomes the menu scene. The
// save is set up by then (loaded from the card, or a new one), so the place
// picked from the menu is reached with it. Only the scene argument changes.
// Only while R on controller 1 is held as the file starts: otherwise the file
// loads as usual. (The menu uses R and L for the start point, which is easy to
// spot and harmless if R is still held when it opens.)
//
// BLUEWAKE_WARP goes to one place with no menu. changeGameScene has just set
// the next stage to the save's restart place (dComIfGs_gameStart), so the
// hook writes the place over it and the game starts there, as it would from
// the menu. The place is "stage:room:point[:layer]" (the game's own codes),
// or "group.room" or a name from the names file below, whose lines carry the
// codes. BLUEWAKE_WARP_TIME sets the time of day (an hour, or morning, noon,
// evening, night). BLUEWAKE_WARP_FILE=N presses A until the file starts and
// picks quest log N in the file select, so no one has to be at the controls.
#define BLUEWAKE_STAGE_SELECT_CHANGE 0x80029E6Cu    // fopScnM_ChangeReq
#define BLUEWAKE_STAGE_SELECT_NAME_RETURN 0x802322ECu // after its call in changeGameScene
#define BLUEWAKE_STAGE_SELECT_PLAY 7u                 // fpcNm_PLAY_SCENE_e
#define BLUEWAKE_STAGE_SELECT_OPEN 14u                // fpcNm_OPEN_SCENE_e (a new game's start)
#define BLUEWAKE_STAGE_SELECT_MENU 6u                 // fpcNm_MENU_SCENE_e
#define BLUEWAKE_STAGE_SELECT_DATA_SELECT 0x80181634u // dFile_select_c::dataSelect
#define BLUEWAKE_STAGE_SELECT_SELECT_NUM 0x3922u      // dFile_select_c::selectNum
#define BLUEWAKE_STAGE_SELECT_PAD1_HOLD 0x803A4E20u   // g_mDoCPd_cpadInfo[0].mButtonHold
#define BLUEWAKE_STAGE_SELECT_PAD_R 0x0400u        // R in mButtonHold's own bit order (measured)
#define BLUEWAKE_STAGE_SELECT_PAD_A 0x0100u
#define BLUEWAKE_STAGE_SELECT_PAD_START 0x1000u
#define BLUEWAKE_STAGE_SELECT_NEXT_STAGE 0x803C9D48u  // play.mNextStage: name[8], s16 point, s8 room, s8 layer
#define BLUEWAKE_STAGE_SELECT_SAVE_TIME 0x803C4C2Cu   // dSv_player_status_b_c::mTime (degrees, 15 an hour)

static bool bluewake_stage_select_armed;
// True only when the stage select can do anything this run (BLUEWAKE_STAGE_SELECT=1 or a
// BLUEWAKE_WARP), decided once at attach. host_chassis_edge_service runs once per guest
// block, so the per-block hooks below test this one flag first and cost a single
// predicted branch when the stage select is off.
static bool bluewake_stage_select_live;

// BLUEWAKE_STAGE_SELECT_NAMES=path: names for the menu's lines in place of
// the Japanese ones in Menu1.dat, written over them in memory each time the
// menu has loaded its list. A line of the file is "group<TAB>room<TAB>name",
// room "-" for the group's own name; up to 31 bytes of plain ASCII, which the
// menu's font draws. A room's line can go on with its place (stage, room
// number, point, layer: what the menu entry holds, for BLUEWAKE_WARP) and the
// spawn ids the room has (comma-separated, for R/L below), and a note.
#define BLUEWAKE_STAGE_SELECT_EXECUTE 0x8022ED50u // dScnMenu_Execute
#define BLUEWAKE_STAGE_SELECT_NAME_MAX 640u
typedef struct BluewakeStageSelectName {
    u8 group;
    s16 room; // -1: the group's name
    char text[32];
    bool has_place; // the line's optional stage, room number, point and layer
    char stage[8];
    s8 room_no;
    s16 point;
    s8 layer;
    bool has_spawns; // the room's spawn ids (its own and its stage's), as dStage_playerInit finds them
    u8 spawns[32];
    char note[12]; // shown after the name: (before), (after), (crash), (hangs)
} BluewakeStageSelectName;
static BluewakeStageSelectName bluewake_stage_select_names[BLUEWAKE_STAGE_SELECT_NAME_MAX];
static unsigned bluewake_stage_select_name_count;
static u32 bluewake_stage_select_named_list;

// "stage<sep>room<sep>point[<sep>layer]"; the layer is -1 (the story's) when left out.
static inline bool bluewake_stage_select_parse_place(const char* text, char sep, char stage[8], s8* room,
                                                     s16* point, s8* layer) {
    unsigned n = 0;
    for (; *text != '\0' && *text != sep; ++text) {
        if (n >= 7u)
            return false;
        stage[n++] = *text;
    }
    if (n == 0u || *text != sep)
        return false;
    while (n < 8u)
        stage[n++] = '\0';
    long values[3] = {0, 0, -1};
    for (int k = 0; k < 3; ++k) {
        char* end = NULL;
        values[k] = strtol(text + 1, &end, 10);
        if (end == text + 1)
            return false;
        text = end;
        if (*text != sep)
            break;
        if (k == 2)
            return false;
    }
    if (*text != '\0' && *text != '\r' && *text != '\n')
        return false;
    if (values[0] < -1 || values[0] > 127 || values[1] < -1 || values[1] > 255 || values[2] < -1 ||
        values[2] > 15)
        return false;
    *room = (s8)values[0];
    *point = (s16)values[1];
    *layer = (s8)values[2];
    return true;
}

static inline BluewakeStageSelectName* bluewake_stage_select_find_line(long group, long room) {
    for (unsigned k = 0; k < bluewake_stage_select_name_count; ++k)
        if (bluewake_stage_select_names[k].group == group && bluewake_stage_select_names[k].room == room)
            return &bluewake_stage_select_names[k];
    return NULL;
}

// One names file. A line already named by an earlier file keeps its place,
// spawn ids and note unless this line has them too: a later file replaces
// names only.
static inline void bluewake_stage_select_load_names(const char* path, bool required) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        if (required)
            fprintf(stderr, "[stage-select] cannot open names file %s\n", path);
        return;
    }
    char line[512];
    unsigned read = 0;
    while (fgets(line, sizeof line, file) != NULL) {
        char* end = NULL;
        if (line[0] == '#')
            continue;
        const long group = strtol(line, &end, 10);
        if (end == line || *end != '\t' || group < 0 || group > 255)
            continue;
        char* room_text = end + 1;
        long room = -1;
        if (*room_text == '-') {
            end = room_text + 1;
        } else {
            room = strtol(room_text, &end, 10);
            if (end == room_text || room < 0 || room > 255)
                continue;
        }
        if (*end != '\t')
            continue;
        // Split the rest at tabs: name, stage, room number, point, layer, spawn ids, note.
        char* column[7] = {NULL};
        unsigned columns = 0;
        for (char* c = end + 1; c != NULL && columns < 7u;) {
            column[columns++] = c;
            c = strchr(c, '\t');
            if (c != NULL)
                *c++ = '\0';
        }
        for (unsigned k = 0; k < columns; ++k)
            column[k][strcspn(column[k], "\r\n")] = '\0';
        BluewakeStageSelectName* name = bluewake_stage_select_find_line(group, room);
        if (name == NULL) {
            if (bluewake_stage_select_name_count >= BLUEWAKE_STAGE_SELECT_NAME_MAX)
                break;
            name = &bluewake_stage_select_names[bluewake_stage_select_name_count++];
            memset(name, 0, sizeof *name);
            name->group = (u8)group;
            name->room = (s16)room;
        }
        unsigned n = 0;
        for (const char* c = column[0]; *c != '\0'; ++c)
            if ((unsigned char)*c >= 0x20u && (unsigned char)*c < 0x7Fu && n < 31u)
                name->text[n++] = *c;
        name->text[n] = '\0';
        read++;
        if (room < 0 || columns < 5u)
            continue;
        char place[64];
        snprintf(place, sizeof place, "%s\t%s\t%s\t%s", column[1], column[2], column[3], column[4]);
        name->has_place = bluewake_stage_select_parse_place(place, '\t', name->stage, &name->room_no,
                                                            &name->point, &name->layer);
        if (columns >= 6u && name->has_place) {
            memset(name->spawns, 0, sizeof name->spawns);
            name->has_spawns = false;
            for (char* id_text = column[5]; id_text != NULL && *id_text >= '0' && *id_text <= '9';) {
                char* id_end = NULL;
                const long id = strtol(id_text, &id_end, 10);
                if (id >= 0 && id <= 255) {
                    name->spawns[id >> 3] |= (u8)(1u << (id & 7));
                    name->has_spawns = true;
                }
                id_text = *id_end == ',' ? id_end + 1 : NULL;
            }
        }
        if (columns >= 7u)
            snprintf(name->note, sizeof name->note, "%s", column[6]);
    }
    fclose(file);
    fprintf(stderr, "[stage-select] %u names from %s\n", read, path);
}

// BLUEWAKE_STAGE_SELECT_NAMES is a names file, or a folder holding names.tsv
// (ours, with the places), then names-randomizer.tsv and names-decomp.tsv,
// whose names replace ours for the rooms they cover.
static inline void bluewake_stage_select_load_names_from(const char* path) {
    static const char* const files[] = {"names.tsv", "names-randomizer.tsv", "names-decomp.tsv"};
    char file[1024];
    snprintf(file, sizeof file, "%s/%s", path, files[0]);
    FILE* probe = fopen(file, "rb");
    if (probe == NULL) {
        bluewake_stage_select_load_names(path, true);
        return;
    }
    fclose(probe);
    for (unsigned k = 0; k < sizeof files / sizeof files[0]; ++k) {
        snprintf(file, sizeof file, "%s/%s", path, files[k]);
        bluewake_stage_select_load_names(file, k == 0u);
    }
}

static bool bluewake_warp_set;
static char bluewake_warp_stage[8];
static s8 bluewake_warp_room;
static s16 bluewake_warp_point;
static s8 bluewake_warp_layer;
static int bluewake_warp_time = -1; // degrees, 15 an hour; -1 keeps the save's
static unsigned bluewake_warp_file; // 1-3; 0 when no one presses for the player
static bool bluewake_warp_file_pending;
// BLUEWAKE_WARP_STOP_AFTER=N ends the run N play-scene frames after the warp
// arrives (its execute runs), for sweeps that try many places one run each.
static bool bluewake_warp_applied;
static unsigned bluewake_warp_frames;
static unsigned bluewake_warp_stop_after;
static bool bluewake_stage_select_stop; // main.c's loop ends the run

static inline char bluewake_stage_select_lower(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

// Whether text starts with part (whole: and ends there), ignoring case.
static inline bool bluewake_stage_select_starts(const char* text, const char* part, bool whole) {
    for (; *text != '\0' && *part != '\0'; ++text, ++part)
        if (bluewake_stage_select_lower(*text) != bluewake_stage_select_lower(*part))
            return false;
    return *part == '\0' && (!whole || *text == '\0');
}

static inline bool bluewake_stage_select_matches(const char* text, const char* part, bool whole) {
    if (whole)
        return bluewake_stage_select_starts(text, part, true);
    for (; *text != '\0'; ++text)
        if (bluewake_stage_select_starts(text, part, false))
            return true;
    return false;
}

// "group.room", or a room's name: the whole name if one room has it, else a
// part of a name that only one room has.
static inline const BluewakeStageSelectName* bluewake_stage_select_find(const char* text) {
    char* end = NULL;
    const long group = strtol(text, &end, 10);
    if (end != text && *end == '.') {
        char* room_end = NULL;
        const long room = strtol(end + 1, &room_end, 10);
        if (room_end != end + 1 && *room_end == '\0') {
            for (unsigned k = 0; k < bluewake_stage_select_name_count; ++k) {
                const BluewakeStageSelectName* name = &bluewake_stage_select_names[k];
                if (name->has_place && name->group == group && name->room == room)
                    return name;
            }
            return NULL;
        }
    }
    for (int whole = 1; whole >= 0; --whole) {
        const BluewakeStageSelectName* found = NULL;
        unsigned matches = 0;
        for (unsigned k = 0; k < bluewake_stage_select_name_count; ++k) {
            const BluewakeStageSelectName* name = &bluewake_stage_select_names[k];
            if (name->has_place && bluewake_stage_select_matches(name->text, text, whole)) {
                found = name;
                matches++;
            }
        }
        if (matches == 1u)
            return found;
        if (matches > 1u) {
            fprintf(stderr, "[warp] \"%s\" names %u rooms; use one of:", text, matches);
            for (unsigned k = 0; k < bluewake_stage_select_name_count; ++k) {
                const BluewakeStageSelectName* name = &bluewake_stage_select_names[k];
                if (name->has_place && bluewake_stage_select_matches(name->text, text, whole))
                    fprintf(stderr, " %u.%d (%s)", name->group, name->room, name->text);
            }
            fprintf(stderr, "\n");
            return NULL;
        }
    }
    return NULL;
}

static inline void bluewake_stage_select_attach(void) {
    // On from the launch environment, or from the settings toggle (Mac and Linux save it under its
    // own key, so a saved setting never overrides BLUEWAKE_STAGE_SELECT=1 on the command line).
    const char* on = getenv("BLUEWAKE_STAGE_SELECT");
    const char* toggle = getenv("BLUEWAKE_STAGE_SELECT_TOGGLE");
    bluewake_stage_select_armed = (on != NULL && on[0] == '1') || (toggle != NULL && toggle[0] == '1');
    const char* warp = getenv("BLUEWAKE_WARP");
    const bool warp_asked = warp != NULL && warp[0] != '\0';
    const char* names = getenv("BLUEWAKE_STAGE_SELECT_NAMES");
    if ((bluewake_stage_select_armed || warp_asked) && names != NULL && names[0] != '\0')
        bluewake_stage_select_load_names_from(names);
    if (warp_asked) {
        if (strchr(warp, ':') != NULL) {
            bluewake_warp_set = bluewake_stage_select_parse_place(warp, ':', bluewake_warp_stage, &bluewake_warp_room,
                                                                  &bluewake_warp_point, &bluewake_warp_layer);
        } else {
            const BluewakeStageSelectName* name = bluewake_stage_select_find(warp);
            if (name != NULL) {
                memcpy(bluewake_warp_stage, name->stage, sizeof bluewake_warp_stage);
                bluewake_warp_room = name->room_no;
                bluewake_warp_point = name->point;
                bluewake_warp_layer = name->layer;
                bluewake_warp_set = true;
                fprintf(stderr, "[warp] %s is %u.%d \"%s\"\n", warp, name->group, name->room, name->text);
            }
        }
        if (bluewake_warp_set)
            fprintf(stderr, "[warp] starting a file goes to %.8s room %d point %d layer %d\n", bluewake_warp_stage,
                    bluewake_warp_room, bluewake_warp_point, bluewake_warp_layer);
        else
            fprintf(stderr,
                    "[warp] cannot read BLUEWAKE_WARP=%s: expected stage:room:point[:layer], or group.room or a "
                    "name from BLUEWAKE_STAGE_SELECT_NAMES\n",
                    warp);
    }
    const char* time = getenv("BLUEWAKE_WARP_TIME");
    if (bluewake_warp_set && time != NULL && time[0] != '\0') {
        static const struct {
            const char* name;
            int hour;
        } named[] = {{"morning", 6}, {"noon", 12}, {"evening", 18}, {"night", 22}};
        for (unsigned k = 0; k < sizeof named / sizeof named[0]; ++k)
            if (bluewake_stage_select_starts(time, named[k].name, true))
                bluewake_warp_time = named[k].hour * 15;
        char* end = NULL;
        const long hour = strtol(time, &end, 10);
        if (bluewake_warp_time < 0 && end != time && *end == '\0' && hour >= 0 && hour < 24)
            bluewake_warp_time = (int)hour * 15;
        if (bluewake_warp_time < 0)
            fprintf(stderr,
                    "[warp] cannot read BLUEWAKE_WARP_TIME=%s: expected 0-23, morning, noon, evening or night\n",
                    time);
    }
    const char* stop_after = getenv("BLUEWAKE_WARP_STOP_AFTER");
    if (bluewake_warp_set && stop_after != NULL && stop_after[0] != '\0')
        bluewake_warp_stop_after = (unsigned)strtoul(stop_after, NULL, 10);
    const char* file = getenv("BLUEWAKE_WARP_FILE");
    if (file != NULL && file[0] >= '1' && file[0] <= '3' && file[1] == '\0') {
        bluewake_warp_file = (unsigned)(file[0] - '0');
        bluewake_warp_file_pending = true;
        fprintf(stderr, "[warp] pressing A until quest log %u starts\n", bluewake_warp_file);
    } else if (file != NULL && file[0] != '\0') {
        fprintf(stderr, "[warp] cannot read BLUEWAKE_WARP_FILE=%s: expected 1, 2 or 3\n", file);
    }
    if (bluewake_stage_select_armed && !bluewake_warp_set)
        fprintf(stderr, "[stage-select] hold R while starting a file to open the developers' stage select\n");
    bluewake_stage_select_live = bluewake_stage_select_armed || warp_asked || bluewake_warp_file_pending;
}

// Controller 1's buttons while BLUEWAKE_WARP_FILE presses for the player: A
// for 2 retraces in every 20 and Start 10 retraces after it, until the file
// starts. A finished game's log opens the name entry for New Game+, where
// Start moves to End and the next A confirms it; elsewhere Start does what A
// does. Input from a person ends it with the host's other scripted presses.
static inline u16 bluewake_stage_select_pad_buttons(u64 retrace, u16 buttons) {
    if (__builtin_expect(!bluewake_warp_file_pending, 1))
        return buttons;
    const u64 phase = retrace % 20u;
    if (phase < 2u)
        return (u16)(buttons | BLUEWAKE_STAGE_SELECT_PAD_A);
    if (phase >= 10u && phase < 12u)
        return (u16)(buttons | BLUEWAKE_STAGE_SELECT_PAD_START);
    return buttons;
}

// F7 (stage_select_key.h). In play it opens the stage select: the play
// scene's execute becomes fopScnM_ChangeReq(scene, MENU), as changeGameScene
// would ask, after the place play started from (the current stage's name,
// point, room and layer), the time and the day are kept. In the menu it goes
// back there: the press becomes the menu's own Start (its trigger bit), whose
// change to the play scene gets the kept place, and the kept time and day go
// back when the menu is deleted (its execute sets them every frame). Play
// resumes at the point the room was entered by, not where Link stood.
#define BLUEWAKE_STAGE_SELECT_PLAY_EXECUTE 0x80234FD0u // dScnPly_Execute
#define BLUEWAKE_STAGE_SELECT_LINK 0x803CA754u         // play.mpPlayerPtr[0]
#define BLUEWAKE_STAGE_SELECT_MENU_DELETE 0x8022F320u  // dScnMenu_Delete
#define BLUEWAKE_STAGE_SELECT_MENU_RETURN 0x8022F078u  // after dScnMenu_Execute's change to the play scene
#define BLUEWAKE_STAGE_SELECT_CUR_STAGE 0x803C9D3Cu    // play.mCurStage, laid out as mNextStage's first 12 bytes
#define BLUEWAKE_STAGE_SELECT_SAVE_DATE 0x803C4C30u    // dSv_player_status_b_c::mDate
#define BLUEWAKE_STAGE_SELECT_PAD1_TRIG 0x803A4E22u    // g_mDoCPd_cpadInfo[0].mButtonTrig
#define BLUEWAKE_STAGE_SELECT_GAME_START 0x0010u       // Start in the game's own bit order
#define BLUEWAKE_STAGE_SELECT_GAME_Z 0x0800u           // Z
#define BLUEWAKE_STAGE_SELECT_GAME_A 0x0100u           // A
#define BLUEWAKE_STAGE_SELECT_PAD3_TRIG 0x803A4E9Au    // g_mDoCPd_cpadInfo[2].mButtonTrig
#define BLUEWAKE_STAGE_SELECT_DEMO23 0x803F72A6u       // the menu's l_demo23
#define BLUEWAKE_STAGE_SELECT_FLAG_2D 0x803C5259u      // event flags (save + 0x624) byte 0x2D: bit 0x01 is 0x2D01

static atomic_uint bluewake_stage_select_key_presses;
static bool bluewake_stage_select_in_game; // a file has started
static bool bluewake_stage_select_kept;
static u8 bluewake_stage_select_kept_place[12];
static u32 bluewake_stage_select_kept_time;
static u16 bluewake_stage_select_kept_date;
static bool bluewake_stage_select_going_back;
static bool bluewake_stage_select_restore_time;

void bluewake_stage_select_hotkey(void) {
    if (bluewake_stage_select_armed)
        atomic_fetch_add_explicit(&bluewake_stage_select_key_presses, 1u, memory_order_relaxed);
}

static inline bool bluewake_stage_select_key_pending(void) {
    return atomic_load_explicit(&bluewake_stage_select_key_presses, memory_order_relaxed) != 0u;
}

static inline void bluewake_stage_select_key_take(void) {
    atomic_store_explicit(&bluewake_stage_select_key_presses, 0u, memory_order_relaxed);
}

static __attribute__((noinline)) bool bluewake_stage_select_observes_body(u32 address) {
    return ((bluewake_stage_select_armed || bluewake_warp_set || bluewake_warp_file_pending) &&
            address == BLUEWAKE_STAGE_SELECT_CHANGE) ||
           (bluewake_stage_select_armed &&
            (address == BLUEWAKE_STAGE_SELECT_EXECUTE || address == BLUEWAKE_STAGE_SELECT_MENU_DELETE ||
             (address == BLUEWAKE_STAGE_SELECT_PLAY_EXECUTE && bluewake_stage_select_key_pending()))) ||
           (bluewake_warp_file_pending && address == BLUEWAKE_STAGE_SELECT_DATA_SELECT) ||
           (bluewake_warp_applied && address == BLUEWAKE_STAGE_SELECT_PLAY_EXECUTE &&
            bluewake_warp_frames < 1u + bluewake_warp_stop_after);
}

static inline bool bluewake_stage_select_ram(u32 address, u32 size) {
    return address >= 0x80000000u && address + size <= 0x81800000u && address + size > address;
}

static inline void bluewake_stage_select_write_name(CPUState* cpu, u32 address, const char* text) {
    u32 i = 0;
    for (; text[i] != '\0' && i < 31u; ++i)
        mem_write8(cpu, address + i, (u8)text[i]);
    for (; i < 32u; ++i)
        mem_write8(cpu, address + i, 0u);
}

// menu_of_scene_class (d_s_menu.h): info at 0x1D4. menu_inf: the count (u8)
// at 0, the groups at 4. stage_inf (0x28): name[0x20], the room count at
// 0x21, the rooms at 0x24. room_inf (0x2C): name[0x20].
static inline void bluewake_stage_select_name_lines(CPUState* cpu) {
    const u32 scene = cpu->gpr[3];
    if (!bluewake_stage_select_ram(scene, 0x1D8u))
        return;
    const u32 info = mem_read32(cpu, scene + 0x1D4u);
    if (info == bluewake_stage_select_named_list || !bluewake_stage_select_ram(info, 8u))
        return;
    const u32 groups = mem_read8(cpu, info);
    const u32 stages = mem_read32(cpu, info + 4u);
    if (!bluewake_stage_select_ram(stages, groups * 0x28u))
        return;
    // Groups first, so each room's name can be fitted to the space its
    // group's name leaves: a line shows about 44 letters of "group <room>".
    unsigned written = 0;
    for (int pass = 0; pass < 2; ++pass) {
        for (unsigned k = 0; k < bluewake_stage_select_name_count; ++k) {
            const BluewakeStageSelectName* name = &bluewake_stage_select_names[k];
            if (name->group >= groups || (name->room >= 0) != (pass == 1))
                continue;
            const u32 stage = stages + name->group * 0x28u;
            u32 target = stage;
            int width = 31;
            if (name->room >= 0) {
                const u32 rooms = mem_read32(cpu, stage + 0x24u);
                if ((u32)name->room >= mem_read8(cpu, stage + 0x21u) ||
                    !bluewake_stage_select_ram(rooms + (u32)name->room * 0x2Cu, 0x2Cu))
                    continue;
                target = rooms + (u32)name->room * 0x2Cu;
                int group_length = 0;
                while (group_length < 0x20 && mem_read8(cpu, stage + (u32)group_length) != 0u)
                    group_length++;
                width = 44 - group_length;
                width = width < 12 ? 12 : width > 31 ? 31 : width;
            }
            // "name (note)": the name is cut short so the note always shows.
            const int note = name->note[0] != '\0' ? (int)strlen(name->note) + 3 : 0;
            int keep = (int)strlen(name->text);
            if (keep > width - note)
                keep = width - note > 0 ? width - note : 0;
            while (keep > 0 && name->text[keep - 1] == ' ')
                keep--;
            char text[32];
            if (note != 0)
                snprintf(text, sizeof text, "%.*s (%s)", keep, name->text, name->note);
            else
                snprintf(text, sizeof text, "%.*s", keep, name->text);
            bluewake_stage_select_write_name(cpu, target, text);
            written++;
        }
    }
    bluewake_stage_select_named_list = info;
    fprintf(stderr, "[stage-select] %u lines named\n", written);
}

// The menu's fixed lines (dScnMenu_Draw's JUTReport strings, Shift-JIS in
// main.dol's data) in English, each written over the original, which is
// never shorter. The first two bytes are checked before writing, so another
// build of the game is left alone. Their places are fixed and the debug font
// is wide (about 12 units a letter), so the text is fitted to them. The
// instructions go where the build line ("NDEBUG <date> FINAL"), the controls
// labels and the language were; X/Y change the time and the D-pad the day
// (the developers' "forward/back" labels were printed by the other setting).
typedef struct BluewakeStageSelectText {
    u32 address;
    u16 size; // bytes before the original's terminator
    u16 first; // the original's first two bytes
    const char* text;
} BluewakeStageSelectText;
static const BluewakeStageSelectText bluewake_stage_select_texts[] = {
    {0x80362D90u, 12u, 0x4E44u, "%sA/B:room"},  // "NDEBUG %s %s", the top line
    {0x80371598u, 17u, 0x3033u, "Up/Dn:line "},  // mDoMain::COPYDATE_STRING, its first argument (set once at boot)
    {0x80362CC2u, 37u, 0x454Eu, "D-pad L/R:day  R/L:spawn"}, // "ENGLISH" (through ITALIAN, this menu's own)
    {0x80362CE8u, 8u, 0x8381u, "Spawn"},
    {0x80362CF6u, 17u, 0x2563u, "%c %2d %s <%s>"},
    {0x80362D08u, 18u, 0x8277u, ""},
    {0x80362D2Fu, 26u, 0x8F5Cu, "Start:go X/Y:time"},
    {0x80362D4Au, 8u, 0x976Au, "Day: %s"},
    {0x80362D1Bu, 10u, 0x8E9Eu, "%d%s"},
    {0x80362D26u, 8u, 0x8E9Eu, "%s"},
    {0x80362C30u, 4u, 0x92CAu, "Norm"},
    {0x80362C35u, 8u, 0x8D82u, "Fast"},
    {0x80362C3Eu, 16u, 0x92A9u, "Morning"},
    {0x80362C4Fu, 16u, 0x928Bu, "Noon"},
    {0x80362C60u, 22u, 0x975Bu, "Evening"},
    {0x80362C77u, 16u, 0x96E9u, "Night"},
    {0x80362C88u, 8u, 0x8E9Eu, ":00"},
    {0x80362C91u, 6u, 0x93FAu, "Sun"},
    {0x80362C98u, 6u, 0x8C8Eu, "Mon"},
    {0x80362C9Fu, 6u, 0x89CEu, "Tue"},
    {0x80362CA6u, 6u, 0x9085u, "Wed"},
    {0x80362CADu, 6u, 0x96D8u, "Thu"},
    {0x80362CB4u, 6u, 0x8BE0u, "Fri"},
    {0x80362CBBu, 6u, 0x9379u, "Sat"},
    {0x80362D56u, 27u, 0x8252u, "Z: after rescue"},
    {0x80362D72u, 29u, 0x8252u, "Z: before rescue"},
};
static bool bluewake_stage_select_texts_done;

static inline void bluewake_stage_select_translate(CPUState* cpu) {
    bluewake_stage_select_texts_done = true;
    unsigned written = 0;
    for (unsigned k = 0; k < sizeof bluewake_stage_select_texts / sizeof bluewake_stage_select_texts[0]; ++k) {
        const BluewakeStageSelectText* t = &bluewake_stage_select_texts[k];
        if (mem_read16(cpu, t->address) != t->first)
            continue;
        u32 i = 0;
        for (; t->text[i] != '\0' && i < t->size; ++i)
            mem_write8(cpu, t->address + i, (u8)t->text[i]);
        mem_write8(cpu, t->address + i, 0u);
        written++;
    }
    fprintf(stderr, "[stage-select] %u of the menu's fixed lines in English\n", written);
}

// R/L in the menu step the spawn override (startCode: 0 is the room's own
// point, n is point n-1) through every number, and a point the room does not
// have freezes the game as it loads (dStage_playerInit reads past the end of
// its list). With the names file's spawn ids, each frame before the menu reads
// its buttons: a step onto a missing point goes on in the same direction to
// the next one the room has (0 counts), and a line or room change that leaves
// the override on a missing point puts it back to 0.
#define BLUEWAKE_STAGE_SELECT_CURSOR 0x803F729Cu      // l_cursolID
#define BLUEWAKE_STAGE_SELECT_GROUP_POINT 0x803F72A8u // l_groupPoint (s8*, each line's room)
#define BLUEWAKE_STAGE_SELECT_START_CODE 0x1E0u       // menu_of_scene_class::startCode

static inline bool bluewake_stage_select_spawn_ok(const BluewakeStageSelectName* name, unsigned start_code) {
    if (start_code == 0u)
        return true;
    const unsigned id = start_code - 1u;
    return (name->spawns[id >> 3] & (1u << (id & 7u))) != 0u;
}

static inline void bluewake_stage_select_check_spawn(CPUState* cpu) {
    static u8 last_code;
    const u32 scene = cpu->gpr[3];
    const u32 cursor = mem_read32(cpu, BLUEWAKE_STAGE_SELECT_CURSOR);
    const u32 points = mem_read32(cpu, BLUEWAKE_STAGE_SELECT_GROUP_POINT);
    if (!bluewake_stage_select_ram(scene, 0x1E4u) || cursor > 255u || !bluewake_stage_select_ram(points + cursor, 1u))
        return;
    const s8 room = (s8)mem_read8(cpu, points + cursor);
    const BluewakeStageSelectName* name = NULL;
    for (unsigned k = 0; k < bluewake_stage_select_name_count && name == NULL; ++k)
        if (bluewake_stage_select_names[k].has_spawns && bluewake_stage_select_names[k].group == cursor &&
            bluewake_stage_select_names[k].room == room)
            name = &bluewake_stage_select_names[k];
    u8 code = mem_read8(cpu, scene + BLUEWAKE_STAGE_SELECT_START_CODE);
    if (name != NULL && !bluewake_stage_select_spawn_ok(name, code)) {
        if (code == last_code) {
            code = 0u; // another line or room
        } else {
            const int step = (u8)(code - last_code) < 128u ? 1 : -1;
            for (int k = 0; k < 256 && !bluewake_stage_select_spawn_ok(name, code); ++k)
                code = (u8)(code + step);
        }
        mem_write8(cpu, scene + BLUEWAKE_STAGE_SELECT_START_CODE, code);
    }
    last_code = code;
}

// The file select's quest log choice, while BLUEWAKE_WARP_FILE presses for the
// player: the cursor is set to that log before the A press reads it. An empty
// log would start a new game, so the presses stop there instead.
static inline void bluewake_stage_select_pick_file(CPUState* cpu) {
    const u32 select = cpu->gpr[3];
    if (!bluewake_stage_select_ram(select, 0x3930u))
        return;
    const u32 index = bluewake_warp_file - 1u;
    if (mem_read8(cpu, select + 0x3914u + index) != 0u) { // dataNew
        bluewake_warp_file_pending = false;
        fprintf(stderr, "[warp] quest log %u is empty; stopped pressing A\n", bluewake_warp_file);
        return;
    }
    if (mem_read8(cpu, select + BLUEWAKE_STAGE_SELECT_SELECT_NUM) != index)
        mem_write8(cpu, select + BLUEWAKE_STAGE_SELECT_SELECT_NUM, (u8)index);
}

// changeGameScene has set the next stage from the save (dComIfGs_gameStart);
// the place goes over it before the play scene is asked for.
static inline void bluewake_stage_select_warp(CPUState* cpu) {
    for (u32 i = 0; i < 8u; ++i)
        mem_write8(cpu, BLUEWAKE_STAGE_SELECT_NEXT_STAGE + i, (u8)bluewake_warp_stage[i]);
    mem_write16(cpu, BLUEWAKE_STAGE_SELECT_NEXT_STAGE + 8u, (u16)bluewake_warp_point);
    mem_write8(cpu, BLUEWAKE_STAGE_SELECT_NEXT_STAGE + 0xAu, (u8)bluewake_warp_room);
    mem_write8(cpu, BLUEWAKE_STAGE_SELECT_NEXT_STAGE + 0xBu, (u8)bluewake_warp_layer);
    if (bluewake_warp_time >= 0) {
        const float degrees = (float)bluewake_warp_time;
        u32 bits;
        memcpy(&bits, &degrees, sizeof bits);
        mem_write32(cpu, BLUEWAKE_STAGE_SELECT_SAVE_TIME, bits);
    }
    bluewake_warp_applied = true;
    cpu->gpr[4] = BLUEWAKE_STAGE_SELECT_PLAY; // a new game skips its opening too
    static bool told;
    if (!told)
        fprintf(stderr, "[warp] going to %.8s room %d point %d layer %d with life %u of %u\n",
                bluewake_warp_stage, bluewake_warp_room, bluewake_warp_point, bluewake_warp_layer,
                mem_read16(cpu, 0x803C4C0Au), mem_read16(cpu, 0x803C4C08u));
    told = true;
}

// The play scene's execute, with F7 pressed: the tail call to the scene change
// (r3, the scene, stays; lr goes back to execute's caller). True: pc changed.
static __attribute__((noinline)) bool bluewake_stage_select_redirect_body(CPUState* cpu, u32 address) {
    if (__builtin_expect(address != BLUEWAKE_STAGE_SELECT_PLAY_EXECUTE || !bluewake_stage_select_key_pending(), 1))
        return false;
    bluewake_stage_select_key_take();
    if (!bluewake_stage_select_in_game || !bluewake_stage_select_ram(cpu->gpr[3], 4u))
        return false; // the title screen runs on the play scene's code too
    for (u32 i = 0; i < 12u; ++i)
        bluewake_stage_select_kept_place[i] = mem_read8(cpu, BLUEWAKE_STAGE_SELECT_CUR_STAGE + i);
    bluewake_stage_select_kept_time = mem_read32(cpu, BLUEWAKE_STAGE_SELECT_SAVE_TIME);
    bluewake_stage_select_kept_date = mem_read16(cpu, BLUEWAKE_STAGE_SELECT_SAVE_DATE);
    bluewake_stage_select_kept = true;
    cpu->gpr[4] = BLUEWAKE_STAGE_SELECT_MENU;
    cpu->gpr[5] = 0u; // fpcNm_OVERLAP0_e
    cpu->gpr[6] = 5u;
    cpu->pc = BLUEWAKE_STAGE_SELECT_CHANGE;
    fprintf(stderr, "[stage-select] F7: to the stage select from %.8s room %d point %d\n",
            (const char*)bluewake_stage_select_kept_place, (s8)bluewake_stage_select_kept_place[10],
            (s16)((bluewake_stage_select_kept_place[8] << 8) | bluewake_stage_select_kept_place[9]));
    return true;
}

// The menu's execute: F7 presses its Start, for the way back.
// Controller 1's Z, which the menu does not read, presses controller 3's A
// for it: the menu's own "demo 23" switch, story flag 0x2D01 (rescue.stb,
// M2tower). It picks Link's cutscene animations, /res/Object/LkD00.arc
// before Aryll's rescue and LkD01.arc after (d_s_play.cpp, phase_0); a
// cutscene from one half started with the other half's set overflows Link's
// face-animation heap and crashes.
static inline void bluewake_stage_select_demo_switch(CPUState* cpu) {
    if ((mem_read16(cpu, BLUEWAKE_STAGE_SELECT_PAD1_TRIG) & BLUEWAKE_STAGE_SELECT_GAME_Z) == 0u)
        return;
    // The menu flips its own l_demo23 (starting at 0), not the flag, so a
    // file with the flag on would need two presses: match it to the flag first.
    const bool on = (mem_read8(cpu, BLUEWAKE_STAGE_SELECT_FLAG_2D) & 0x01u) != 0u;
    mem_write16(cpu, BLUEWAKE_STAGE_SELECT_DEMO23, on ? 1u : 0u);
    mem_write16(cpu, BLUEWAKE_STAGE_SELECT_PAD3_TRIG,
                (u16)(mem_read16(cpu, BLUEWAKE_STAGE_SELECT_PAD3_TRIG) | BLUEWAKE_STAGE_SELECT_GAME_A));
}

static inline void bluewake_stage_select_key_in_menu(CPUState* cpu) {
    if (!bluewake_stage_select_key_pending())
        return;
    bluewake_stage_select_key_take();
    if (!bluewake_stage_select_kept)
        return;
    bluewake_stage_select_going_back = true;
    mem_write16(cpu, BLUEWAKE_STAGE_SELECT_PAD1_TRIG,
                (u16)(mem_read16(cpu, BLUEWAKE_STAGE_SELECT_PAD1_TRIG) | BLUEWAKE_STAGE_SELECT_GAME_START));
}

// The menu's change to the play scene, on the way back: the kept place over
// the room it set.
static inline void bluewake_stage_select_go_back(CPUState* cpu) {
    bluewake_stage_select_going_back = false;
    for (u32 i = 0; i < 12u; ++i)
        mem_write8(cpu, BLUEWAKE_STAGE_SELECT_NEXT_STAGE + i, bluewake_stage_select_kept_place[i]);
    bluewake_stage_select_restore_time = true;
    fprintf(stderr, "[stage-select] F7: back to %.8s room %d\n", (const char*)bluewake_stage_select_kept_place,
            (s8)bluewake_stage_select_kept_place[10]);
}

static __attribute__((noinline)) void bluewake_stage_select_dispatch_body(CPUState* cpu, u32 address) {
    if (__builtin_expect(!bluewake_stage_select_observes_body(address), 1))
        return;
    if (address == BLUEWAKE_STAGE_SELECT_EXECUTE) {
        if (!bluewake_stage_select_texts_done)
            bluewake_stage_select_translate(cpu);
        if (bluewake_stage_select_name_count != 0u) {
            bluewake_stage_select_name_lines(cpu);
            bluewake_stage_select_check_spawn(cpu);
        }
        bluewake_stage_select_demo_switch(cpu);
        bluewake_stage_select_key_in_menu(cpu);
        return;
    }
    if (address == BLUEWAKE_STAGE_SELECT_MENU_DELETE) {
        // The next menu loads its list again, often at the same address.
        bluewake_stage_select_named_list = 0u;
        if (bluewake_stage_select_restore_time) {
            mem_write32(cpu, BLUEWAKE_STAGE_SELECT_SAVE_TIME, bluewake_stage_select_kept_time);
            mem_write16(cpu, BLUEWAKE_STAGE_SELECT_SAVE_DATE, bluewake_stage_select_kept_date);
            bluewake_stage_select_restore_time = false;
        }
        return;
    }
    if (address == BLUEWAKE_STAGE_SELECT_DATA_SELECT) {
        bluewake_stage_select_pick_file(cpu);
        return;
    }
    if (address == BLUEWAKE_STAGE_SELECT_PLAY_EXECUTE) {
        // Arrived: the play scene runs with Link in it (a room can run its
        // scene without ever making him, as ITest62 does).
        if (bluewake_warp_applied && mem_read32(cpu, BLUEWAKE_STAGE_SELECT_LINK) >= 0x80000000u &&
            ++bluewake_warp_frames == 1u)
            fprintf(stderr, "[warp] arrived: Link is in the room\n");
        if (bluewake_warp_stop_after != 0u && bluewake_warp_frames >= bluewake_warp_stop_after &&
            !bluewake_stage_select_stop) {
            bluewake_stage_select_stop = true;
            fprintf(stderr, "[warp] stopping %u play frames after arriving\n", bluewake_warp_frames);
        }
        return;
    }
    if (address != BLUEWAKE_STAGE_SELECT_CHANGE)
        return;
    if (cpu->lr == BLUEWAKE_STAGE_SELECT_MENU_RETURN && cpu->gpr[4] == BLUEWAKE_STAGE_SELECT_PLAY) {
        if (bluewake_stage_select_going_back)
            bluewake_stage_select_go_back(cpu);
        return;
    }
    if (cpu->lr != BLUEWAKE_STAGE_SELECT_NAME_RETURN ||
        (cpu->gpr[4] != BLUEWAKE_STAGE_SELECT_PLAY && cpu->gpr[4] != BLUEWAKE_STAGE_SELECT_OPEN))
        return;
    bluewake_warp_file_pending = false;
    bluewake_stage_select_in_game = true;
    if (bluewake_warp_set) {
        bluewake_stage_select_warp(cpu);
        return;
    }
    if (!bluewake_stage_select_armed ||
        (mem_read16(cpu, BLUEWAKE_STAGE_SELECT_PAD1_HOLD) & BLUEWAKE_STAGE_SELECT_PAD_R) == 0u)
        return;
    cpu->gpr[4] = BLUEWAKE_STAGE_SELECT_MENU;
    static bool told;
    if (!told)
        fprintf(stderr, "[stage-select] R held: the file's start goes to the stage select\n");
    told = true;
}


// The per-block entry points: only the flag test is inline in the hot path; the
// bodies stay out of line so they don't add to host_chassis_edge_service's size.
static inline bool bluewake_stage_select_observes(u32 address) {
    return __builtin_expect(bluewake_stage_select_live, 0) && bluewake_stage_select_observes_body(address);
}
static inline bool bluewake_stage_select_redirect(CPUState* cpu, u32 address) {
    return __builtin_expect(bluewake_stage_select_live, 0) && bluewake_stage_select_redirect_body(cpu, address);
}
static inline void bluewake_stage_select_dispatch(CPUState* cpu, u32 address) {
    if (__builtin_expect(bluewake_stage_select_live, 0))
        bluewake_stage_select_dispatch_body(cpu, address);
}

#endif
