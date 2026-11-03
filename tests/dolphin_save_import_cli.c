// Command-line front end for apple/ios/src/dolphin_save_import.c, so the
// in-app importer can be checked against build/device-setup/ww_save.py
// (scripts/check_dolphin_import.py builds and runs it).
//
//   dolphin_save_import_cli show FILE                  (.gci, .raw or .card)
//   dolphin_save_import_cli inject CARD SRC SRC_SLOT DST_SLOT OUT_CARD
//   dolphin_save_import_cli export CARD OUT_GCI
#include "../apple/ios/src/dolphin_save_import.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t* slurp(const char* path, size_t* size) {
    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return NULL;
    fseek(f, 0, SEEK_END);
    *size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* bytes = (uint8_t*)malloc(*size ? *size : 1);
    if (bytes != NULL && fread(bytes, 1, *size, f) != *size) {
        free(bytes);
        bytes = NULL;
    }
    fclose(f);
    return bytes;
}

static void print_logs(const BWQuestLog logs[BW_QUEST_LOGS]) {
    for (int i = 0; i < BW_QUEST_LOGS; i++)
        printf("  slot %d: %s checksum %s hearts %.2f rupees %u name '%s'\n", i + 1,
               logs[i].empty ? "empty" : "used", logs[i].checksum_ok ? "ok" : "BAD",
               logs[i].max_life / 4.0, logs[i].rupees, logs[i].name);
}

int main(int argc, char** argv) {
    size_t size = 0;
    if (argc == 3 && strcmp(argv[1], "show") == 0) {
        uint8_t* bytes = slurp(argv[2], &size);
        if (bytes == NULL)
            return perror(argv[2]), 1;
        BWQuestLog logs[BW_QUEST_LOGS];
        if (size >= 8 && memcmp(bytes, "DOLCARD1", 8) == 0) {
            bool has = false;
            const char* error = bw_card_quest_logs(bytes, size, &has, logs);
            if (error != NULL)
                return fprintf(stderr, "%s\n", error), 1;
            printf("%s: BlueWake card, %s\n", argv[2], has ? "has saves" : "no saves yet");
        } else {
            BWDolphinSave save;
            const char* error = bw_dolphin_save_parse(bytes, size, &save);
            if (error != NULL)
                return fprintf(stderr, "%s\n", error), 1;
            bw_gczelda_quest_logs(save.data, logs);
            printf("%s: Dolphin save\n", argv[2]);
            bw_dolphin_save_free(&save);
        }
        print_logs(logs);
        free(bytes);
        return 0;
    }
    if (argc == 7 && strcmp(argv[1], "inject") == 0) {
        size_t source_size = 0;
        uint8_t* card = slurp(argv[2], &size);
        uint8_t* source = slurp(argv[3], &source_size);
        if (card == NULL || source == NULL)
            return fprintf(stderr, "could not read inputs\n"), 1;
        BWDolphinSave save;
        const char* error = bw_dolphin_save_parse(source, source_size, &save);
        uint8_t* out = NULL;
        size_t out_size = 0;
        if (error == NULL)
            error = bw_card_import(card, size, &save, atoi(argv[4]), atoi(argv[5]), &out, &out_size);
        if (error != NULL)
            return fprintf(stderr, "%s\n", error), 1;
        FILE* f = fopen(argv[6], "wb");
        if (f == NULL || fwrite(out, 1, out_size, f) != out_size || fclose(f) != 0)
            return perror(argv[6]), 1;
        printf("wrote %s\n", argv[6]);
        bw_dolphin_save_free(&save);
        free(out);
        free(card);
        free(source);
        return 0;
    }
    if (argc == 4 && strcmp(argv[1], "export") == 0) {
        uint8_t* card = slurp(argv[2], &size);
        if (card == NULL)
            return perror(argv[2]), 1;
        uint8_t* out = NULL;
        size_t out_size = 0;
        const char* error = bw_card_export(card, size, &out, &out_size);
        if (error != NULL) {
            fprintf(stderr, "%s\n", error);
            free(card);
            return 1;
        }
        FILE* f = fopen(argv[3], "wb");
        if (f == NULL || fwrite(out, 1, out_size, f) != out_size || fclose(f) != 0)
            return perror(argv[3]), 1;
        printf("wrote %s (%zu bytes)\n", argv[3], out_size);
        free(out);
        free(card);
        return 0;
    }
    fprintf(stderr, "usage: %s show FILE | inject CARD SRC SRC_SLOT DST_SLOT OUT | export CARD OUT_GCI\n",
            argv[0]);
    return 2;
}
