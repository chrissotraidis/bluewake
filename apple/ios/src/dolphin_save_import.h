// Move Wind Waker (GZLE01) saves between Dolphin and BlueWake's memory card.
//
// Dolphin exports a save as a .gci (a 64-byte GameCube directory entry, then
// the file's blocks) and keeps whole cards as .raw images. The game's save is
// the 'gczelda' file: 12 blocks of 0x2000, where blocks 1 and 2 are two copies
// of three 0x770-byte quest logs, each with its own checksum, followed by a
// checksum of the block (zeldaret/tww m_Do_MemCardRWmng.h). BlueWake's card is
// the GXRuntime container (magic DOLCARD1, ref/recompcore memory_card.c).
// build/device-setup/ww_save.py is the reference the import ports.
#ifndef BLUEWAKE_DOLPHIN_SAVE_IMPORT_H
#define BLUEWAKE_DOLPHIN_SAVE_IMPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BW_QUEST_LOGS 3

typedef struct BWQuestLog {
    bool empty;        // no player name: the game shows this quest log as new
    bool checksum_ok;  // the quest log's own checksum matches
    char name[17];     // player name, printable ASCII
    unsigned max_life; // quarter hearts
    unsigned rupees;
} BWQuestLog;

typedef struct BWDolphinSave {
    uint8_t entry[64]; // GameCube directory entry (the .gci header)
    uint8_t* data;     // the gczelda file, 98,304 bytes
    size_t length;
} BWDolphinSave;

// Each call returns NULL on success, or a short plain-English reason.

// Reads a .gci, or a raw memory card image holding a gczelda file.
const char* bw_dolphin_save_parse(const uint8_t* bytes, size_t size, BWDolphinSave* out);
void bw_dolphin_save_free(BWDolphinSave* save);

// The quest logs of a gczelda file (either copy, preferring the first).
void bw_gczelda_quest_logs(const uint8_t* gczelda, BWQuestLog out[BW_QUEST_LOGS]);

// The quest logs on a BlueWake card. *has_saves is false when the card has no
// gczelda file yet; out is then left zeroed.
const char* bw_card_quest_logs(const uint8_t* card, size_t size, bool* has_saves,
                               BWQuestLog out[BW_QUEST_LOGS]);

// Puts quest log src_slot (1-3) of the Dolphin save into slot dst_slot (1-3)
// of both copies on the card, with every checksum and container hash
// recomputed. If the card has no gczelda file yet, the whole Dolphin file is
// added instead and the slots are ignored. *out is malloc'd; the caller frees.
const char* bw_card_import(const uint8_t* card, size_t size, const BWDolphinSave* save,
                           int src_slot, int dst_slot, uint8_t** out, size_t* out_size);

// Writes the card's gczelda file out as a .gci (the 64-byte directory entry and
// the 98,304 bytes of save data), on demand: the card is only read, never
// changed. All three quest logs go, so a save made in BlueWake opens in Dolphin
// and copies to a real memory card. The bookkeeping that says where a file sits
// on a card (the copy counter and the first block) is not part of the container
// and Dolphin reassigns both when it imports, so they are written as 0 and the
// first user block. *out is malloc'd; the caller frees.
const char* bw_card_export(const uint8_t* card, size_t size, uint8_t** out, size_t* out_size);

#ifdef __cplusplus
}
#endif

#endif
