// SPDX-License-Identifier: GPL-3.0-or-later
// Actual launcher and POSIX files/processes; importer results are synthetic.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../linux/src/linux_disc.c"

static int check_failed, prepare_failed, preparations;
static char checked[4096];
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
    char path[4096]; snprintf(path, sizeof path, "%s/main.dol", out);
    put(path, "synthetic executable");
    if (prepare_failed) { snprintf(error, capacity, "Injected prepare failure"); return -1; }
    snprintf(path, sizeof path, "%s/rels", out); assert(mkdir(path, 0755) == 0);
    for (int i = 0; i < 415; ++i) {
        snprintf(path, sizeof path, "%s/rels/fixture-%03d.rel", out, i);
        put(path, "synthetic module");
    }
    return 0;
}
static void new_launch(void) {
    setenv("BLUEWAKE_DOL", "", 1); setenv("BLUEWAKE_RELS_DIR", "", 1); setenv("BLUEWAKE_DISC", "", 1);
}
int main(void) {
    char root[] = "/tmp/bluewake-disc-test-XXXXXX";
    char data[4096], exe[4096];
    assert(mkdtemp(root) != NULL);
    snprintf(data, sizeof data, "%s/data/", root);
    snprintf(exe, sizeof exe, "%s/app/", root);
    assert(mkdir(data, 0755) == 0);
    assert(mkdir(exe, 0755) == 0);
    setenv("BLUEWAKE_NO_DIALOG", "1", 1);
    setenv("BLUEWAKE_DISC_CHOICE", "", 1);
    new_launch(); assert(!bw_disc_has_usable_source(exe, data));
    assert(bw_disc_setup(exe, data, 0, 0) == 1); // cancellation
    char source[4096], canonical[4096], settings[4096];
    snprintf(source, sizeof source, "%s/input with spaces.iso", root);
    snprintf(canonical, sizeof canonical, "%sGZLE01.iso", data);
    snprintf(settings, sizeof settings, "%ssettings.ini", data);
    put(canonical, "previous converted disc"); put(settings, "preserve preferences");
    setenv("BLUEWAKE_DISC", source, 1); assert(bw_disc_setup(exe, data, 0, 0) == -1); // missing explicit file
    put(source, "synthetic original"); check_failed = 1;
    assert(bw_disc_setup(exe, data, 0, 0) == -1); same(source, "synthetic original");
    same(canonical, "previous converted disc");
    check_failed = 0; new_launch(); setenv("BLUEWAKE_DISC_CHOICE", source, 1);
    assert(bw_disc_setup(exe, data, 0, 0) == 0); assert(preparations == 1);
    char game[4096], record[4096], remembered[4096];
    snprintf(record, sizeof record, "%sgame.txt", data); assert(read_line(record, game, sizeof game));
    snprintf(remembered, sizeof remembered, "%sdisc.txt", data);
    char old_disc[4096]; assert(read_line(remembered, old_disc, sizeof old_disc));
    assert(strcmp(old_disc, source) == 0);
    new_launch(); assert(bw_disc_has_usable_source(exe, data));
    assert(bw_disc_setup(exe, data, 0, 0) == 0); assert(preparations == 1); // verified cache
    // A changed input and failed importer keep the old selection and outputs.
    put(source, "synthetic changed original"); prepare_failed = 1; new_launch();
    assert(bw_disc_setup(exe, data, 0, 0) == -1);
    char current[4096]; assert(read_line(record, current, sizeof current)); assert(strcmp(current, game) == 0);
    char old_dol[4096]; snprintf(old_dol, sizeof old_dol, "%s/main.dol", game);
    same(old_dol, "synthetic executable"); same(source, "synthetic changed original");
    prepare_failed = 0; new_launch(); assert(bw_disc_setup(exe, data, 0, 0) == 0); // retry into new directory
    assert(read_line(record, current, sizeof current)); assert(strcmp(current, game) != 0);
    same(old_dol, "synthetic executable");
    // Incomplete cache does not bypass preparation after an interruption.
    char rel[4096]; snprintf(rel, sizeof rel, "%s/rels/fixture-000.rel", current);
    assert(remove(rel) == 0); int before = preparations;
    new_launch(); assert(bw_disc_setup(exe, data, 0, 0) == 0); assert(preparations == before + 1);
    // An explicit CLI disc stays temporary, while the setup UI selection is remembered.
    char alternate[4096];
    snprintf(alternate, sizeof alternate, "%s/setup selection.iso", root);
    put(alternate, "synthetic setup selection");
    new_launch(); setenv("BLUEWAKE_DISC", alternate, 1);
    assert(bw_disc_setup(exe, data, 0, 0) == 0);
    assert(read_line(remembered, old_disc, sizeof old_disc)); assert(strcmp(old_disc, source) == 0);
    new_launch(); setenv("BLUEWAKE_DISC", alternate, 1);
    assert(bw_disc_setup(exe, data, 1, 0) == 0);
    assert(read_line(remembered, old_disc, sizeof old_disc)); assert(strcmp(old_disc, alternate) == 0);
    before = preparations; new_launch();
    assert(bw_disc_setup(exe, data, 0, 0) == 0); assert(preparations == before);
    // --iso forces the simple picker even when another remembered disc exists.
    new_launch(); setenv("BLUEWAKE_DISC_CHOICE", source, 1);
    assert(bw_disc_setup(exe, data, 0, 1) == 0);
    assert(read_line(remembered, old_disc, sizeof old_disc)); assert(strcmp(old_disc, source) == 0);
    // Compressed explicit input is rejected without replacing the last disc.
    char rvz[4096];
    snprintf(rvz, sizeof rvz, "%s/fixture.rvz", root); put(rvz, "synthetic compressed input");
    new_launch(); setenv("BLUEWAKE_DISC", rvz, 1); check_failed = 1;
    assert(bw_disc_setup(exe, data, 0, 0) == -1); assert(strcmp(checked, rvz) != 0);
    same(rvz, "synthetic compressed input");
    same(canonical, "previous converted disc"); same(settings, "preserve preferences");
    printf("linux_disc_test: OK (synthetic importer; files retained in %s)\n", root);
    return 0;
}
