// Standalone macOS Foundation check of the iOS import operation, not UIKit or
// a Files provider. Only generated local files and synthetic grant owners.
#include "BWHDImportOperation.h"
#include <cstdio>
#include <cstdlib>

static std::atomic<int> released{0};
static void require(bool ok, const char* why) {
    if (!ok) { fprintf(stderr, "FAIL: %s\n", why); exit(1); }
}
static void until(bool (^condition)(void)) {
    NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:5];
    while (!condition() && deadline.timeIntervalSinceNow > 0)
        [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.005]];
    require(condition(), "five-second operation timeout");
}
@interface Grant : NSObject @end
@implementation Grant
- (void)dealloc { ++released; }
@end

// Holds a real NSFileCoordinator request before its accessor can begin.
@interface Provider : NSObject <NSFilePresenter>
@property(nonatomic, copy) NSURL* presentedItemURL;
@property(nonatomic, strong) NSOperationQueue* presentedItemOperationQueue;
@property(nonatomic, copy) void (^reply)(NSError*);
@end
@implementation Provider
- (void)savePresentedItemChangesWithCompletionHandler:(void (^)(NSError*))completion {
    dispatch_async(dispatch_get_main_queue(), ^{ self.reply = completion; });
}
@end

enum class Case { success, error, beforeStart, providerWait, conversion, committed };
static void run(Case mode, NSURL* file) {
    released = 0;
    auto operation = std::make_shared<BWHDImportOperation>();
    const std::weak_ptr<BWHDImportOperation> weakOperation = operation;
    __block BOOL entered = NO, done = NO;
    __block int completions = 0, progress = 0;
    __block BWHDImportOperation::Result result = BWHDImportOperation::Result::failed;
    dispatch_semaphore_t resume = dispatch_semaphore_create(0);
    Provider* provider = nil;
    if (mode == Case::error || mode == Case::providerWait) {
        provider = [Provider new];
        provider.presentedItemURL = file;
        provider.presentedItemOperationQueue = [NSOperationQueue new];
        [NSFileCoordinator addFilePresenter:provider];
    }
    if (mode == Case::beforeStart) operation->cancel();
    @autoreleasepool {
        NSArray* grants = @[ [Grant new] ];
        NSArray* inputs = @[ [NSFileAccessIntent readingIntentWithURL:file options:0] ];
        operation->start(inputs, grants, ^bool(NSError* error, const std::atomic<bool>* cancel) {
            require(![NSThread isMainThread], "accessor must run off main");
            if (error != nil) return false;
            require([[NSData dataWithContentsOfURL:((NSFileAccessIntent*)inputs[0]).URL]
                     isEqualToData:[@"SYNTHETIC" dataUsingEncoding:NSUTF8StringEncoding]], "coordinated input read");
            if (mode == Case::conversion || mode == Case::committed) {
                dispatch_async(dispatch_get_main_queue(), ^{ entered = YES; });
                require(dispatch_semaphore_wait(resume, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC)) == 0,
                        "worker resume timeout");
                require(cancel->load(), "conversion sees the existing atomic token");
                // Model already queued progress arriving after Cancel.
                dispatch_async(dispatch_get_main_queue(), ^{
                    if (operation->acceptsProgress()) ++progress;
                });
                require(released == 0, "grants survive cancellation and worker cleanup");
                return mode == Case::committed; // A completed commit wins late Cancel.
            }
            return true;
        }, ^(BWHDImportOperation::Result value) {
            require([NSThread isMainThread], "completion on main");
            require(released == 1, "grant released only after accessor ends");
            require(operation->finished() && !operation->acceptsProgress(), "finished state rejects progress");
            result = value;
            ++completions;
            done = YES;
        });
    }
    if (provider != nil) {
        until(^bool { return provider.reply != nil; });
        require(released == 0 && !done, "provider wait retains grant and remains active");
        if (mode == Case::error) {
            provider.reply([NSError errorWithDomain:@"SyntheticFailure" code:1 userInfo:nil]);
            provider.reply = nil;
        } else {
            operation->cancel();
            // A cancelled coordinator must finish even while the provider has
            // not answered. This exercises the actual asynchronous cancel API.
            until(^bool { return done; });
            provider.reply(nil);
            provider.reply = nil;
        }
    }
    if (mode == Case::conversion || mode == Case::committed) {
        until(^bool { return entered; });
        operation->cancel();
        operation->cancel();
        require(!operation->finished() && !operation->acceptsProgress() && released == 0,
                "Cancel neither finishes worker nor releases grants");
        dispatch_semaphore_signal(resume);
    }
    until(^bool { return done; });
    require(completions == 1 && progress == 0, "one completion and no delayed cancelled progress");
    const auto expected = mode == Case::success || mode == Case::committed
        ? BWHDImportOperation::Result::succeeded : mode == Case::error
        ? BWHDImportOperation::Result::failed : BWHDImportOperation::Result::cancelled;
    require(result == expected, "authoritative terminal result");
    operation->cancel(); // No change to an already delivered outcome.
    if (provider != nil) [NSFileCoordinator removeFilePresenter:provider];
    operation.reset();
    until(^bool { return weakOperation.expired(); });
}

int main(int argc, char** argv) {
    @autoreleasepool {
        require(argc == 2, "expected ignored build directory");
        NSURL* file = [[NSURL fileURLWithPath:@(argv[1]) isDirectory:YES]
            URLByAppendingPathComponent:NSUUID.UUID.UUIDString];
        require([[@"SYNTHETIC" dataUsingEncoding:NSUTF8StringEncoding] writeToURL:file atomically:YES], "create fixture");
        for (Case mode : {Case::success, Case::error, Case::beforeStart, Case::providerWait, Case::conversion, Case::committed})
            run(mode, file);
        require([[NSFileManager defaultManager] removeItemAtURL:file error:nil], "remove fixture");
        puts("PASS: success, coordination error, pre-start/provider/conversion cancellation, late commit; grants, progress and completion lifetime");
    }
}
