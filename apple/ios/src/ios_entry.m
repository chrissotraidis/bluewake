// BlueWake iOS/iPadOS/tvOS entry shim.
//
// SDL owns main() on iOS (SDL_main.h renames ours to SDL_main and starts
// UIApplicationMain first). This file only fills in default paths inside the
// app container, then runs the unchanged host.
//
// Data layout, all user-provided and never bundled. iOS/iPadOS use Documents;
// tvOS uses Library/Caches because this Apple TV rejects writes to Documents
// and Application Support:
//   Library/Caches/BlueWake/GZLE01.iso         the user's disc image
//   Library/Caches/BlueWake/main.dol, rels/    prepared from that disc on the
//                                              device (first_run.m)
//   Library/Caches/BlueWake/GZLE01.card        the memory card (saves)
//   Frameworks/gGZLE01_recomp.dylib            the translated composite, built
//                                              on a Mac from the same disc
//                                              (Documents/BlueWake in the
//                                              simulator)
//   Documents/BlueWake/dsp_rom.bin, dsp_coef.bin  only for BLUEWAKE_DSP_MODE=lle
// When any required piece is missing, the first-run screen says which and
// imports the disc.
// In the simulator dev loop, BLUEWAKE_ROOT (passed as SIMCTL_CHILD_BLUEWAKE_ROOT)
// points at the repository instead and the host resolves its usual layout.
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include "gxruntime/memory_card.h"
#include <TargetConditionals.h>
#include <SDL3/SDL_main.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <CommonCrypto/CommonDigest.h>

#include "first_run.h"
#include "atomic_file.h"
#include "controller_settings.h" // Shared settings keys; the tvOS shell uses controller defaults.
#if !TARGET_OS_TV
#include "touch_controls.h"
#endif

int bluewake_host_main(int argc, char** argv);

static void bw_default(const char* name, NSString* value) {
    const char* existing = getenv(name);
    if (existing != NULL && existing[0] != '\0') return;
    setenv(name, value.fileSystemRepresentation, 1);
}

// SHA-1 of the executable inside a GameCube disc image (its offset is at 0x420
// of the disc header; the DOL's size is the end of its furthest section), or
// nil if the file is missing or not a disc. Only for the legacy mod format.
static NSString* bw_iso_dol_sha1(NSString* path) {
    NSFileHandle* f = [NSFileHandle fileHandleForReadingAtPath:path];
    if (f == nil)
        return nil;
    NSString* result = nil;
    @try {
        [f seekToFileOffset:0x420];
        NSData* off = [f readDataOfLength:4];
        if (off.length == 4) {
            const uint8_t* o = off.bytes;
            const uint64_t dol = ((uint32_t)o[0] << 24) | ((uint32_t)o[1] << 16) | ((uint32_t)o[2] << 8) | o[3];
            [f seekToFileOffset:dol];
            NSData* hdr = [f readDataOfLength:0x100];
            if (hdr.length == 0x100) {
                const uint8_t* h = hdr.bytes;
                uint64_t size = 0x100;
                for (int i = 0; i < 18; i++) {
                    const uint8_t* p = h + i * 4;
                    const uint8_t* q = h + 0x90 + i * 4;
                    const uint32_t so = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
                    const uint32_t ss = ((uint32_t)q[0] << 24) | ((uint32_t)q[1] << 16) | ((uint32_t)q[2] << 8) | q[3];
                    if (ss != 0 && (uint64_t)so + ss > size)
                        size = (uint64_t)so + ss;
                }
                if (size < (16u << 20)) {
                    [f seekToFileOffset:dol];
                    NSData* bytes = [f readDataOfLength:size];
                    if (bytes.length == size) {
                        uint8_t digest[CC_SHA1_DIGEST_LENGTH];
                        CC_SHA1(bytes.bytes, (CC_LONG)bytes.length, digest);
                        NSMutableString* hex = [NSMutableString string];
                        for (int i = 0; i < CC_SHA1_DIGEST_LENGTH; i++)
                            [hex appendFormat:@"%02x", digest[i]];
                        result = hex;
                    }
                }
            }
        }
    } @catch (NSException* e) {
        result = nil;
    }
    [f closeFile];
    return result;
}

static void bw_default_if_exists(const char* name, NSString* path) {
    if ([[NSFileManager defaultManager] fileExistsAtPath:path])
        bw_default(name, path);
}

// Session log. Everything the app writes to stdout and stderr also goes to
// the app data folder's logs/session-YYYYMMDD-HHMMSS.log, each line stamped with
// the local wall-clock time, so a session can be read back from the device
// (Finder, Files, or afcclient) without a console attached, and a moment the
// player remembers ("it lagged around 3:42") can be found in it. The original
// stderr still receives every line, so devicectl --console keeps working. The
// newest eight sessions are kept.
static int g_log_console_fd = -1;
static FILE* g_log_file = NULL;

static void* bw_log_pump(void* arg) {
    const int read_fd = (int)(intptr_t)arg;
    char buffer[16384];
    char line[8192];
    size_t line_len = 0;
    for (;;) {
        const ssize_t n = read(read_fd, buffer, sizeof buffer);
        if (n <= 0) break;
        if (g_log_console_fd >= 0) (void)write(g_log_console_fd, buffer, (size_t)n);
        for (ssize_t i = 0; i < n; i++) {
            if (line_len < sizeof line - 1) line[line_len++] = buffer[i];
            if (buffer[i] != '\n') continue;
            struct timeval tv;
            gettimeofday(&tv, NULL);
            struct tm local;
            localtime_r(&tv.tv_sec, &local);
            fprintf(g_log_file, "%02d:%02d:%02d.%03d ", local.tm_hour, local.tm_min,
                    local.tm_sec, (int)(tv.tv_usec / 1000));
            fwrite(line, 1, line_len, g_log_file);
            line_len = 0;
        }
        fflush(g_log_file);
    }
    return NULL;
}

static void bw_start_session_log(NSString* data) {
    NSString* dir = [data stringByAppendingPathComponent:@"logs"];
    NSFileManager* fm = [NSFileManager defaultManager];
    [fm createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil];
    NSArray<NSString*>* old = [[[fm contentsOfDirectoryAtPath:dir error:nil]
        filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"SELF BEGINSWITH 'session-'"]]
        sortedArrayUsingSelector:@selector(compare:)];
    for (NSUInteger i = 0; i + 7 < old.count; i++)
        [fm removeItemAtPath:[dir stringByAppendingPathComponent:old[i]] error:nil];
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    char name[64];
    strftime(name, sizeof name, "session-%Y%m%d-%H%M%S.log", &local);
    NSString* path = [dir stringByAppendingPathComponent:@(name)];
    g_log_file = fopen(path.fileSystemRepresentation, "w");
    int fds[2];
    if (g_log_file == NULL || pipe(fds) != 0) return;
    g_log_console_fd = dup(STDERR_FILENO);
    dup2(fds[1], STDOUT_FILENO);
    dup2(fds[1], STDERR_FILENO);
    close(fds[1]);
    pthread_t thread;
    pthread_create(&thread, NULL, bw_log_pump, (void*)(intptr_t)fds[0]);
    pthread_detach(thread);
}

// Keep damaged cards and let the player choose a local backup or a new card.
// This runs before the host opens storage, on UIKit's existing SDL main loop.
@interface BWCardRecovery : UIViewController
@property(nonatomic, copy) NSString* card;
@property(nonatomic, copy) NSString* backups;
@property(nonatomic) BOOL finished;
@end
@implementation BWCardRecovery
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self choose];
}
- (void)replaceFrom:(NSString*)backup {
    NSFileManager* fm = NSFileManager.defaultManager;
    NSString* staged = [self.card stringByAppendingFormat:@".recovery-%@", NSUUID.UUID.UUIDString];
    NSError* error = nil;
    if (backup && (!dol_card_validate(backup.fileSystemRepresentation) ||
                   ![fm copyItemAtPath:backup toPath:staged error:&error] ||
                   !bw_atomic_flush_path(staged.fileSystemRepresentation) ||
                   !dol_card_validate(staged.fileSystemRepresentation))) {
        [self failure:error.localizedDescription ?: @"That backup is incomplete or damaged."];
        return;
    }
    NSString* preserved = [self.card stringByAppendingFormat:@".corrupt-%@", NSUUID.UUID.UUIDString];
    if ([fm fileExistsAtPath:self.card]) {
        // A backup replaces the canonical path atomically. Keep that path
        // present until publication, so an interrupted restore still offers
        // recovery on the next launch. Only an explicit new-card choice moves it.
        BOOL kept = backup ? [fm copyItemAtPath:self.card toPath:preserved error:&error]
                           : [fm moveItemAtPath:self.card toPath:preserved error:&error];
        if (!kept || (backup && !bw_atomic_flush_path(preserved.fileSystemRepresentation))) {
            [self failure:error.localizedDescription ?: @"Your original card could not be preserved."];
            return;
        }
    }
    if (backup && rename(staged.fileSystemRepresentation, self.card.fileSystemRepresentation) != 0) {
        // The original remains at the unique preserved path; never remove it.
        [self failure:@"The backup could not be put in place. Your original card was preserved."];
        return;
    }
    fprintf(stderr, "[card-recovery] original preserved at %s\n", preserved.fileSystemRepresentation);
    self.finished = YES;
}
- (void)failure:(NSString*)message {
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"Card Not Replaced"
        message:message preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
        handler:^(UIAlertAction* action) { (void)action; [self choose]; }]];
    [self presentViewController:alert animated:YES completion:nil];
}
- (void)chooseBackup {
    NSFileManager* fm = NSFileManager.defaultManager;
    NSMutableArray<NSString*>* candidates = [NSMutableArray new];
    for (NSString* name in [fm contentsOfDirectoryAtPath:self.backups error:nil]) {
        NSString* path = [self.backups stringByAppendingPathComponent:name];
        if (dol_card_validate(path.fileSystemRepresentation)) [candidates addObject:path];
    }
    NSString* bak = [self.card stringByAppendingString:@".bak"];
    if (dol_card_validate(bak.fileSystemRepresentation)) [candidates addObject:bak];
    if (!candidates.count) {
        [self failure:@"No valid local backup was found. Your damaged card is kept. You can start a new card and use Restore Saves from the menu to choose an exported backup."];
        return;
    }
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"Choose a Backup"
        message:@"Your damaged card will be preserved before restoring."
        preferredStyle:UIAlertControllerStyleAlert];
    for (NSString* path in [candidates sortedArrayUsingSelector:@selector(compare:)])
        [alert addAction:[UIAlertAction actionWithTitle:path.lastPathComponent style:UIAlertActionStyleDefault
            handler:^(UIAlertAction* action) { (void)action; [self replaceFrom:path]; }]];
    [alert addAction:[UIAlertAction actionWithTitle:@"Back" style:UIAlertActionStyleCancel
        handler:^(UIAlertAction* action) { (void)action; [self choose]; }]];
    [self presentViewController:alert animated:YES completion:nil];
}
- (void)choose {
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"Memory Card Needs Recovery"
        message:@"BlueWake cannot read this card. Your saves will be kept unchanged unless you choose a backup or a new card. The original is always preserved."
        preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"Backups" style:UIAlertActionStyleDefault
        handler:^(UIAlertAction* action) { (void)action; [self chooseBackup]; }]];
    [alert addAction:[UIAlertAction actionWithTitle:@"Start a New Card" style:UIAlertActionStyleDestructive
        handler:^(UIAlertAction* action) { (void)action; [self replaceFrom:nil]; }]];
    [self presentViewController:alert animated:YES completion:nil];
}
@end

static void bw_recover_card(NSString* card, NSString* backups) {
    if (![NSFileManager.defaultManager fileExistsAtPath:card] ||
        dol_card_validate(card.fileSystemRepresentation)) return;
    BWCardRecovery* controller = [BWCardRecovery new];
    controller.card = card;
    controller.backups = backups;
    UIWindow* window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    for (UIScene* scene in UIApplication.sharedApplication.connectedScenes)
        if ([scene isKindOfClass:UIWindowScene.class]) { window.windowScene = (UIWindowScene*)scene; break; }
    window.windowLevel = UIWindowLevelAlert + 1;
    window.rootViewController = controller;
    [window makeKeyAndVisible];
    while (!controller.finished) {
        @autoreleasepool {
            [NSRunLoop.currentRunLoop runMode:NSDefaultRunLoopMode
                beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
        }
    }
    window.hidden = YES;
    window.rootViewController = nil;
}

int main(int argc, char** argv) {
    @autoreleasepool {
        // Logs go to files under simctl/devicectl; keep them readable live.
        setvbuf(stdout, NULL, _IOLBF, 0);
        setvbuf(stderr, NULL, _IOLBF, 0);
#if TARGET_OS_TV
        NSString* caches = [NSSearchPathForDirectoriesInDomains(
            NSCachesDirectory, NSUserDomainMask, YES) firstObject];
        NSString* data = [caches stringByAppendingPathComponent:@"BlueWake"];
#else
        NSString* docs = [NSSearchPathForDirectoriesInDomains(
            NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
        NSString* data = [docs stringByAppendingPathComponent:@"BlueWake"];
#endif
        NSError* dataDirectoryError = nil;
        if (![[NSFileManager defaultManager] createDirectoryAtPath:data
                                     withIntermediateDirectories:YES
                                                      attributes:nil
                                                           error:&dataDirectoryError])
            fprintf(stderr, "[ios] cannot create data directory %s: %s\n",
                    data.fileSystemRepresentation,
                    dataDirectoryError.localizedDescription.UTF8String ?: "unknown error");
        if (getenv("BLUEWAKE_SESSION_LOG") == NULL ||
            strcmp(getenv("BLUEWAKE_SESSION_LOG"), "0") != 0)
            bw_start_session_log(data);
        setvbuf(stdout, NULL, _IOLBF, 0);
        setvbuf(stderr, NULL, _IOLBF, 0);
        // One line a second: speed, the worst frame gap, dropped frames and
        // how busy the game thread was (runtime/host/src/main.c). And the
        // player's inputs next to the moments the game reads them.
        bw_default("BLUEWAKE_PERF_LOG", @"1");
        bw_default("BLUEWAKE_INPUT_LOG", @"1");
        // Pace retraces by the wall clock (runtime/host/src/main.c
        // host_wall_pace) instead of by the audio queue alone.
        bw_default("BLUEWAKE_WALL_PACE", @"1");
        bw_default("DOL_AUDIO_NO_THROTTLE", @"1");

        bw_default("BLUEWAKE_RENDERER", @"aurora");
        bw_default("BLUEWAKE_CYCLE_CAP", @"16384");
        bw_default("BLUEWAKE_MAX_BLOCKS", @"100000000000");
        // Dolphin's high-level Zelda ucode: about 15% fewer play-window cycles
        // than the LLE interpreter, with an audio envelope that matches it at
        // r=0.999 (docs/status/CURRENT.md). BLUEWAKE_DSP_MODE=lle restores LLE.
        bw_default("BLUEWAKE_DSP_MODE", @"hle");
        // The console's SRAM (ipl_sram.h): without it the game read an empty
        // SRAM, took mono from OSGetSoundMode and played mono. Dolphin's
        // defaults (stereo), kept in the data folder so the game's own
        // Stereo/Mono option persists. BLUEWAKE_SRAM=0 turns it off.
        bw_default("BLUEWAKE_SRAM", [data stringByAppendingPathComponent:@"sram.bin"]);
        // The console clock: saves carry the real local date and time (the
        // file select shows it) instead of 01/01/2000.
        bw_default("BLUEWAKE_CLOCK", @"now");
        // The shell's aspect choice (BWGameOverlay.mm) applies at launch:
        // 0 keeps the original 4:3 picture, 1 fills the screen.
        if ([[NSUserDefaults standardUserDefaults] integerForKey:@"BlueWake.AspectMode"] == 1)
            bw_default("DOL_AURORA_ASPECT_FIT", @"0");
        const char* root = getenv("BLUEWAKE_ROOT");
        const char* composite_env = getenv("BLUEWAKE_COMPOSITE");
        NSString* composite = composite_env != NULL && composite_env[0] != '\0'
            ? [NSString stringWithUTF8String:composite_env] : nil;
        if (composite == nil && (root == NULL || root[0] == '\0')) {
            NSString* inData = [data stringByAppendingPathComponent:@"gGZLE01_recomp.dylib"];
            NSString* inBundle = [[[NSBundle mainBundle] privateFrameworksPath]
                stringByAppendingPathComponent:@"gGZLE01_recomp.dylib"];
            composite = [[NSFileManager defaultManager] fileExistsAtPath:inBundle] ? inBundle : inData;
        }
        // Mods (the Mods menu in BWGameOverlay.mm), applied at launch. The
        // widescreen code renders anamorphic 16:9 (or 16:10), so the picture
        // is letterboxed to that shape whatever the aspect setting.
        {
            NSUserDefaults* d = [NSUserDefaults standardUserDefaults];
            const BOOL movement = [d boolForKey:@"BlueWake.MovementExtras"];
            bw_default("BLUEWAKE_JUMP_BUTTON", movement ? @"1" : @"0");
            bw_default("BLUEWAKE_SPRINT_SPEED", movement ? @"1.5" : @"1");
            const BOOL fast = [d boolForKey:@"BlueWake.FastTransitions"];
            bw_default("BLUEWAKE_FADE_FRAMES", fast ? @"6" : @"0");
            bw_default("BLUEWAKE_FAST_FORWARD", fast ? @"1" : @"0");
            bw_default("BLUEWAKE_QUICK_DOORS", [d boolForKey:@"BlueWake.QuickDoors"] ? @"1" : @"0");
            NSMutableArray<NSString*>* mods = [NSMutableArray array];
            if ([d boolForKey:@"BlueWake.Mod.Widescreen1610"]) {
                [mods addObject:@"widescreen1610"];
                bw_default("DOL_AURORA_ASPECT_RATIO", @"1.6");
            } else if ([d boolForKey:@"BlueWake.Mod.Widescreen"]) {
                [mods addObject:@"widescreen"];
                bw_default("DOL_AURORA_ASPECT_RATIO", @"1.7778");
            }
            // HD textures: Dolphin-format packs go in the app data folder's
            // Load/Textures/GZLE01 (Dolphin's own layout).
            NSString* pack = [data stringByAppendingPathComponent:@"Load/Textures/GZLE01"];
            [[NSFileManager defaultManager] createDirectoryAtPath:pack withIntermediateDirectories:YES
                                                       attributes:nil error:nil];
            if ([d boolForKey:@"BlueWake.Mod.HDTextures"])
                bw_default("DOL_AURORA_TEXTURE_PACK", pack);
            // Better Wind Waker's settings (game options: mods/betterww/
            // options.txt, runtime/host/src/game_options.c): the mod carries
            // their code, and each setting the player changed from its default
            // in Mods > Better Wind Waker Settings is passed as name or -name.
            if ([d boolForKey:@"BlueWake.Mod.BetterWW"]) {
                // Keep previous personal modules usable during an app-only upgrade.
                // They need their exact patched disc; new option modules do not.
                void* lib = composite != nil ? dlopen(composite.fileSystemRepresentation, RTLD_LAZY) : NULL;
                typedef uint32_t (*CountFn)(void);
                CountFn count = lib != NULL ? (CountFn)dlsym(lib, "bluewake_composite_option_count") : NULL;
                const BOOL builtIn = count != NULL && count() > 0;
                if (lib != NULL) dlclose(lib);
                NSString* legacy = [data stringByAppendingPathComponent:@"Mods/betterww.iso"];
                if (builtIn) {
                    [mods addObject:@"betterww"];
                } else if ([bw_iso_dol_sha1(legacy) isEqualToString:@"e884a349a28ca534e272cf4c17737db587245cdd"]) {
                    [mods addObject:@"betterww"];
                    setenv("BLUEWAKE_DISC", legacy.fileSystemRepresentation, 1);
                    fprintf(stderr, "[mods] legacy Better Wind Waker disc; rebuild the personal module for individual options\n");
                } else {
                    fprintf(stderr, "[mods] Better Wind Waker not enabled: rebuild the personal module or restore its matching legacy disc\n");
                }
                NSMutableArray<NSString*>* options = [NSMutableArray array];
                for (NSString* key in [[d dictionaryRepresentation] allKeys]) {
                    if (![key hasPrefix:@BW_OPTION_KEY_PREFIX])
                        continue;
                    NSString* name = [key substringFromIndex:strlen(BW_OPTION_KEY_PREFIX)];
                    [options addObject:[d boolForKey:key] ? name : [@"-" stringByAppendingString:name]];
                }
                if (options.count > 0)
                    bw_default("BLUEWAKE_OPTIONS", [options componentsJoinedByString:@","]);
            }
            if (mods.count > 0)
                bw_default("BLUEWAKE_MODS", [mods componentsJoinedByString:@","]);
        }

        if (root == NULL || root[0] == '\0') {
            if (bluewake_first_run_needed(data.fileSystemRepresentation,
                                          composite.fileSystemRepresentation))
                bluewake_first_run_present(data.fileSystemRepresentation,
                                           composite.fileSystemRepresentation);
            bw_default_if_exists("BLUEWAKE_DOL",
                [data stringByAppendingPathComponent:@"main.dol"]);
            bw_default_if_exists("BLUEWAKE_RELS_DIR",
                [data stringByAppendingPathComponent:@"rels"]);
            bw_default_if_exists("BLUEWAKE_DISC",
                [data stringByAppendingPathComponent:@"GZLE01.iso"]);
            bw_default_if_exists("BLUEWAKE_DSP_IROM",
                [data stringByAppendingPathComponent:@"dsp_rom.bin"]);
            bw_default_if_exists("BLUEWAKE_DSP_COEF",
                [data stringByAppendingPathComponent:@"dsp_coef.bin"]);
#if TARGET_OS_TV
            bw_default("BLUEWAKE_CARD_PATH", [data stringByAppendingPathComponent:@"GZLE01.card"]);
#else
            NSString* support = [[NSSearchPathForDirectoriesInDomains(
                NSApplicationSupportDirectory, NSUserDomainMask, YES) firstObject]
                stringByAppendingPathComponent:@"BlueWake"];
            NSError* storageError = nil;
            BOOL ready = [NSFileManager.defaultManager createDirectoryAtPath:support
                withIntermediateDirectories:YES attributes:nil error:&storageError];
            NSString* card = [support stringByAppendingPathComponent:@"GZLE01.card"];
            NSString* legacy = [data stringByAppendingPathComponent:@"GZLE01.card"];
            // Keep the Documents copy; migration never consumes the player's file.
            ready = ready && bw_atomic_copy_if_missing(legacy.fileSystemRepresentation, card.fileSystemRepresentation);
            if (!ready) fprintf(stderr, "[card] migration failed; using preserved legacy card\n");
            bw_default("BLUEWAKE_CARD_PATH", ready ? card : legacy);
#endif
        }

        fprintf(stderr, "[ios] data=%s root=%s composite=%s\n",
                data.fileSystemRepresentation, root ? root : "(container)",
                composite ? composite.fileSystemRepresentation : "(host default)");

        char* host_argv[3] = {argv[0], NULL, NULL};
        int host_argc = 1;
        if (composite != nil) {
            host_argv[1] = strdup(composite.fileSystemRepresentation);
            host_argc = 2;
        }
        // Drawn in the game's own frame through Aurora's overlay hook.
#if !TARGET_OS_TV
        bluewake_touch_controls_install();
#endif
        const char* card_path = getenv("BLUEWAKE_CARD_PATH");
        if (card_path != NULL)
            bw_recover_card(@(card_path), [data stringByAppendingPathComponent:@"Backups"]);
        const int status = bluewake_host_main(host_argc, host_argv);
        // Returning from SDL's main leaves UIKit running with no game. The
        // host only returns at a bounded stop (BLUEWAKE_MAX_RETRACES), a quit
        // or a fatal error, and in each case the process should end.
        fflush(stdout);
        fflush(stderr);
        if (status != 0) {
            UIWindow* errorWindow = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
            for (UIScene* scene in UIApplication.sharedApplication.connectedScenes)
                if ([scene isKindOfClass:UIWindowScene.class]) { errorWindow.windowScene = (UIWindowScene*)scene; break; }
            errorWindow.windowLevel = UIWindowLevelAlert + 1;
            errorWindow.rootViewController = [UIViewController new];
            [errorWindow makeKeyAndVisible];
            UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"BlueWake Could Not Continue"
                message:@"Your saves have been kept. Check the session log in BlueWake’s Documents folder for the error."
                preferredStyle:UIAlertControllerStyleAlert];
            [alert addAction:[UIAlertAction actionWithTitle:@"Close BlueWake" style:UIAlertActionStyleDefault
                handler:^(UIAlertAction* action) { (void)action; _exit(status); }]];
            [errorWindow.rootViewController presentViewController:alert animated:YES completion:nil];
            for (;;) [NSRunLoop.currentRunLoop runMode:NSDefaultRunLoopMode
                beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
        }
        _exit(status);
    }
}
