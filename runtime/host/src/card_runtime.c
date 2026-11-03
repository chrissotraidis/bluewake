#include "card_runtime.h"

#include "gxruntime/hle.h"
#include "gxruntime/hle_abi.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>

#define BLUEWAKE_CARD_PATH_CAPACITY 4096u

typedef void (*CardHandler)(CPUState* cpu);

static char g_card_path[BLUEWAKE_CARD_PATH_CAPACITY];
static bool g_card_open;
static bool g_card_suspended;
static pthread_mutex_t g_card_mutex = PTHREAD_MUTEX_INITIALIZER;

void bluewake_card_runtime_suspend_writes(bool suspend) {
    // Wait for an in-flight write before returning to the restore/import UI.
    pthread_mutex_lock(&g_card_mutex);
    g_card_suspended = suspend;
    pthread_mutex_unlock(&g_card_mutex);
}
static bool g_card_writes_refused;
static const char* g_card_refused_why;
static u64 g_card_write_calls;
static u64 g_card_write_bytes;

static void say_saving_is_off(CPUState* cpu, bool search);

// BLUEWAKE_STAGE_SELECT_SAVES=1 (--stage-select-saves): the player has chosen
// to keep saving whatever the stage select does to their file.
bool bluewake_card_runtime_saving_kept(void) {
    static int kept = -1;
    if (kept < 0) {
        const char* value = getenv("BLUEWAKE_STAGE_SELECT_SAVES");
        kept = value != NULL && value[0] == '1';
    }
    return kept != 0;
}

// With g_card_mutex held.
static void refuse_writes_locked(CPUState* cpu, const char* why) {
    if (bluewake_card_runtime_saving_kept()) {
        static bool said;
        if (!said)
            fprintf(stderr, "[card] saving stays on (BLUEWAKE_STAGE_SELECT_SAVES): %s\n", why);
        said = true;
        return;
    }
    if (!g_card_writes_refused) {
        g_card_writes_refused = true;
        g_card_refused_why = why;
        fprintf(stderr, "[card] saving is off until BlueWake restarts: %s\n", why);
    }
    if (cpu != NULL)
        say_saving_is_off(cpu, true);
}

void bluewake_card_runtime_refuse_writes(CPUState* cpu, const char* why) {
    pthread_mutex_lock(&g_card_mutex);
    refuse_writes_locked(cpu, why);
    pthread_mutex_unlock(&g_card_mutex);
}

bool bluewake_card_runtime_writes_refused(void) { return g_card_writes_refused; }

u64 bluewake_card_runtime_write_calls(void) { return g_card_write_calls; }
u64 bluewake_card_runtime_write_bytes(void) { return g_card_write_bytes; }

static bool ensure_parent_directories(const char* path) {
    char parent[BLUEWAKE_CARD_PATH_CAPACITY];
    const size_t length = strlen(path);
    if (length == 0u || length >= sizeof parent)
        return false;
    memcpy(parent, path, length + 1u);

    for (char* cursor = parent + 1; *cursor != '\0'; cursor++) {
        if (*cursor != '/')
            continue;
        *cursor = '\0';
        if (mkdir(parent, 0755) != 0 && errno != EEXIST)
            return false;
        *cursor = '/';
    }
    return true;
}

static bool resolve_card_path(const char* explicit_path) {
    if (explicit_path != NULL && explicit_path[0] != '\0') {
        if (snprintf(g_card_path, sizeof g_card_path, "%s", explicit_path) >=
            (int)sizeof g_card_path)
            return false;
        return true;
    }

    const char* home = getenv("HOME");
    if (home == NULL || home[0] == '\0')
        return false;
    return snprintf(g_card_path, sizeof g_card_path,
                    "%s/Library/Application Support/BlueWake/GZLE01.card",
                    home) < (int)sizeof g_card_path;
}

bool bluewake_card_runtime_open(const char* explicit_path) {
    bluewake_card_runtime_close();
    if (!resolve_card_path(explicit_path)) {
        fprintf(stderr, "[card] unable to resolve slot-A storage path\n");
        return false;
    }
    if (!ensure_parent_directories(g_card_path)) {
        fprintf(stderr, "[card] unable to create parent directory for %s\n",
                g_card_path);
        g_card_path[0] = '\0';
        return false;
    }

    DolHleConfig config;
    memset(&config, 0, sizeof config);
    memcpy(config.game_code, "GZLE", sizeof config.game_code);
    memcpy(config.company, "01", sizeof config.company);
    dol_hle_init(&config);
    if (getenv("BLUEWAKE_CARD_LOG") != NULL)
        g_card_log = true;
    if (!dol_hle_card_open(g_card_path)) {
        fprintf(stderr, "[card] unable to open slot-A storage at %s\n",
                g_card_path);
        g_card_path[0] = '\0';
        return false;
    }
    g_card_open = true;
    return true;
}

void bluewake_card_runtime_close(void) {
    pthread_mutex_lock(&g_card_mutex);
    if (g_card_open)
        dol_hle_card_close();
    g_card_open = false;
    g_card_suspended = false;
    pthread_mutex_unlock(&g_card_mutex);
}

const char* bluewake_card_runtime_path(void) {
    return g_card_open ? g_card_path : NULL;
}

void bluewake_card_runtime_service_callback(CPUState* cpu) {
    if (dol_hle_handle_callback_return(cpu, cpu->pc))
        return;
    dol_hle_poll_callback(cpu);
}

static CardHandler card_handler(u32 address) {
    switch (address) {
    case 0x8031A6E8u:
        return dol_hle_CARDFreeBlocks;
    case 0x8031CD50u:
        return dol_hle_CARDCheckExAsync;
    case 0x8031D400u:
        return dol_hle_CARDProbe;
    case 0x8031D438u:
        return dol_hle_CARDProbeEx;
    case 0x8031DAFCu:
        return dol_hle_CARDMountAsync;
    case 0x8031DD80u:
        return dol_hle_CARDUnmount;
    case 0x8031E74Cu:
        return dol_hle_CARDOpen;
    case 0x8031E8C4u:
        return dol_hle_CARDClose;
    case 0x8031EA48u:
        return dol_hle_CARDCreateAsync;
    case 0x8031EF98u:
        return dol_hle_CARDReadAsync;
    case 0x8031F348u:
        return dol_hle_CARDWriteAsync;
    case 0x8031F69Cu:
        return dol_hle_CARDGetStatus;
    case 0x8031F7C8u:
        return dol_hle_CARDSetStatusAsync;
    case 0x8031F984u:
        return dol_hle_CARDGetSerialNo;
    default:
        return NULL;
    }
}

bool bluewake_card_runtime_intercepts(u32 address) {
    switch (address) {
    case 0x8031D2E0u:
    case 0x8031DC9Cu:
    case 0x8031E5C8u:
    case 0x8031EC68u:
    case 0x8031F0E0u:
    case 0x8031F45Cu:
    case 0x8031F93Cu:
        return true;
    default:
        return card_handler(address) != NULL;
    }
}

// With saving off, the card answers as one that has been taken out: the calls
// that look for it (probe, check, mount) and the ones that change it (format,
// create, write, set status; each synchronous wrapper and asynchronous form).
// Tried on the pause menu's save screen (2026-10-11): refusing only the writes
// made it retry by itself every few seconds (with "no card") or call the card
// damaged (with an I/O error); with no card to find it says "There is no
// Memory Card in Slot A" and waits for the player, as with a card pulled out.
static const char* card_call_refused(u32 address) {
    switch (address) {
    case 0x8031D400u:
    case 0x8031D438u:
        return "CARDProbe";
    case 0x8031CD50u:
    case 0x8031D2E0u:
        return "CARDCheck";
    case 0x8031DAFCu:
    case 0x8031DC9Cu:
        return "CARDMount";
    case 0x8031E5C8u:
        return "CARDFormat";
    case 0x8031EA48u:
    case 0x8031EC68u:
        return "CARDCreate";
    case 0x8031F348u:
    case 0x8031F45Cu:
        return "CARDWrite";
    case 0x8031F7C8u:
    case 0x8031F93Cu:
        return "CARDSetStatus";
    default:
        return NULL;
    }
}

// The calls that mount or check the card, which the game makes before it saves.
static bool card_call_mounts(u32 address) {
    switch (address) {
    case 0x8031CD50u:
    case 0x8031D2E0u:
    case 0x8031DAFCu:
    case 0x8031DC9Cu:
        return true;
    default:
        return false;
    }
}

// The calls that look for the card.
static bool card_call_looks(u32 address) {
    switch (address) {
    case 0x8031D400u:
    case 0x8031D438u:
    case 0x8031CD50u:
    case 0x8031D2E0u:
    case 0x8031DAFCu:
    case 0x8031DC9Cu:
        return true;
    default:
        return false;
    }
}

// The game's two "no Memory Card" messages (the English message table, loaded
// in MEM1), said as what they mean while saving is off: written over in place
// when saving goes off, no longer than they were and padded with zeros, and
// again whenever the game looks for the card (a reset loads the table anew).
// `seen_at` is where every run so far had the table; the rest of MEM1 is
// searched when it isn't there.
static const struct {
    const char* text;
    const char* instead;
    u32 seen_at;
} k_no_card_texts[] = {
    {"The game cannot be saved.\n\nThere is no Memory Card in Slot A.\nPlease check to be sure that a Memory\n"
     "Card has been inserted properly.",
     "The game cannot be saved.\n\nSaving is off because the stage\nselect was used. Restart BlueWake\n"
     "to save again.",
     0x80E0ECCAu},
    {"There is no Memory Card in Slot A.\n\nPlease check to confirm that a Memory\nCard is inserted properly.",
     "Saving is off because the stage\nselect was used. Restart BlueWake\nto save again.",
     0x80E0E63Cu},
};

// The game's text, ending where its message does, in MEM1: NULL when it isn't
// there. Without `search`, only where it was seen: a search of all of MEM1 takes
// milliseconds, too long for the game's frequent probes.
static u8* find_game_text(CPUState* cpu, const char* text, u32 seen_at, bool search) {
    const size_t length = strlen(text);
    if (cpu->ram == NULL || length + 1u > cpu->ram_size)
        return NULL;
    const u32 hint = seen_at - 0x80000000u;
    if (hint < cpu->ram_size && length < cpu->ram_size - hint && memcmp(cpu->ram + hint, text, length) == 0 &&
        cpu->ram[hint + length] == 0u)
        return cpu->ram + hint;
    if (!search)
        return NULL;
    u8* const end = cpu->ram + cpu->ram_size - length - 1u;
    for (u8* at = cpu->ram; at < end && (at = memchr(at, text[0], (size_t)(end - at))) != NULL; ++at)
        if (memcmp(at, text, length) == 0 && at[length] == 0u)
            return at;
    return NULL;
}

// A save state is the game's memory, so the messages said this way ride along
// in every state taken after saving went off: finding them means the game in
// memory came from such a session, whatever this one has done.
static bool saving_was_off(CPUState* cpu) {
    for (size_t k = 0; k < sizeof k_no_card_texts / sizeof k_no_card_texts[0]; ++k)
        if (find_game_text(cpu, k_no_card_texts[k].instead, k_no_card_texts[k].seen_at, true) != NULL)
            return true;
    return false;
}

static void say_saving_is_off(CPUState* cpu, bool search) {
    for (size_t k = 0; k < sizeof k_no_card_texts / sizeof k_no_card_texts[0]; ++k) {
        u8* at = find_game_text(cpu, k_no_card_texts[k].text, k_no_card_texts[k].seen_at, search);
        if (at == NULL)
            continue;
        const size_t length = strlen(k_no_card_texts[k].text);
        memset(at, 0, length);
        memcpy(at, k_no_card_texts[k].instead, strlen(k_no_card_texts[k].instead));
        fprintf(stderr, "[card] the game's no-card message at 0x%08X now says saving is off\n",
                0x80000000u + (u32)(at - cpu->ram));
    }
}

static bool dispatch_card(CPUState* cpu) {
    static unsigned probe_reports;
    switch (cpu->pc) {
    case 0x8031D2E0u:
        cpu->gpr[4] = 0u;
        dol_hle_CARDCheckAsync(cpu);
        break;
    case 0x8031DC9Cu:
        cpu->gpr[6] = 0u;
        dol_hle_CARDMountAsync(cpu);
        break;
    case 0x8031E5C8u:
        cpu->gpr[4] = 0u;
        dol_hle_CARDFormatAsync(cpu);
        break;
    case 0x8031EC68u:
        cpu->gpr[7] = 0u;
        dol_hle_CARDCreateAsync(cpu);
        break;
    case 0x8031F0E0u:
        cpu->gpr[7] = 0u;
        dol_hle_CARDReadAsync(cpu);
        break;
    case 0x8031F45Cu:
        cpu->gpr[7] = 0u;
        // The guest's save data write arrives here (arg2 = length), so this is
        // the moment the game saves. Counted, not interpreted.
        g_card_write_calls++;
        g_card_write_bytes += (u32)cpu->gpr[6];
        dol_hle_CARDWriteAsync(cpu);
        break;
    case 0x8031F93Cu:
        cpu->gpr[6] = 0u;
        dol_hle_CARDSetStatusAsync(cpu);
        break;
    default:
        goto asynchronous_or_direct;
    }
    cpu->pc = cpu->lr & ~3u;
    return true;

asynchronous_or_direct:
    ;
    CardHandler handler = card_handler(cpu->pc);
    if (handler == NULL)
        return false;
    const u32 address = cpu->pc;
    const u32 arg1 = cpu->gpr[4];
    const u32 arg2 = cpu->gpr[5];
    if (handler == dol_hle_CARDWriteAsync) {
        g_card_write_calls++;
        g_card_write_bytes += (u32)cpu->gpr[6];
    }
    handler(cpu);
    if (g_card_log && address == 0x8031D438u && probe_reports < 8u) {
        fprintf(stderr,
                "[card-bridge] CARDProbeEx result=%d mem_ptr=0x%08X "
                "mem=%u sector_ptr=0x%08X sector=%u return=0x%08X\n",
                (s32)cpu->gpr[3], arg1,
                arg1 != 0u ? mem_read32(cpu, arg1) : 0u, arg2,
                arg2 != 0u ? mem_read32(cpu, arg2) : 0u, cpu->lr & ~3u);
        probe_reports++;
    }
    cpu->pc = cpu->lr & ~3u;
    return true;
}

bool bluewake_card_runtime_dispatch(CPUState* cpu) {
    if (!bluewake_card_runtime_intercepts(cpu->pc))
        return false;
    pthread_mutex_lock(&g_card_mutex);
    bool handled;
    // On a mount or check only, which the game does before it saves and seldom
    // otherwise (twice in a 26,000-retrace new game and save); it probes far
    // more often, and the search of MEM1 has no place there.
    if (!g_card_writes_refused && !bluewake_card_runtime_saving_kept() && card_call_mounts(cpu->pc) &&
        saving_was_off(cpu))
        refuse_writes_locked(cpu, "the game in memory is from a session that used the stage select");
    if (g_card_suspended && bluewake_card_runtime_intercepts(cpu->pc)) {
        cpu->gpr[3] = (u32)DOL_CARD_RESULT_BUSY;
        cpu->pc = cpu->lr & ~3u;
        handled = true;
    } else if (g_card_writes_refused && card_call_refused(cpu->pc) != NULL) {
        // Answered before it starts, so the asynchronous forms never call back.
        static unsigned refusals;
        if (card_call_looks(cpu->pc))
            say_saving_is_off(cpu, card_call_mounts(cpu->pc));
        if (refusals++ < 8u)
            fprintf(stderr, "[card] %s: no card, saving is off (%s)\n", card_call_refused(cpu->pc),
                    g_card_refused_why);
        // CARDProbe answers whether a card is there; the rest with a result.
        cpu->gpr[3] = cpu->pc == 0x8031D400u ? 0u : (u32)DOL_CARD_RESULT_NO_CARD;
        cpu->pc = cpu->lr & ~3u;
        handled = true;
    } else {
        handled = dispatch_card(cpu);
    }
    pthread_mutex_unlock(&g_card_mutex);
    return handled;
}
