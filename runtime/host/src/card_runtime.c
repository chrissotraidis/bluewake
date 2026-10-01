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
static u64 g_card_write_calls;
static u64 g_card_write_bytes;

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
    if (g_card_suspended && bluewake_card_runtime_intercepts(cpu->pc)) {
        cpu->gpr[3] = (u32)DOL_CARD_RESULT_BUSY;
        cpu->pc = cpu->lr & ~3u;
        handled = true;
    } else {
        handled = dispatch_card(cpu);
    }
    pthread_mutex_unlock(&g_card_mutex);
    return handled;
}
