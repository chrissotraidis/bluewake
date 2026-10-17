// Disc check of the run harness (docs/NATIVE_PORT_PHASE4_6.md, step 6.0, decision H9).
//
// COS_DISC must name a readable GameCube disc image (.iso) of the build's disc, revision 0 (GZLE01 for
// the USA build, GZLP01 for the European one: pc_game_id.h, docs/PAL_DISC_PLAN.md): the disc header
// (boot.bin) holds the game code and maker at 0x00, the version byte at 0x07 and the GameCube
// magic 0xC2339F3D at 0x1C, all big-endian. Anything else exits 14 before the SDK starts.
// Opening the disc for the game (aurora_dvd_open) is step 6.1; the SHA-1 check of the image on
// first use is done by native/tools/run.sh (main.dol SHA-1 against the supported revision).
#include "pc_internal.h"

#include "pc/pc_game_id.h"

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__SWITCH__)
#include "cos_switch.h"
#endif

namespace pc {

namespace {
char sDiscError[512];
// For the player's message (exitDisc): COS_DISC does not exist; COS_DISC is the other region's disc
// (revision 0), so the player started the other build or misnamed the image.
bool sDiscMissing;
bool sOtherRegionDisc;

#if defined(__SWITCH__)
// One build as the player knows it: its disc, its version's name, its download (pc_game_id.h). Both
// install as sdmc:/switch/switchwaker/switchwaker.nro (one region per SD card), so the remedy for the
// other region's disc is the other download's switchwaker.nro.
struct PlayerBuild {
    const char* id;          // GZLE01
    const char* iso;         // GZLE01.iso
    const char* discEn;      // the USA disc
    const char* discEs;      // el disco de EE. UU.
    const char* versionEn;   // the USA version
    const char* versionEs;   // la versión de EE. UU.
    const char* entry;       // SwitchWaker (the NACP name)
    const char* zip;         // -switch.zip
    const char* regionEn;    // USA
    const char* regionEs;    // EE. UU.
};
constexpr PlayerBuild kUsaBuild{PC_GAME_ID_USA, PC_GAME_ID_USA ".iso", "the USA disc", "el disco de EE. UU.",
                                "the USA version", "la versi\u00f3n de EE. UU.", PC_GAME_ENTRY_USA,
                                PC_GAME_ZIP_USA, "USA", "EE. UU."};
constexpr PlayerBuild kEurBuild{PC_GAME_ID_EUR, PC_GAME_ID_EUR ".iso", "the European disc", "el disco europeo",
                                "the European version", "la versi\u00f3n europea", PC_GAME_ENTRY_EUR,
                                PC_GAME_ZIP_EUR, "Europe", "Europa"};

// The disc folder: COS_SWITCH_DEFAULT_DISC without its file name.
constexpr const char kDiscDir[] = "sdmc:/switch/switchwaker/";

// The message for the player: with the other region's disc in the folder (and not this build's), or
// at this build's path, say which version this switchwaker.nro is and which download replaces it.
// English, then Spanish.
void playerMessage(char* text, size_t size) {
    const PlayerBuild& self = PC_GAME_IS_EUR ? kEurBuild : kUsaBuild;
    const PlayerBuild& other = PC_GAME_IS_EUR ? kUsaBuild : kEurBuild;
    struct stat st;
    const bool otherInFolder =
        sDiscMissing && stat((std::string("/switch/switchwaker/") + other.iso).c_str(), &st) == 0;
    if (otherInFolder) {
        snprintf(text, size,
                 "This switchwaker.nro is %s (%s), for %s (%s), but %s holds %s (%s). For that disc, download "
                 "the release file whose name ends in \"%s\" (%s) and replace switchwaker.nro with the one "
                 "inside it.\n\n"
                 "Este switchwaker.nro es %s (%s), para %s (%s), pero %s contiene %s (%s). Para ese disco, "
                 "descarga el archivo de la release cuyo nombre termina en \"%s\" (%s) y reemplaza "
                 "switchwaker.nro por el que trae.",
                 self.versionEn, self.entry, self.discEn, self.iso, kDiscDir, other.iso, other.discEn, other.zip,
                 other.regionEn, self.versionEs, self.entry, self.discEs, self.iso, kDiscDir, other.iso,
                 other.discEs, other.zip, other.regionEs);
        return;
    }
    if (sOtherRegionDisc) {
        snprintf(text, size,
                 "%s%s is %s (%s), and this switchwaker.nro is %s (%s). For %s, rename the file to %s, "
                 "download the release file whose name ends in \"%s\" (%s) and replace switchwaker.nro with "
                 "the one inside it.\n\n"
                 "%s%s es %s (%s), y este switchwaker.nro es %s (%s). Para %s, cambia el nombre del archivo "
                 "a %s, descarga el archivo de la release cuyo nombre termina en \"%s\" (%s) y reemplaza "
                 "switchwaker.nro por el que trae.",
                 kDiscDir, self.iso, other.discEn, other.id, self.versionEn, self.entry, other.discEn, other.iso,
                 other.zip, other.regionEn, kDiscDir, self.iso, other.discEs, other.id, self.versionEs, self.entry,
                 other.discEs, other.iso, other.zip, other.regionEs);
        return;
    }
    snprintf(text, size,
             "%s needs your own disc of The Wind Waker (%s, %s, revision 0) as an uncompressed image at %s%s.\n\n"
             "%s\n\n"
             "%s necesita tu propio disco de The Wind Waker (%s, %s, revision 0) como imagen sin comprimir "
             "en %s%s.",
             self.entry, PC_GAME_REGION_EN, self.id, kDiscDir, self.iso, sDiscError, self.entry, PC_GAME_REGION_ES,
             self.id, kDiscDir, self.iso);
}
#endif
} // namespace

void discMessage(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sDiscError, sizeof sDiscError, fmt, ap);
    va_end(ap);
    writef(STDERR_FILENO, "[cos] DISC: %s\n", sDiscError);
}

void exitDisc() {
#if defined(__SWITCH__)
    char text[2048];
    playerMessage(text, sizeof text);
    writef(STDERR_FILENO, "[cos] DISC: shown: %s\n", text);
    cos_switch_show_error(text);
#endif
    pc_exit(PC_EXIT_DISC);
    __builtin_unreachable();
}

int checkDisc() {
    const char* path = gConfig.disc;
    if (path == nullptr) {
        discMessage("COS_DISC is not set (path of the " COS_GAME_ID_STR " .iso)");
        return PC_EXIT_DISC;
    }
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        sDiscMissing = errno == ENOENT;
        discMessage("cannot open COS_DISC=%s: %s", path, strerror(errno));
        return PC_EXIT_DISC;
    }
    struct stat st;
    unsigned char header[0x20];
    ssize_t got = -1;
    if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
#if defined(__SWITCH__)
        // Horizon's C library has no pread.
        got = lseek(fd, 0, SEEK_SET) == 0 ? read(fd, header, sizeof(header)) : -1;
#else
        got = pread(fd, header, sizeof(header), 0);
#endif
    }
    close(fd);
    if (got != (ssize_t)sizeof(header)) {
        discMessage("%s is not a regular file with a disc header", path);
        return PC_EXIT_DISC;
    }
    if (memcmp(header, "CISO", 4) == 0) {
        discMessage("%s is a CISO image; convert it to a plain .iso (decision H9)", path);
        return PC_EXIT_DISC;
    }
    const uint32_t magic = ((uint32_t)header[0x1C] << 24) | ((uint32_t)header[0x1D] << 16) |
                           ((uint32_t)header[0x1E] << 8) | (uint32_t)header[0x1F];
    if (magic != 0xC2339F3Du) {
        discMessage("%s is not a GameCube disc image (magic 0x%08x at 0x1C; compressed?)", path, magic);
        return PC_EXIT_DISC;
    }
    char id[7];
    for (int i = 0; i < 6; i++) {
        id[i] = (header[i] >= 0x20 && header[i] < 0x7F) ? (char)header[i] : '?';
    }
    id[6] = '\0';
    const unsigned int version = header[7];
    if (strcmp(id, COS_GAME_ID_STR) != 0 || version != 0) {
        // the other region's disc: name the build that runs it
        const bool usa = strcmp(id, PC_GAME_ID_USA) == 0 && version == 0;
        const bool eur = strcmp(id, PC_GAME_ID_EUR) == 0 && version == 0;
        if (usa || eur) {
            sOtherRegionDisc = true;
            discMessage("%s is %s revision 0, the %s disc: it runs with the %s build (%s); this build is for %s",
                        path, id, usa ? "USA" : "European", usa ? "USA" : "European",
                        usa ? PC_GAME_NRO_USA : PC_GAME_NRO_EUR, COS_GAME_ID_STR);
        } else {
            discMessage("%s is %s revision %u; the supported disc is %s revision 0", path, id, version,
                        COS_GAME_ID_STR);
        }
        return PC_EXIT_DISC;
    }
    writef(STDERR_FILENO, "[cos] disc: %s %s revision 0, %lld bytes\n", path, COS_GAME_ID_STR,
           (long long)st.st_size);
    return 0;
}

} // namespace pc
