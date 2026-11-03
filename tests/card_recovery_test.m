// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#define _DARWIN_C_SOURCE 1
#import <Foundation/Foundation.h>
#include "atomic_file.h"
#include "gxruntime/memory_card.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int fail_replacement = 1;
static int injected_rename(const char* from, const char* to) {
    if (fail_replacement && strstr(from, ".recovery-")) { errno = EACCES; return -1; }
    return rename(from, to);
}
@interface BWCardRecovery : NSObject
@property(nonatomic, copy) NSString* card;
@property(nonatomic, copy) NSString* backups;
@property(nonatomic) BOOL finished;
- (void)replaceFrom:(NSString*)backup;
- (void)failure:(NSString*)message;
@end
@implementation BWCardRecovery
// Exact production replacement method; only the failure-alert UI is mocked.
#define rename injected_rename
#include "card_recovery_under_test.inc"
#undef rename
- (void)failure:(NSString*)message { fprintf(stderr, "mocked recovery alert: %s\n", message.UTF8String); }
@end
// Exact production startup early-return condition, with a boolean result
// in place of launching UIKit's alert loop. This does not exercise UIKit.
#include "card_recovery_guard_under_test.inc"
int main(void) {
    @autoreleasepool {
        char directory[] = "/tmp/bluewake-review-recovery-XXXXXX";
        assert(mkdtemp(directory));
        NSString* dir = @(directory);
        NSString* backup = [dir stringByAppendingPathComponent:@"backup.card"];
        NSString* card = [dir stringByAppendingPathComponent:@"GZLE01.card"];
        DolMemoryCardConfig config = {.path = backup.fileSystemRepresentation, .size_mbits = 4,
            .game_code = {'G','Z','L','E'}, .company = {'0','1'}};
        DolMemoryCard* handle = dol_card_open(&config);
        assert(handle && dol_card_mount(handle) == 0);
        s32 number;
        assert(dol_card_create_file(handle, "synthetic-progress", 8192, &number) == 0);
        dol_card_close(handle);
        assert([NSFileManager.defaultManager copyItemAtPath:backup toPath:card error:NULL]);
        FILE* file = fopen(card.fileSystemRepresentation, "r+b");
        assert(file && fseek(file, -1, SEEK_END) == 0);
        assert(fputc(0x43, file) != EOF && fclose(file) == 0);
        assert(recovery_offered(card, dir));
        NSData* damaged = [NSData dataWithContentsOfFile:card];
        BWCardRecovery* controller = [BWCardRecovery new];
        controller.card = card;
        controller.backups = dir;
        [controller replaceFrom:backup];
        assert(!controller.finished);
        BOOL offered = recovery_offered(card, dir);
        NSUInteger originals = 0;
        for (NSString* name in [NSFileManager.defaultManager contentsOfDirectoryAtPath:dir error:NULL])
            if ([name hasPrefix:@"GZLE01.card.corrupt-"]) {
                originals++;
                assert([[NSData dataWithContentsOfFile:[dir stringByAppendingPathComponent:name]] isEqualToData:damaged]);
            }
        printf("replacement failed: original preserved=%lu canonical exists=%d recovery on relaunch=%d\n",
            (unsigned long)originals, [NSFileManager.defaultManager fileExistsAtPath:card], offered);
        config.path = card.fileSystemRepresentation;
        handle = dol_card_open(&config);
        assert(handle == NULL); // The damaged canonical card remains; no empty replacement.
        assert(originals == 1 && offered);
        assert([[NSData dataWithContentsOfFile:card] isEqualToData:damaged]);
        fail_replacement = 0;
        [controller replaceFrom:backup];
        assert(controller.finished && dol_card_validate(card.fileSystemRepresentation));
        assert([[NSData dataWithContentsOfFile:card] isEqualToData:[NSData dataWithContentsOfFile:backup]]);
        controller.finished = NO;
        [controller replaceFrom:nil]; // Explicit player choice permits a fresh card.
        assert(controller.finished && ![NSFileManager.defaultManager fileExistsAtPath:card]);
        handle = dol_card_open(&config);
        assert(handle && dol_card_mount(handle) == 0);
        assert(dol_card_open_file(handle, "synthetic-progress", &number, NULL) == DOL_CARD_RESULT_NO_FILE);
        dol_card_close(handle);
        printf("synthetic fixture retained: %s\n", directory);
        return offered ? 0 : 1;
    }
}
