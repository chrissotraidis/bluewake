// Move Wind Waker saves between Dolphin and BlueWake's card; see dolphin_save_import.h.
#include "dolphin_save_import.h"

#include <stdlib.h>
#include <string.h>

#define BLOCK 0x2000u
#define GCZELDA_LENGTH (12u * BLOCK)
#define QUEST 0x770u
#define QUEST_DATA 0x768u
#define NAME_OFFSET 0x157u // dSv_player_info_c::mPlayerName, 17 bytes
#define ENTRY 64u

#define CARD_HEADER 40u
#define CARD_RECORD 68u
#define CARD_MAX_FILES 127u
#define CARD_SYSTEM_BLOCKS 5u

// Offsets in a GameCube directory entry (.gci's 64-byte header, Dolphin's
// DEntry): 0x06 is reserved and always 0xff, and 0x07 is the image key, whose
// low two bits are the banner format. That key is what the container keeps as
// its banner_format, so it goes back into 0x07 rather than 0x06.
#define ENTRY_GAME 0x00u
#define ENTRY_RESERVED 0x06u
#define ENTRY_IMAGE_KEY 0x07u
#define ENTRY_NAME 0x08u
#define ENTRY_TIME 0x28u
#define ENTRY_IMAGE 0x2Cu
#define ENTRY_ICON_FORMAT 0x30u
#define ENTRY_ANIMATION 0x32u
#define ENTRY_PERMISSION 0x34u
#define ENTRY_COPY_COUNTER 0x35u
#define ENTRY_FIRST_BLOCK 0x36u
#define ENTRY_BLOCK_COUNT 0x38u
#define ENTRY_UNUSED 0x3Au
#define ENTRY_COMMENTS 0x3Cu

static uint32_t be16(const uint8_t* p) { return (uint32_t)p[0] << 8 | p[1]; }
static uint32_t be32(const uint8_t* p) { return be16(p) << 16 | be16(p + 2); }
static uint64_t be64(const uint8_t* p) { return (uint64_t)be32(p) << 32 | be32(p + 4); }
static void put16(uint8_t* p, uint32_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void put32(uint8_t* p, uint32_t v) { put16(p, v >> 16); put16(p + 2, v); }

static uint32_t fnv1a(const uint8_t* data, size_t size) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; i++)
        hash = (hash ^ data[i]) * 16777619u;
    return hash;
}

// Sum of the bytes in the high word, sum of their 32-bit complements in the low.
static uint64_t quest_checksum(const uint8_t* data) {
    uint32_t sum = 0, inverse = 0;
    for (size_t i = 0; i < QUEST_DATA; i++) {
        sum += data[i];
        inverse += ~(uint32_t)data[i];
    }
    return (uint64_t)sum << 32 | inverse;
}

// 16-bit word sums over the block, less its trailing checksum.
static uint32_t block_checksum(const uint8_t* block) {
    uint32_t sum = 0, inverse = 0;
    for (size_t i = 0; i < BLOCK - 4; i += 2) {
        uint32_t word = be16(block + i);
        sum = (sum + word) & 0xFFFFu;
        inverse = (inverse + (~word & 0xFFFFu)) & 0xFFFFu;
    }
    return sum << 16 | inverse;
}

static const uint8_t* quest_at(const uint8_t* gczelda, int copy, int slot) {
    return gczelda + BLOCK * (1u + (unsigned)copy) + 8u + (unsigned)slot * QUEST;
}

static bool quest_valid(const uint8_t* quest) {
    return be64(quest + QUEST_DATA) == quest_checksum(quest);
}

// Copy 1 unless only copy 2 of this slot is intact.
static const uint8_t* best_quest(const uint8_t* gczelda, int slot) {
    const uint8_t* first = quest_at(gczelda, 0, slot);
    const uint8_t* second = quest_at(gczelda, 1, slot);
    return !quest_valid(first) && quest_valid(second) ? second : first;
}

void bw_gczelda_quest_logs(const uint8_t* gczelda, BWQuestLog out[BW_QUEST_LOGS]) {
    for (int slot = 0; slot < BW_QUEST_LOGS; slot++) {
        const uint8_t* quest = best_quest(gczelda, slot);
        BWQuestLog* log = &out[slot];
        memset(log, 0, sizeof *log);
        log->checksum_ok = quest_valid(quest);
        log->max_life = be16(quest);
        log->rupees = be16(quest + 4);
        const uint8_t* name = quest + NAME_OFFSET;
        size_t start = 0, end = 0;
        while (end < 16 && name[end] != 0)
            end++;
        while (start < end && name[start] == ' ')
            start++;
        while (end > start && name[end - 1] == ' ')
            end--;
        for (size_t i = start; i < end; i++)
            log->name[i - start] = name[i] >= 0x20 && name[i] < 0x7F ? (char)name[i] : '?';
        log->empty = name[0] == 0;
    }
}

static bool entry_is_gczelda(const uint8_t* entry) {
    return memcmp(entry, "GZLE01", 6) == 0 && memcmp(entry + 8, "gczelda", 8) == 0;
}

static const char* take(const uint8_t* entry, const uint8_t* data, BWDolphinSave* out) {
    if (be16(entry + 0x38) != 12)
        return "The Wind Waker save in this file is not the expected size.";
    out->data = (uint8_t*)malloc(GCZELDA_LENGTH);
    if (out->data == NULL)
        return "Out of memory.";
    memcpy(out->entry, entry, ENTRY);
    memcpy(out->data, data, GCZELDA_LENGTH);
    out->length = GCZELDA_LENGTH;
    return NULL;
}

// A raw card: block 0 header, 1-2 directory and its backup, 3-4 block map and
// its backup, then files. The copy with the higher update counter is current.
static const char* parse_raw(const uint8_t* bytes, size_t size, BWDolphinSave* out) {
    size_t blocks = size / BLOCK;
    const uint8_t* dir = bytes + BLOCK;
    if ((int16_t)be16(bytes + 2 * BLOCK + 0x1FFA) > (int16_t)be16(dir + 0x1FFA))
        dir = bytes + 2 * BLOCK;
    const uint8_t* bat = bytes + 3 * BLOCK;
    if ((int16_t)be16(bytes + 4 * BLOCK + 4) > (int16_t)be16(bat + 4))
        bat = bytes + 4 * BLOCK;
    for (unsigned i = 0; i < CARD_MAX_FILES; i++) {
        const uint8_t* entry = dir + i * ENTRY;
        if (!entry_is_gczelda(entry))
            continue;
        if (be16(entry + 0x38) != 12)
            return "The Wind Waker save on this card is not the expected size.";
        uint8_t* data = (uint8_t*)malloc(GCZELDA_LENGTH);
        if (data == NULL)
            return "Out of memory.";
        unsigned block = be16(entry + 0x36);
        for (unsigned n = 0; n < 12; n++) {
            if (block < CARD_SYSTEM_BLOCKS || block >= blocks ||
                0x0Au + (block - CARD_SYSTEM_BLOCKS) * 2u + 2u > BLOCK) {
                free(data);
                return "The Wind Waker save on this card is damaged.";
            }
            memcpy(data + n * BLOCK, bytes + (size_t)block * BLOCK, BLOCK);
            block = be16(bat + 0x0A + (block - CARD_SYSTEM_BLOCKS) * 2);
        }
        const char* error = take(entry, data, out);
        free(data);
        return error;
    }
    return "This memory card has no Wind Waker (USA) save on it.";
}

const char* bw_dolphin_save_parse(const uint8_t* bytes, size_t size, BWDolphinSave* out) {
    memset(out, 0, sizeof *out);
    if (size == ENTRY + GCZELDA_LENGTH) {
        if (!entry_is_gczelda(bytes))
            return "This is not a Wind Waker (USA) save. BlueWake needs a GZLE01 gczelda file.";
        return take(bytes, bytes + ENTRY, out);
    }
    if (size >= 64 * BLOCK && size % BLOCK == 0)
        return parse_raw(bytes, size, out);
    return "This is not a Dolphin save (.gci) or memory card (.raw).";
}

void bw_dolphin_save_free(BWDolphinSave* save) {
    free(save->data);
    memset(save, 0, sizeof *save);
}

// Checks the container and finds the gczelda record (offset 0 if none), the
// lowest unused file number, and the blocks in use.
static const char* scan_card(const uint8_t* card, size_t size, size_t* record, unsigned* free_index,
                             unsigned* used_blocks) {
    static const char* unreadable = "BlueWake's memory card could not be read.";
    if (size < CARD_HEADER || memcmp(card, "DOLCARD1", 8) != 0 || be32(card + 8) != 1 ||
        be32(card + 16) != BLOCK || be32(card + 32) != size - CARD_HEADER ||
        be32(card + 36) != fnv1a(card + CARD_HEADER, size - CARD_HEADER))
        return unreadable;
    bool taken[CARD_MAX_FILES] = {false};
    size_t cursor = CARD_HEADER;
    *record = 0;
    *used_blocks = 0;
    for (uint32_t i = 0, count = be32(card + 28); i < count; i++) {
        if (size - cursor < CARD_RECORD)
            return unreadable;
        unsigned index = be16(card + cursor);
        uint32_t length = be32(card + cursor + 4);
        if (index >= CARD_MAX_FILES || length > size - cursor - CARD_RECORD)
            return unreadable;
        taken[index] = true;
        *used_blocks += (length + BLOCK - 1) / BLOCK;
        if (memcmp(card + cursor + 12, "gczelda", 8) == 0 && memcmp(card + cursor + 44, "GZLE01", 6) == 0) {
            if (length != GCZELDA_LENGTH)
                return "The Wind Waker save on BlueWake's card is not the expected size.";
            *record = cursor;
        }
        cursor += CARD_RECORD + length;
    }
    if (cursor != size)
        return unreadable;
    *free_index = 0;
    while (*free_index < CARD_MAX_FILES && taken[*free_index])
        (*free_index)++;
    return NULL;
}

const char* bw_card_quest_logs(const uint8_t* card, size_t size, bool* has_saves, BWQuestLog out[BW_QUEST_LOGS]) {
    size_t record;
    unsigned free_index, used;
    memset(out, 0, sizeof(BWQuestLog) * BW_QUEST_LOGS);
    *has_saves = false;
    const char* error = scan_card(card, size, &record, &free_index, &used);
    if (error != NULL)
        return error;
    if (record != 0) {
        *has_saves = true;
        bw_gczelda_quest_logs(card + record + CARD_RECORD, out);
    }
    return NULL;
}

const char* bw_card_import(const uint8_t* card, size_t size, const BWDolphinSave* save, int src_slot, int dst_slot,
                           uint8_t** out, size_t* out_size) {
    *out = NULL;
    *out_size = 0;
    size_t record;
    unsigned free_index, used;
    const char* error = scan_card(card, size, &record, &free_index, &used);
    if (error != NULL)
        return error;
    if (save->data == NULL || save->length != GCZELDA_LENGTH)
        return "The Dolphin save could not be read.";

    uint8_t* result;
    size_t result_size;
    if (record != 0) {
        if (src_slot < 1 || src_slot > BW_QUEST_LOGS || dst_slot < 1 || dst_slot > BW_QUEST_LOGS)
            return "Pick a quest log from 1 to 3.";
        const uint8_t* quest = best_quest(save->data, src_slot - 1);
        if (!quest_valid(quest))
            return "That quest log is damaged in the Dolphin save.";
        result_size = size;
        result = (uint8_t*)malloc(result_size);
        if (result == NULL)
            return "Out of memory.";
        memcpy(result, card, size);
        uint8_t* gczelda = result + record + CARD_RECORD;
        for (int copy = 0; copy < 2; copy++) {
            uint8_t* block = gczelda + BLOCK * (1u + (unsigned)copy);
            memcpy(block + 8u + (unsigned)(dst_slot - 1) * QUEST, quest, QUEST);
            put32(block + BLOCK - 4, block_checksum(block));
        }
        put32(result + record + 64, fnv1a(gczelda, GCZELDA_LENGTH));
    } else {
        // No saves yet: add the Dolphin file as a new record, keeping what its
        // directory entry says about it.
        unsigned size_mbits = be16(card + 12);
        if (free_index >= CARD_MAX_FILES || used + 12 > size_mbits * 16u - CARD_SYSTEM_BLOCKS)
            return "BlueWake's memory card is full.";
        result_size = size + CARD_RECORD + GCZELDA_LENGTH;
        result = (uint8_t*)calloc(1, result_size);
        if (result == NULL)
            return "Out of memory.";
        memcpy(result, card, size);
        const uint8_t* entry = save->entry;
        uint8_t* r = result + size;
        put16(r, free_index);
        put32(r + 4, GCZELDA_LENGTH);
        put32(r + 8, be32(entry + 0x28));  // modification time
        memcpy(r + 12, entry + 8, 32);     // file name
        memcpy(r + 44, entry, 6);          // game code and company
        r[50] = entry[0x07];               // banner format
        r[51] = entry[0x34];               // permission
        put32(r + 52, be32(entry + 0x2C)); // icon address
        put16(r + 56, be16(entry + 0x30)); // icon format
        put16(r + 58, be16(entry + 0x32)); // icon speed
        put32(r + 60, be32(entry + 0x3C)); // comment address
        put32(r + 64, fnv1a(save->data, GCZELDA_LENGTH));
        memcpy(r + CARD_RECORD, save->data, GCZELDA_LENGTH);
        put32(result + 28, be32(card + 28) + 1);
        put32(result + 32, (uint32_t)(result_size - CARD_HEADER));
    }
    put32(result + 36, fnv1a(result + CARD_HEADER, result_size - CARD_HEADER));
    *out = result;
    *out_size = result_size;
    return NULL;
}

const char* bw_card_export(const uint8_t* card, size_t size, uint8_t** out, size_t* out_size) {
    *out = NULL;
    *out_size = 0;
    size_t record;
    unsigned free_index, used;
    const char* error = scan_card(card, size, &record, &free_index, &used);
    if (error != NULL)
        return error;
    if (record == 0)
        return "BlueWake's memory card has no save to export yet.";
    const uint8_t* r = card + record;
    if (be32(r + 64) != fnv1a(r + CARD_RECORD, GCZELDA_LENGTH))
        return "The Wind Waker save on BlueWake's card is damaged.";
    size_t total = ENTRY + GCZELDA_LENGTH;
    uint8_t* gci = (uint8_t*)calloc(1, total);
    if (gci == NULL)
        return "Out of memory.";
    memcpy(gci + ENTRY_GAME, r + 44, 6); // game code and company
    gci[ENTRY_RESERVED] = 0xFFu;
    gci[ENTRY_IMAGE_KEY] = r[50]; // banner format and icon animation
    memcpy(gci + ENTRY_NAME, r + 12, 32);
    put32(gci + ENTRY_TIME, be32(r + 8));
    put32(gci + ENTRY_IMAGE, be32(r + 52));
    put16(gci + ENTRY_ICON_FORMAT, be16(r + 56));
    put16(gci + ENTRY_ANIMATION, be16(r + 58));
    gci[ENTRY_PERMISSION] = r[51];
    gci[ENTRY_COPY_COUNTER] = 0;
    put16(gci + ENTRY_FIRST_BLOCK, CARD_SYSTEM_BLOCKS);
    put16(gci + ENTRY_BLOCK_COUNT, GCZELDA_LENGTH / BLOCK);
    put16(gci + ENTRY_UNUSED, 0xFFFFu);
    put32(gci + ENTRY_COMMENTS, be32(r + 60));
    memcpy(gci + ENTRY, r + CARD_RECORD, GCZELDA_LENGTH);
    *out = gci;
    *out_size = total;
    return NULL;
}
