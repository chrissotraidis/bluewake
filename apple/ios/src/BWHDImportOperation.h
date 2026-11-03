// Foundation-only lifetime/cancellation for one coordinated HD import.
// Header-only so the overlay and its standalone check use the same code.
#pragma once

#import <Foundation/Foundation.h>
#include <atomic>
#include <memory>

class BWHDImportOperation : public std::enable_shared_from_this<BWHDImportOperation> {
public:
    enum class Result { succeeded, cancelled, failed };

    // Setup, cancellation, progress admission and completion are main-thread-only.
    // Only the cancellation flag is shared with the synchronous importer.
    void cancel() {
        if (!finished_) {
            cancelled_ = true;
            [coordinator_ cancel];
        }
    }
    bool acceptsProgress() const { return !finished_ && !cancelled_.load(); }
    bool finished() const { return finished_; }

    void start(NSArray<NSFileAccessIntent*>* inputs, NSArray* grants,
               bool (^work)(NSError*, const std::atomic<bool>*), void (^done)(Result)) {
        NSCAssert(!started_, @"An HD import operation can only start once");
        started_ = true;
        grants_ = grants;
        auto operation = shared_from_this();
        if (cancelled_) {
            dispatch_async(dispatch_get_main_queue(), ^{ operation->finish(Result::cancelled, done); });
            return;
        }
        queue_ = [NSOperationQueue new];
        queue_.maxConcurrentOperationCount = 1;
        queue_.qualityOfService = NSQualityOfServiceUserInitiated;
        coordinator_ = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
        [coordinator_ coordinateAccessWithIntents:inputs queue:queue_ byAccessor:^(NSError* error) {
            if ([error.domain isEqualToString:NSCocoaErrorDomain] && error.code == NSUserCancelledError)
                operation->cancelled_ = true;
            const bool succeeded = !operation->cancelled_.load() && work(error, &operation->cancelled_);
            // A normal importer return means the pack was committed, even if
            // cancellation raced its final rename. Never relabel it cancelled.
            const Result result = succeeded ? Result::succeeded
                : operation->cancelled_.load() ? Result::cancelled : Result::failed;
            // This serial-queue operation runs only after the accessor returns:
            // coordination and every input read end before grants are released.
            [operation->queue_ addOperationWithBlock:^{
                dispatch_async(dispatch_get_main_queue(), ^{ operation->finish(result, done); });
            }];
        }];
    }

private:
    void finish(Result result, void (^done)(Result)) {
        finished_ = true;
        grants_ = nil;
        coordinator_ = nil;
        queue_ = nil;
        done(result);
    }
    std::atomic<bool> cancelled_{false};
    bool started_ = false, finished_ = false;
    NSArray* __strong grants_ = nil;
    NSFileCoordinator* __strong coordinator_ = nil;
    NSOperationQueue* __strong queue_ = nil;
};
