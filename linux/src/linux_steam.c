// Steam's Game Mode: detect it, and put BlueWake in the Steam library (#581).
//
// The Steam library's non-Steam entries live in one binary VDF file per account,
// userdata/<id>/config/shortcuts.vdf, which Steam rewrites when it exits. So
// the writer here backs the file up, refuses to run while Steam does, keeps
// every other entry exactly as it found it, and computes the same app id Steam
// does (CRC32 of "<exe>"<name>, high bit set) so artwork can be matched later.
//
#define _POSIX_C_SOURCE 200809L  // kill(2), dirent, access

// See linux_steam.h for the contract; the tests are tests/linux_steam_test.c.
#include "linux_steam.h"

#include <ctype.h>
#include <dirent.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Detection

static int env_truth(const char* name) {
    const char* v = getenv(name);
    if (v == NULL || v[0] == '\0')
        return 0;
    return strcmp(v, "0") != 0 && strcasecmp(v, "false") != 0;
}

static int steam_disabled(void) {
    const char* v = getenv("BLUEWAKE_STEAM");
    return v != NULL && (strcmp(v, "0") == 0 || strcasecmp(v, "false") == 0);
}

// A line "KEY=value" in a file, value unquoted. Returns 1 when found.
static int key_from_file(const char* path, const char* key, char* out, size_t size) {
    FILE* f = fopen(path, "r");
    if (f == NULL)
        return 0;
    char line[512];
    const size_t key_len = strlen(key);
    int found = 0;
    while (fgets(line, sizeof line, f) != NULL && !found) {
        if (strncmp(line, key, key_len) != 0 || line[key_len] != '=')
            continue;
        char* v = line + key_len + 1;
        char* end = v + strlen(v);
        while (end > v && (end[-1] == '\n' || end[-1] == '\r'))
            *--end = '\0';
        if (end - v >= 2 && v[0] == '"' && end[-1] == '"') {
            ++v;
            end[-1] = '\0';
        }
        snprintf(out, size, "%s", v);
        found = 1;
    }
    fclose(f);
    return found;
}

// A whole small file with the newline trimmed (a DMI attribute).
static int attr_from_file(const char* path, char* out, size_t size) {
    FILE* f = fopen(path, "r");
    if (f == NULL)
        return 0;
    char line[256];
    int ok = fgets(line, sizeof line, f) != NULL;
    fclose(f);
    if (!ok)
        return 0;
    char* end = line + strlen(line);
    while (end > line && (end[-1] == '\n' || end[-1] == '\r'))
        *--end = '\0';
    snprintf(out, size, "%s", line);
    return line[0] != '\0';
}

// One place decides, so the answer and the sentence cannot disagree.
static int steam_detect(char* why, size_t why_size) {
    if (steam_disabled()) {
        snprintf(why, why_size, "BLUEWAKE_STEAM=0");
        return 0;
    }
    if (env_truth("SteamDeck")) {
        snprintf(why, why_size, "SteamDeck=1 in the environment");
        return 1;
    }
    char id[64];
    if (key_from_file("/etc/os-release", "ID", id, sizeof id) && strcmp(id, "steamos") == 0) {
        snprintf(why, why_size, "ID=steamos in /etc/os-release");
        return 1;
    }
    char vendor[128], product[128];
    if (attr_from_file("/sys/class/dmi/id/board_vendor", vendor, sizeof vendor) &&
        strcmp(vendor, "Valve") == 0) {
        if (attr_from_file("/sys/class/dmi/id/product_name", product, sizeof product) &&
            (strcmp(product, "Jupiter") == 0 || strcmp(product, "Galileo") == 0))
            snprintf(why, why_size, "a Valve %s board", product);
        else
            snprintf(why, why_size, "a Valve board");
        return 1;
    }
    snprintf(why, why_size, "not SteamOS: no SteamDeck=1, no ID=steamos, no Valve board");
    return 0;
}

int bw_steam_available(void) {
    char why[192];
    return steam_detect(why, sizeof why);
}

const char* bw_steam_reason(void) {
    static char reason[192];
    steam_detect(reason, sizeof reason);
    return reason;
}

// ---------------------------------------------------------------------------
// Paths

static const char* home_dir(void) {
    const char* home = getenv("HOME");
    return home != NULL && home[0] != '\0' ? home : NULL;
}

// The account directory Steam is using. A Deck can hold more than one (a
// second account, or a directory left behind by an old install), and Steam
// touches the shortcuts.vdf of the account it is signed in to — so the newest
// one is the library to write to. With no such file anywhere, a single numeric
// directory is taken; several are ambiguous and are left alone.
static int account_dir(const char* userdata, char* out, size_t size) {
    DIR* d = opendir(userdata);
    if (d == NULL)
        return 0;
    char found[256] = "", only[256] = "";
    time_t newest = 0;
    struct dirent* e;
    int count = 0;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.')
            continue;
        int numeric = 1;
        for (const char* c = e->d_name; *c != '\0'; ++c)
            if (!isdigit((unsigned char)*c))
                numeric = 0;
        if (!numeric)
            continue;
        ++count;
        snprintf(only, sizeof only, "%s", e->d_name);
        char vdf[1024];
        snprintf(vdf, sizeof vdf, "%s/%s/config/shortcuts.vdf", userdata, e->d_name);
        struct stat st;
        if (stat(vdf, &st) == 0 && S_ISREG(st.st_mode) && st.st_mtime >= newest) {
            newest = st.st_mtime;
            snprintf(found, sizeof found, "%s", e->d_name);
        }
    }
    closedir(d);
    if (found[0] != '\0') {
        snprintf(out, size, "%s", found);
        return 1;
    }
    if (count != 1)
        return 0;
    snprintf(out, size, "%s", only);
    return 1;
}

int bw_steam_shortcuts_path(char* out, size_t size) {
    if (out == NULL || size == 0)
        return 0;
    out[0] = '\0';
    const char* override = getenv("BLUEWAKE_STEAM_VDF");
    if (override != NULL && override[0] != '\0') {
        snprintf(out, size, "%s", override);
        return 1;
    }
    const char* home = home_dir();
    if (home == NULL)
        return 0;
    static const char* const roots[] = {"/.steam/steam", "/.local/share/Steam"};
    for (size_t i = 0; i < sizeof roots / sizeof roots[0]; ++i) {
        char userdata[512], account[128];
        snprintf(userdata, sizeof userdata, "%s%s/userdata", home, roots[i]);
        if (!account_dir(userdata, account, sizeof account))
            continue;
        snprintf(out, size, "%s/%s/config/shortcuts.vdf", userdata, account);
        return 1;
    }
    return 0;
}

int bw_steam_running(void) {
    const char* home = home_dir();
    if (home == NULL)
        return 0;
    char pidfile[512], text[64];
    snprintf(pidfile, sizeof pidfile, "%s/.steam/steam/steam.pid", home);
    FILE* f = fopen(pidfile, "r");
    if (f == NULL)
        return 0;
    int ok = fgets(text, sizeof text, f) != NULL;
    fclose(f);
    if (!ok)
        return 0;
    const long pid = strtol(text, NULL, 10);
    if (pid <= 0)
        return 0;
    return kill((pid_t)pid, 0) == 0 || errno == EPERM;
}

// SteamOS ships steamos-add-to-steam (a wrapper over
// `steam steam://addnonsteamgame/<path>`). It is the route that works while
// Steam is running, which on a Deck is always — Gaming Mode starts it at login.
static const char* steam_helper(void) {
    const char* override = getenv("BLUEWAKE_STEAM_HELPER");
    if (override != NULL && override[0] != '\0')
        return access(override, X_OK) == 0 ? override : NULL;
    static const char* const candidates[] = {"/usr/bin/steamos-add-to-steam",
                                             "/usr/local/bin/steamos-add-to-steam",
                                             "/usr/bin/holo-add-to-steam"};
    for (size_t i = 0; i < sizeof candidates / sizeof candidates[0]; ++i)
        if (access(candidates[i], X_OK) == 0)
            return candidates[i];
    return NULL;
}

// Asks the running Steam to add `path`; the shortcut list is then Steam's own
// to write when it exits. `touch /tmp/addnonsteamgamefile` is what the wrapper
// itself does before it calls Steam, and what makes the URI act on a file.
static int steam_helper_add(const char* helper, const char* path, char* detail, size_t detail_size) {
    const int fd = open("/tmp/addnonsteamgamefile", O_CREAT | O_WRONLY, 0644);
    if (fd >= 0)
        close(fd);
    pid_t pid = 0;
    char* const argv[] = {(char*)helper, (char*)path, NULL};
    extern char** environ;
    const int spawned = posix_spawn(&pid, helper, NULL, NULL, argv, environ);
    if (spawned != 0) {
        if (detail != NULL && detail_size > 0)
            snprintf(detail, detail_size, "%s could not be run: %s", helper, strerror(spawned));
        return BW_STEAM_FAILED;
    }
    return BW_STEAM_ADDED;
}

// ---------------------------------------------------------------------------
// The binary VDF

enum { VDF_OBJECT = 0x00, VDF_STRING = 0x01, VDF_UINT32 = 0x02, VDF_END = 0x08 };

typedef struct {
    char* key;
    int type;                  // VDF_OBJECT, VDF_STRING, VDF_UINT32
    char* text;                // the string, or a nested object's bytes
    size_t size;               // the nested object's length
    unsigned long long number; // the integer
} bw_field;

typedef struct {
    bw_field* fields;
    size_t count;
    char* raw;       // the entry's own bytes, as Steam wrote them
    size_t raw_size; // ... so an entry this writer does not own comes back byte for byte
} bw_entry;

typedef struct {
    bw_entry* entries;
    size_t count;
} bw_entries;

static void entries_free(bw_entries* list) {
    for (size_t i = 0; i < list->count; ++i) {
        for (size_t f = 0; f < list->entries[i].count; ++f) {
            free(list->entries[i].fields[f].key);
            free(list->entries[i].fields[f].text);
        }
        free(list->entries[i].fields);
        free(list->entries[i].raw);
    }
    free(list->entries);
    list->entries = NULL;
    list->count = 0;
}

static int read_string(const unsigned char* buf, size_t size, size_t* pos, char** out) {
    const size_t start = *pos;
    while (*pos < size && buf[*pos] != 0)
        ++*pos;
    if (*pos >= size)
        return 0;
    const size_t len = *pos - start;
    char* s = (char*)malloc(len + 1);
    if (s == NULL)
        return 0;
    memcpy(s, buf + start, len);
    s[len] = '\0';
    ++*pos; // the terminator
    *out = s;
    return 1;
}

static int read_u32(const unsigned char* buf, size_t size, size_t* pos, unsigned long long* out) {
    if (size - *pos < 4)
        return 0;
    *out = (unsigned long long)buf[*pos] | ((unsigned long long)buf[*pos + 1] << 8) |
           ((unsigned long long)buf[*pos + 2] << 16) | ((unsigned long long)buf[*pos + 3] << 24);
    *pos += 4;
    return 1;
}

// Past one value of `type`, for a field this writer only carries along.
static int skip_value(const unsigned char* buf, size_t size, size_t* pos, int type, int depth) {
    if (depth > 4)
        return 0;
    if (type == VDF_STRING) {
        char* ignored = NULL;
        const int ok = read_string(buf, size, pos, &ignored);
        free(ignored);
        return ok;
    }
    if (type == VDF_UINT32) {
        unsigned long long ignored = 0;
        return read_u32(buf, size, pos, &ignored);
    }
    if (type == VDF_OBJECT) {
        while (*pos < size) {
            const int inner = buf[*pos];
            ++*pos;
            if (inner == VDF_END)
                return 1;
            char* key = NULL;
            if (!read_string(buf, size, pos, &key))
                return 0;
            free(key);
            if (!skip_value(buf, size, pos, inner, depth + 1))
                return 0;
        }
        return 0;
    }
    return 0;
}

// One entry's fields, up to its end marker. A nested object (Steam writes a
// `tags` object under every entry, for a game's categories) is kept as its own
// bytes, so an entry this writer does not own comes back unchanged.
static int parse_entry(const unsigned char* buf, size_t size, size_t* pos, bw_entry* entry) {
    while (*pos < size) {
        const int type = buf[*pos];
        ++*pos;
        if (type == VDF_END)
            return 1;
        if (type != VDF_OBJECT && type != VDF_STRING && type != VDF_UINT32)
            return 0; // a type this writer cannot carry along
        char* key = NULL;
        if (!read_string(buf, size, pos, &key))
            return 0;
        bw_field field = {key, type, NULL, 0, 0};
        int ok = 1;
        if (type == VDF_OBJECT) {
            const size_t start = *pos;
            ok = skip_value(buf, size, pos, VDF_OBJECT, 0);
            field.size = *pos - start;
            if (ok && field.size > 0) {
                field.text = (char*)malloc(field.size);
                if (field.text == NULL)
                    ok = 0;
                else
                    memcpy(field.text, buf + start, field.size);
            }
        } else if (type == VDF_STRING) {
            ok = read_string(buf, size, pos, &field.text);
        } else {
            ok = read_u32(buf, size, pos, &field.number);
        }
        if (!ok) {
            free(key);
            return 0;
        }
        bw_field* grown = (bw_field*)realloc(entry->fields, (entry->count + 1) * sizeof *grown);
        if (grown == NULL) {
            free(field.key);
            free(field.text);
            return 0;
        }
        entry->fields = grown;
        entry->fields[entry->count++] = field;
    }
    return 0;
}

// The entries under "shortcuts". A file with no such object is a valid, empty
// library (Steam writes one the first time a shortcut is added).
static int parse_shortcuts(const unsigned char* buf, size_t size, bw_entries* list) {
    size_t pos = 0;
    int seen_root = 0;
    while (pos < size) {
        const int type = buf[pos++];
        if (type == VDF_END)
            break;
        char* key = NULL;
        if (!read_string(buf, size, &pos, &key))
            return 0;
        if (type != VDF_OBJECT) {
            free(key);
            return 0;
        }
        if (strcmp(key, "shortcuts") != 0) {
            free(key);
            return 0;
        }
        free(key);
        seen_root = 1;
        while (pos < size) {
            const size_t entry_start = pos;
            const int entry_type = buf[pos++];
            if (entry_type == VDF_END)
                return 1;
            if (entry_type != VDF_OBJECT)
                return 0;
            char* index = NULL;
            if (!read_string(buf, size, &pos, &index))
                return 0;
            free(index);
            bw_entry entry = {NULL, 0, NULL, 0};
            if (!parse_entry(buf, size, &pos, &entry)) {
                for (size_t f = 0; f < entry.count; ++f) {
                    free(entry.fields[f].key);
                    free(entry.fields[f].text);
                }
                free(entry.fields);
                return 0;
            }
            entry.raw_size = pos - entry_start;
            entry.raw = (char*)malloc(entry.raw_size);
            if (entry.raw == NULL) {
                for (size_t f = 0; f < entry.count; ++f) {
                    free(entry.fields[f].key);
                    free(entry.fields[f].text);
                }
                free(entry.fields);
                return 0;
            }
            memcpy(entry.raw, buf + entry_start, entry.raw_size);
            bw_entry* grown = (bw_entry*)realloc(list->entries, (list->count + 1) * sizeof *grown);
            if (grown == NULL) {
                for (size_t f = 0; f < entry.count; ++f) {
                    free(entry.fields[f].key);
                    free(entry.fields[f].text);
                }
                free(entry.fields);
                return 0;
            }
            list->entries = grown;
            list->entries[list->count++] = entry;
        }
        return 1;
    }
    return seen_root;
}

static const char* field_text(const bw_entry* entry, const char* key) {
    if (entry == NULL) // a new entry: every kept field takes its default
        return NULL;
    for (size_t i = 0; i < entry->count; ++i)
        if (strcmp(entry->fields[i].key, key) == 0)
            return entry->fields[i].type == VDF_STRING ? entry->fields[i].text : NULL;
    return NULL;
}

// CRC32 (the one Steam uses for a non-Steam app id), bit by bit: no table, and
// this runs once per setup.
static unsigned long crc32_of(const char* text) {
    unsigned long crc = 0xFFFFFFFFul;
    for (const unsigned char* p = (const unsigned char*)text; *p != '\0'; ++p) {
        crc ^= *p;
        for (int i = 0; i < 8; ++i)
            crc = (crc >> 1) ^ (0xEDB88320ul & (unsigned long)-(long)(crc & 1u));
    }
    return (crc ^ 0xFFFFFFFFul) & 0xFFFFFFFFul;
}

static unsigned long app_id_for(const char* exe, const char* name) {
    char buffer[2048];
    snprintf(buffer, sizeof buffer, "\"%s\"%s", exe, name);
    return (crc32_of(buffer) | 0x80000000ul) & 0xFFFFFFFFul;
}

static int load_entries(const char* path, bw_entries* list, int* existed) {
    *existed = 0;
    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return 1; // no library yet: an empty one is written
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }
    const long size = ftell(f);
    if (size < 0 || size > (long)(16 * 1024 * 1024) || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }
    unsigned char* buf = (unsigned char*)malloc((size_t)size + 1);
    if (buf == NULL) {
        fclose(f);
        return 0;
    }
    const size_t read = fread(buf, 1, (size_t)size, f);
    fclose(f);
    if (read != (size_t)size) {
        free(buf);
        return 0;
    }
    *existed = 1;
    const int ok = parse_shortcuts(buf, read, list);
    free(buf);
    return ok;
}

// What identifies an entry: the AppImage it was made for (ShortcutPath when
// there is one, else Exe, which is the same thing unless the AppImage had to be
// wrapped in `env`).
static const char* entry_identity(const bw_entry* entry) {
    const char* shortcut = field_text(entry, "ShortcutPath");
    if (shortcut != NULL && shortcut[0] != '\0')
        return shortcut;
    return field_text(entry, "Exe");
}

int bw_steam_shortcut_state(const char* identity, const char* name) {
    char path[1024];
    if (!bw_steam_shortcuts_path(path, sizeof path))
        return BW_STEAM_ABSENT;
    bw_entries list = {NULL, 0};
    int existed = 0;
    if (!load_entries(path, &list, &existed)) {
        entries_free(&list);
        return BW_STEAM_ABSENT;
    }
    int state = BW_STEAM_ABSENT;
    for (size_t i = 0; i < list.count; ++i) {
        const char* entry_name = field_text(&list.entries[i], "AppName");
        if (entry_name == NULL || strcmp(entry_name, name) != 0)
            continue;
        const char* entry_path = entry_identity(&list.entries[i]);
        state = entry_path != NULL && strcmp(entry_path, identity) == 0 ? BW_STEAM_CURRENT
                                                                        : BW_STEAM_STALE;
        break;
    }
    entries_free(&list);
    return state;
}

// --- writing

static int write_bytes(FILE* f, const void* data, size_t size) {
    return size == 0 || fwrite(data, 1, size, f) == size;
}

static int write_key(FILE* f, int type, const char* key) {
    const unsigned char t = (unsigned char)type;
    return write_bytes(f, &t, 1) && write_bytes(f, key, strlen(key) + 1);
}

static int write_raw_field(FILE* f, int type, const char* key, const char* bytes, size_t size) {
    return write_key(f, type, key) && write_bytes(f, bytes, size);
}

static int write_string_field(FILE* f, const char* key, const char* value) {
    return write_key(f, VDF_STRING, key) && write_bytes(f, value, strlen(value) + 1);
}

static int write_u32_field(FILE* f, const char* key, unsigned long value) {
    unsigned char bytes[4] = {(unsigned char)(value & 0xFFu), (unsigned char)((value >> 8) & 0xFFu),
                              (unsigned char)((value >> 16) & 0xFFu),
                              (unsigned char)((value >> 24) & 0xFFu)};
    return write_key(f, VDF_UINT32, key) && write_bytes(f, bytes, 4);
}

static const char* entry_text_or(const bw_entry* entry, const char* key, const char* fallback) {
    const char* v = field_text(entry, key);
    return v != NULL ? v : fallback;
}

// One entry, in Steam's own field order. `entry` is the existing one when there
// is one: its values are kept, and the four that identify the shortcut are set.
static int write_entry(FILE* f, size_t index, const bw_entry* entry, const char* exe,
                       const char* identity, const char* name, const char* launch_options) {
    char dir[1024];
    snprintf(dir, sizeof dir, "%s", exe);
    char* slash = strrchr(dir, '/');
    if (slash != NULL)
        *slash = '\0';
    char key[32];
    snprintf(key, sizeof key, "%zu", index);
    if (!write_key(f, VDF_OBJECT, key))
        return 0;
    if (!write_u32_field(f, "appid", app_id_for(exe, name)))
        return 0;
    if (!write_string_field(f, "AppName", name) || !write_string_field(f, "Exe", exe) ||
        !write_string_field(f, "StartDir", dir))
        return 0;
    if (!write_string_field(f, "icon", entry_text_or(entry, "icon", "")))
        return 0;
    if (!write_string_field(f, "ShortcutPath", identity))
        return 0;
    if (!write_string_field(f, "LaunchOptions", launch_options != NULL ? launch_options : ""))
        return 0;
    if (!write_u32_field(f, "IsHidden", 0) || !write_u32_field(f, "AllowDesktopConfig", 1) ||
        !write_u32_field(f, "AllowOverlay", 1) || !write_u32_field(f, "OpenVR", 0) ||
        !write_u32_field(f, "Devkit", 0))
        return 0;
    if (!write_string_field(f, "DevkitGameID", entry_text_or(entry, "DevkitGameID", "")))
        return 0;
    if (!write_u32_field(f, "LastPlayTime", 0))
        return 0;
    // Everything this writer does not set itself: Steam's `tags` object (the
    // game's categories) is the one that matters, and dropping it would lose
    // the player's own library organisation.
    static const char* const known[] = {"appid", "AppName", "Exe", "StartDir", "icon",
                                        "ShortcutPath", "LaunchOptions", "IsHidden",
                                        "AllowDesktopConfig", "AllowOverlay", "OpenVR", "Devkit",
                                        "DevkitGameID", "LastPlayTime"};
    if (entry != NULL)
        for (size_t i = 0; i < entry->count; ++i) {
            const bw_field* field = &entry->fields[i];
            int mine = 0;
            for (size_t k = 0; k < sizeof known / sizeof known[0]; ++k)
                if (strcmp(field->key, known[k]) == 0)
                    mine = 1;
            if (mine)
                continue;
            if (field->type == VDF_OBJECT) {
                if (!write_raw_field(f, VDF_OBJECT, field->key, field->text, field->size))
                    return 0;
            } else if (field->type == VDF_STRING) {
                if (!write_string_field(f, field->key, field->text))
                    return 0;
            } else if (!write_u32_field(f, field->key, (unsigned long)field->number)) {
                return 0;
            }
        }
    const unsigned char end = VDF_END;
    return write_bytes(f, &end, 1);
}

static int copy_file(const char* from, const char* to) {
    FILE* in = fopen(from, "rb");
    if (in == NULL)
        return 0;
    FILE* out = fopen(to, "wb");
    if (out == NULL) {
        fclose(in);
        return 0;
    }
    char buffer[64 * 1024];
    size_t n;
    int ok = 1;
    while ((n = fread(buffer, 1, sizeof buffer, in)) > 0)
        if (fwrite(buffer, 1, n, out) != n) {
            ok = 0;
            break;
        }
    if (ferror(in))
        ok = 0;
    fclose(in);
    if (fclose(out) != 0)
        ok = 0;
    return ok;
}

int bw_steam_add_shortcut(const char* exe, const char* identity, const char* name,
                          const char* launch_options, char* detail, size_t detail_size) {
    if (detail != NULL && detail_size > 0)
        detail[0] = '\0';
    if (exe == NULL || exe[0] != '/' || identity == NULL || identity[0] != '/' || name == NULL ||
        name[0] == '\0') {
        if (detail != NULL && detail_size > 0)
            snprintf(detail, detail_size, "the shortcut needs an absolute executable path and a name");
        return BW_STEAM_FAILED;
    }
    char path[1024];
    if (!bw_steam_shortcuts_path(path, sizeof path)) {
        if (detail != NULL && detail_size > 0)
            snprintf(detail, detail_size,
                     "no Steam userdata found: launch Steam once, or sign in, and run this again");
        return BW_STEAM_FAILED;
    }
    bw_entries list = {NULL, 0};
    int existed = 0;
    if (!load_entries(path, &list, &existed)) {
        entries_free(&list);
        if (detail != NULL && detail_size > 0)
            snprintf(detail, detail_size, "%s could not be read as a Steam shortcut list", path);
        return BW_STEAM_FAILED;
    }
    size_t match = list.count;
    for (size_t i = 0; i < list.count; ++i) {
        const char* entry_name = field_text(&list.entries[i], "AppName");
        if (entry_name != NULL && strcmp(entry_name, name) == 0) {
            match = i;
            break;
        }
    }
    if (match < list.count) {
        const char* entry_path = entry_identity(&list.entries[match]);
        if (entry_path != NULL && strcmp(entry_path, identity) == 0) {
            entries_free(&list);
            return BW_STEAM_UNCHANGED;
        }
    }
    if (bw_steam_running()) {
        // Steam rewrites its shortcut list when it exits, so a file written now
        // would be lost. On SteamOS the helper asks the running Steam instead,
        // and that route needs the AppImage's own path, not a wrapper.
        const char* helper = steam_helper();
        entries_free(&list);
        if (helper != NULL) {
            const int asked = steam_helper_add(helper, identity, detail, detail_size);
            if (asked == BW_STEAM_ADDED && detail != NULL && detail_size > 0)
                snprintf(detail, detail_size,
                         "asked Steam to add this AppImage; it appears in the library when Steam next "
                         "refreshes it");
            return asked;
        }
        if (detail != NULL && detail_size > 0)
            snprintf(detail, detail_size,
                     "Steam is running: close it and run this again (Steam rewrites its shortcut list "
                     "when it exits)");
        return BW_STEAM_FAILED;
    }

    char temp[1100];
    snprintf(temp, sizeof temp, "%s.added-by-bluewake", path);
    FILE* f = fopen(temp, "wb");
    if (f == NULL) {
        entries_free(&list);
        if (detail != NULL && detail_size > 0)
            snprintf(detail, detail_size, "%s could not be written: %s", temp, strerror(errno));
        return BW_STEAM_FAILED;
    }
    int ok = write_key(f, VDF_OBJECT, "shortcuts");
    for (size_t i = 0; ok && i < list.count; ++i) {
        const int updating = i == match;
        if (!updating && list.entries[i].raw != NULL) {
            // Not ours: its own bytes come back exactly as they were, so a
            // field this writer does not know about cannot be lost.
            ok = write_bytes(f, list.entries[i].raw, list.entries[i].raw_size);
            continue;
        }
        ok = write_entry(f, i, &list.entries[i],
                         updating ? exe : entry_text_or(&list.entries[i], "Exe", ""),
                         updating ? identity : entry_text_or(&list.entries[i], "ShortcutPath", ""),
                         updating ? name : entry_text_or(&list.entries[i], "AppName", ""),
                         updating ? (launch_options != NULL ? launch_options : "")
                                  : entry_text_or(&list.entries[i], "LaunchOptions", ""));
    }
    if (ok && match == list.count)
        ok = write_entry(f, list.count, NULL, exe, identity, name, launch_options);
    const unsigned char ends[2] = {VDF_END, VDF_END};
    if (ok)
        ok = write_bytes(f, ends, sizeof ends);
    if (fclose(f) != 0)
        ok = 0;
    const int added = match == list.count;
    entries_free(&list);
    if (!ok) {
        remove(temp);
        if (detail != NULL && detail_size > 0)
            snprintf(detail, detail_size, "the shortcut list could not be written");
        return BW_STEAM_FAILED;
    }
    if (existed) {
        char backup[1100];
        snprintf(backup, sizeof backup, "%s.bak", path);
        if (!copy_file(path, backup)) {
            remove(temp);
            if (detail != NULL && detail_size > 0)
                snprintf(detail, detail_size, "%s could not be backed up", path);
            return BW_STEAM_FAILED;
        }
    }
    if (rename(temp, path) != 0) {
        remove(temp);
        if (detail != NULL && detail_size > 0)
            snprintf(detail, detail_size, "%s could not be replaced: %s", path, strerror(errno));
        return BW_STEAM_FAILED;
    }
    return added ? BW_STEAM_ADDED : BW_STEAM_UPDATED;
}
