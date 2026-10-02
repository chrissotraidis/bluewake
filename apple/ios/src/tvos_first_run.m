// Apple TV first-run screen. The tvOS app has no Files app or document picker,
// so a disc image is copied into its app data container from the Mac with
// devicectl. When the file arrives, this screen prepares it and starts the game.
#import <UIKit/UIKit.h>

#include <stdio.h>

#include "disc_import.h"
#include "first_run.h"

@interface BWTVFirstRunController : UIViewController
@property(nonatomic, copy) NSString* dataDir;
@property(nonatomic, copy) NSString* compositePath;
@property(nonatomic) BOOL finished;
- (void)stopPolling;
@end

@implementation BWTVFirstRunController {
    UILabel* _status;
    UIProgressView* _progress;
    UIButton* _retry;
    BOOL _importing;
    BOOL _importFailed;
    NSTimer* _poll;
    unsigned long long _lastDiscSize;
    NSDate* _lastDiscModification;
    NSDate* _discStableSince;
}

- (NSString*)discPath {
    return [self.dataDir stringByAppendingPathComponent:@"GZLE01.iso"];
}

- (BOOL)hasComposite {
    return self.compositePath != nil &&
           [[NSFileManager defaultManager] fileExistsAtPath:self.compositePath];
}

- (BOOL)hasDisc {
    return [[NSFileManager defaultManager] fileExistsAtPath:[self discPath]];
}

- (BOOL)hasPreparedFiles {
    NSFileManager* fm = [NSFileManager defaultManager];
    NSString* rels = [self.dataDir stringByAppendingPathComponent:@"rels"];
    return [fm fileExistsAtPath:[self.dataDir stringByAppendingPathComponent:@"main.dol"]] &&
           [[fm contentsOfDirectoryAtPath:rels error:nil] count] >= 400;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor colorWithRed:0.03 green:0.20 blue:0.36 alpha:1.0];
    self.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;

    UIStackView* stack = [[UIStackView alloc] init];
    stack.axis = UILayoutConstraintAxisVertical;
    stack.alignment = UIStackViewAlignmentLeading;
    stack.spacing = 22;
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:stack];

    UILabel* title = [[UILabel alloc] init];
    title.text = @"BlueWake";
    title.font = [UIFont systemFontOfSize:48 weight:UIFontWeightBold];
    title.textColor = UIColor.whiteColor;
    [stack addArrangedSubview:title];

    UILabel* detail = [[UILabel alloc] init];
    detail.numberOfLines = 0;
    detail.font = [UIFont systemFontOfSize:25];
    detail.textColor = [UIColor colorWithWhite:1.0 alpha:0.82];
    detail.text = @"This personal build uses game code made from your own GZLE01 USA revision 0 disc.\n\nCopy GZLE01.iso from the Mac into this app's Library/Caches/BlueWake folder. The app will prepare the game files and start automatically. tvOS may clear cached data, so keep a backup of your disc image and save card.\n\nFrom the Mac, use: xcrun devicectl device copy to --device <Apple TV> --domain-type appDataContainer --domain-identifier dev.bluewake.BlueWake --source /path/to/GZLE01.iso --destination 'Library/Caches/BlueWake/GZLE01.iso'";
    [stack addArrangedSubview:detail];

    _status = [[UILabel alloc] init];
    _status.numberOfLines = 0;
    _status.font = [UIFont systemFontOfSize:22 weight:UIFontWeightMedium];
    _status.textColor = [UIColor colorWithWhite:1.0 alpha:0.72];
    [stack addArrangedSubview:_status];

    _progress = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
    _progress.hidden = YES;
    [stack addArrangedSubview:_progress];

    _retry = [UIButton buttonWithType:UIButtonTypeSystem];
    _retry.titleLabel.font = [UIFont systemFontOfSize:23 weight:UIFontWeightSemibold];
    [_retry setTitle:@"Check for disc" forState:UIControlStateNormal];
    [_retry addTarget:self action:@selector(retryImport) forControlEvents:UIControlEventPrimaryActionTriggered];
    [stack addArrangedSubview:_retry];

    [NSLayoutConstraint activateConstraints:@[
        [stack.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor constant:110],
        [stack.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor constant:-110],
        [stack.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
        [_progress.widthAnchor constraintEqualToAnchor:stack.widthAnchor],
    ]];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self checkForDisc];
    _poll = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self
                                           selector:@selector(pollForDisc:)
                                           userInfo:nil repeats:YES];
}

- (void)pollForDisc:(NSTimer*)timer {
    (void)timer;
    [self checkForDisc];
}

- (void)retryImport {
    if (_importing) return;
    _importFailed = NO;
    _discStableSince = nil;
    [self checkForDisc];
}

- (void)checkForDisc {
    if (![self hasComposite]) {
        _status.text = @"Translated game code is missing. Rebuild and install BlueWake from the Mac.";
        return;
    }
    if (![self hasDisc]) {
        _lastDiscSize = 0;
        _lastDiscModification = nil;
        _discStableSince = nil;
        _importFailed = NO;
        _status.text = @"Waiting for GZLE01.iso. The screen checks automatically when the transfer finishes.";
        return;
    }
    if ([self hasPreparedFiles]) {
        self.finished = YES;
        return;
    }
    if (!_importing) {
        NSDictionary* attributes = [[NSFileManager defaultManager] attributesOfItemAtPath:[self discPath]
                                                                                     error:nil];
        const unsigned long long size = [attributes[NSFileSize] unsignedLongLongValue];
        NSDate* modification = attributes[NSFileModificationDate];
        if (_discStableSince == nil || size != _lastDiscSize ||
            ![_lastDiscModification isEqualToDate:modification]) {
            _lastDiscSize = size;
            _lastDiscModification = modification;
            _discStableSince = [NSDate date];
            _importFailed = NO;
            _status.text = @"Waiting for the disc image transfer to finish…";
            return;
        }
        if ([[NSDate date] timeIntervalSinceDate:_discStableSince] < 3.0)
            return;
        // Keep a failed image intact, and avoid retrying it every second.
        // A changed file or the focused Retry button starts a new attempt.
        if (_importFailed) return;
        [self prepareFromDisc];
    }
}

static void BWTVProgress(void* context, double fraction, const char* stage) {
    BWTVFirstRunController* controller = (__bridge BWTVFirstRunController*)context;
    NSString* message = stage != NULL ? @(stage) : @"Preparing game files…";
    dispatch_async(dispatch_get_main_queue(), ^{
        [controller->_progress setProgress:(float)fraction animated:YES];
        controller->_status.text = message;
    });
}

- (void)prepareFromDisc {
    _importing = YES;
    _retry.enabled = NO;
    _progress.hidden = NO;
    _progress.progress = 0;
    _status.text = @"Preparing game files…";
    NSString* disc = [self discPath];
    NSString* dir = self.dataDir;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        char error[512] = "";
        const int rc = bluewake_disc_prepare(disc.fileSystemRepresentation, dir.fileSystemRepresentation,
                                             BWTVProgress, (__bridge void*)self, error, sizeof error);
        NSString* message = @(error);
        dispatch_async(dispatch_get_main_queue(), ^{
            self->_importing = NO;
            self->_retry.enabled = YES;
            self->_progress.hidden = YES;
            if (rc != 0) {
                self->_importFailed = YES;
                self->_status.text = message;
                fprintf(stderr, "[first-run] preparation failed: %s\n", message.UTF8String);
            } else {
                for (NSString* name in @[ @"main.dol", @"rels" ])
                    [[NSURL fileURLWithPath:[dir stringByAppendingPathComponent:name]]
                        setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:nil];
                self->_status.text = @"Disc ready. Starting BlueWake…";
                fprintf(stderr, "[first-run] disc prepared\n");
                self.finished = YES;
            }
        });
    });
}

- (void)stopPolling {
    [_poll invalidate];
    _poll = nil;
}

@end

bool bluewake_first_run_needed(const char* data_dir, const char* composite_path) {
    BWTVFirstRunController* probe = [[BWTVFirstRunController alloc] init];
    probe.dataDir = @(data_dir);
    probe.compositePath = composite_path != NULL ? @(composite_path) : nil;
    return ![probe hasComposite] || ![probe hasDisc] || ![probe hasPreparedFiles];
}

void bluewake_first_run_present(const char* data_dir, const char* composite_path) {
    BWTVFirstRunController* controller = [[BWTVFirstRunController alloc] init];
    controller.dataDir = @(data_dir);
    controller.compositePath = composite_path != NULL ? @(composite_path) : nil;

    UIWindowScene* scene = nil;
    for (UIScene* candidate in UIApplication.sharedApplication.connectedScenes)
        if ([candidate isKindOfClass:UIWindowScene.class]) {
            scene = (UIWindowScene*)candidate;
            break;
        }
    UIWindow* window = scene != nil ? [[UIWindow alloc] initWithWindowScene:scene]
                                    : [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    window.windowLevel = UIWindowLevelNormal + 1;
    window.rootViewController = controller;
    [window makeKeyAndVisible];
    fprintf(stderr, "[first-run] tvOS shown: code=%d disc=%d files=%d\n", [controller hasComposite],
            [controller hasDisc], [controller hasPreparedFiles]);

    while (!controller.finished) {
        @autoreleasepool {
            [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                     beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
        }
    }
    [controller stopPolling];
    window.hidden = YES;
    window.rootViewController = nil;
    fprintf(stderr, "[first-run] tvOS done\n");
}
