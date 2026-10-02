// SPDX-License-Identifier: GPL-3.0-or-later
// Actual launcher and Win32 files/processes; importer results are synthetic.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../windows/src/win_disc.c"

static int check_failed, prepare_failed, preparations;
static char checked[MAX_PATH * 4];
static void put(const char* path, const char* text) {
    FILE* f = fopen(path, "wb"); assert(f);
    assert(fwrite(text, 1, strlen(text), f) == strlen(text)); assert(fclose(f) == 0);
}
static void same(const char* path, const char* expected) {
    char text[256] = ""; FILE* f = fopen(path, "rb"); assert(f);
    size_t n = fread(text, 1, sizeof text - 1, f); assert(!ferror(f)); fclose(f);
    assert(n == strlen(expected) && strcmp(text, expected) == 0);
}
int bluewake_disc_check(const char* path, char* error, size_t capacity) {
    snprintf(checked, sizeof checked, "%s", path);
    if (check_failed) snprintf(error, capacity, "Synthetic unsupported disc");
    return check_failed ? -1 : 0;
}
int bluewake_disc_prepare(const char* disc, const char* out, BlueWakeImportProgress progress,
                          void* user, char* error, size_t capacity) {
    (void)disc; (void)progress; (void)user;
    ++preparations;
    char path[MAX_PATH * 4]; snprintf(path, sizeof path, "%s\\main.dol", out);
    put(path, "synthetic executable");
    if (prepare_failed) { snprintf(error, capacity, "Injected prepare failure"); return -1; }
    snprintf(path, sizeof path, "%s\\rels", out); assert(CreateDirectoryA(path, NULL));
    for (int i = 0; i < 415; ++i) {
        snprintf(path, sizeof path, "%s\\rels\\fixture-%03d.rel", out, i);
        put(path, "synthetic module");
    }
    return 0;
}
static void new_launch(void) {
    _putenv_s("BLUEWAKE_DOL", ""); _putenv_s("BLUEWAKE_RELS_DIR", ""); _putenv_s("BLUEWAKE_DISC", "");
}
int main(int argc, char** argv) {
    // A real child stands in for nodtool, writing only a synthetic fixture.
    if (argc == 5 && strcmp(argv[1], "--no-color") == 0 && strcmp(argv[2], "convert") == 0) {
        put(argv[4], "synthetic converted disc"); return 0;
    }
    char temp[MAX_PATH], root[MAX_PATH * 4], data[MAX_PATH * 4], exe[MAX_PATH * 4];
    assert(GetTempPathA(sizeof temp, temp));
    snprintf(root, sizeof root, "%sbluewake-disc-test-%lu-%llu", temp, GetCurrentProcessId(),
             (unsigned long long)GetTickCount64());
    assert(CreateDirectoryA(root, NULL));
    snprintf(data, sizeof data, "%s\\data\\", root); assert(CreateDirectoryA(data, NULL));
    snprintf(exe, sizeof exe, "%s\\app\\", root); assert(CreateDirectoryA(exe, NULL));
    _putenv_s("BLUEWAKE_NO_DIALOG", "1");
    _putenv_s("BLUEWAKE_DISC_CHOICE", "");
    new_launch(); assert(bw_disc_setup(exe, data) == 1); // cancellation
    char source[MAX_PATH * 4], canonical[MAX_PATH * 4], settings[MAX_PATH * 4];
    snprintf(source, sizeof source, "%s\\input with spaces.iso", root);
    snprintf(canonical, sizeof canonical, "%sGZLE01.iso", data);
    snprintf(settings, sizeof settings, "%ssettings.ini", data);
    put(canonical, "previous converted disc"); put(settings, "preserve preferences");
    _putenv_s("BLUEWAKE_DISC", source); assert(bw_disc_setup(exe, data) == -1); // missing explicit file
    put(source, "synthetic original"); check_failed = 1;
    assert(bw_disc_setup(exe, data) == -1); same(source, "synthetic original");
    same(canonical, "previous converted disc");
    check_failed = 0; new_launch(); _putenv_s("BLUEWAKE_DISC_CHOICE", source);
    assert(bw_disc_setup(exe, data) == 0); assert(preparations == 1);
    char game[MAX_PATH * 4], record[MAX_PATH * 4], remembered[MAX_PATH * 4];
    snprintf(record, sizeof record, "%sgame.txt", data); assert(read_line(record, game, sizeof game));
    snprintf(remembered, sizeof remembered, "%sdisc.txt", data);
    char old_disc[MAX_PATH * 4]; assert(read_line(remembered, old_disc, sizeof old_disc));
    assert(strcmp(old_disc, source) == 0);
    new_launch(); assert(bw_disc_setup(exe, data) == 0); assert(preparations == 1); // verified cache
    // A changed input and failed importer keep the old selection and outputs.
    put(source, "synthetic changed original"); prepare_failed = 1; new_launch();
    assert(bw_disc_setup(exe, data) == -1);
    char current[MAX_PATH * 4]; assert(read_line(record, current, sizeof current)); assert(strcmp(current, game) == 0);
    char old_dol[MAX_PATH * 4]; snprintf(old_dol, sizeof old_dol, "%s\\main.dol", game);
    same(old_dol, "synthetic executable"); same(source, "synthetic changed original");
    prepare_failed = 0; new_launch(); assert(bw_disc_setup(exe, data) == 0); // retry into new directory
    assert(read_line(record, current, sizeof current)); assert(strcmp(current, game) != 0);
    same(old_dol, "synthetic executable");
    // Incomplete cache does not bypass preparation after an interruption.
    char rel[MAX_PATH * 4]; snprintf(rel, sizeof rel, "%s\\rels\\fixture-000.rel", current);
    assert(DeleteFileA(rel)); int before = preparations;
    new_launch(); assert(bw_disc_setup(exe, data) == 0); assert(preparations == before + 1);
    // Compressed explicit input must be converted and rejected without replacing
    // the last converted disc. The failed output stays available for recovery.
    char rvz[MAX_PATH * 4], tool[MAX_PATH * 4], self[MAX_PATH * 4];
    snprintf(rvz, sizeof rvz, "%s\\fixture.rvz", root); put(rvz, "synthetic compressed input");
    snprintf(tool, sizeof tool, "%snodtool.exe", exe); assert(GetModuleFileNameA(NULL, self, sizeof self));
    assert(CopyFileA(self, tool, TRUE));
    new_launch(); _putenv_s("BLUEWAKE_DISC", rvz); check_failed = 1;
    assert(bw_disc_setup(exe, data) == -1); assert(strcmp(checked, rvz) != 0);
    same(checked, "synthetic converted disc"); same(rvz, "synthetic compressed input");
    same(canonical, "previous converted disc"); same(settings, "preserve preferences");
    printf("windows_disc_test: OK (synthetic importer; files retained in %s)\n", root);
    return 0;
}
