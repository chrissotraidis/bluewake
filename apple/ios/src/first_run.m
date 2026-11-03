// BlueWake first-run screen: shown before the game when this app container
// lacks something the host needs, and gone for good once everything is there.
//
// The disc image comes from the user through the system document picker (or
// Finder/Files sharing into Documents/BlueWake). main.dol and rels/ are made
// from it on the device by disc_import.c. The translated game code cannot be
// made on an iPad: it is compiled on a Mac from the same disc and embedded in
// the app, so the screen can only say when it is missing.
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "disc_import.h"
#include "first_run.h"

typedef NS_ENUM(NSInteger, BWItemState) { BWItemMissing, BWItemReady, BWItemBusy };

// One checklist row: a status symbol beside a name and a wrapping detail.
@interface BWRowView : UIStackView
@property(nonatomic, strong) UIImageView* icon;
@property(nonatomic, strong) UILabel* name;
@property(nonatomic, strong) UILabel* detail;
@end

@implementation BWRowView
- (instancetype)init {
    self = [super initWithFrame:CGRectZero];
    _icon = [[UIImageView alloc] init];
    _icon.contentMode = UIViewContentModeCenter;
    [_icon setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisHorizontal];
    [_icon.widthAnchor constraintEqualToConstant:28].active = YES;
    _name = [[UILabel alloc] init];
    _name.font = [UIFont systemFontOfSize:17 weight:UIFontWeightSemibold];
    _name.textColor = UIColor.whiteColor;
    _name.numberOfLines = 0;
    _detail = [[UILabel alloc] init];
    _detail.font = [UIFont systemFontOfSize:15];
    _detail.textColor = [UIColor colorWithWhite:1.0 alpha:0.66];
    _detail.numberOfLines = 0;
    UIStackView* text = [[UIStackView alloc] initWithArrangedSubviews:@[ _name, _detail ]];
    text.axis = UILayoutConstraintAxisVertical;
    text.spacing = 3;
    [self addArrangedSubview:_icon];
    [self addArrangedSubview:text];
    self.axis = UILayoutConstraintAxisHorizontal;
    self.alignment = UIStackViewAlignmentFirstBaseline;
    self.spacing = 10;
    return self;
}
@end

@interface BWFirstRunController : UIViewController <UIDocumentPickerDelegate>
@property(nonatomic, copy) NSString* dataDir;
@property(nonatomic, copy) NSString* compositePath;
@property(nonatomic) BOOL finished;
@end

@implementation BWFirstRunController {
    BWRowView* _codeRow;
    BWRowView* _discRow;
    BWRowView* _filesRow;
    UILabel* _subtitle;
    UILabel* _status;
    UIProgressView* _progress;
    UIButton* _choose;
    UIButton* _play;
    UIButton* _guide;
    BOOL _importing;
}

- (NSString*)discPath { return [self.dataDir stringByAppendingPathComponent:@"GZLE01.iso"]; }

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

static UILabel* BWLabel(CGFloat size, UIFontWeight weight, UIColor* color) {
    UILabel* label = [[UILabel alloc] init];
    label.font = [UIFont systemFontOfSize:size weight:weight];
    label.textColor = color;
    label.numberOfLines = 0;
    return label;
}

static UIButton* BWButton(NSString* title, BOOL prominent) {
    UIButtonConfiguration* config = prominent
        ? [UIButtonConfiguration filledButtonConfiguration]
        : [UIButtonConfiguration tintedButtonConfiguration];
    config.title = title;
    config.cornerStyle = UIButtonConfigurationCornerStyleLarge;
    config.buttonSize = UIButtonConfigurationSizeLarge;
    return [UIButton buttonWithConfiguration:config primaryAction:nil];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor colorWithRed:0.03 green:0.20 blue:0.36 alpha:1.0];
    self.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
    UIColor* soft = [UIColor colorWithWhite:1.0 alpha:0.72];

    UILabel* title = BWLabel(40, UIFontWeightBold, UIColor.whiteColor);
    title.text = @"BlueWake";
    UIImageView* icon = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"AppIcon76"]];
    icon.layer.cornerRadius = 16;
    icon.layer.masksToBounds = YES;
    [icon.widthAnchor constraintEqualToConstant:72].active = YES;
    [icon.heightAnchor constraintEqualToConstant:72].active = YES;
    UILabel* tagline = BWLabel(15, UIFontWeightSemibold, [UIColor colorWithRed:0.55 green:0.85 blue:1.0 alpha:1.0]);
    tagline.text = @"The Legend of Zelda: The Wind Waker, recompiled for iPad and iPhone. Experimental preview.";
    UIStackView* titleText = [[UIStackView alloc] initWithArrangedSubviews:@[ title, tagline ]];
    titleText.axis = UILayoutConstraintAxisVertical;
    titleText.spacing = 4;
    UIStackView* header = [[UIStackView alloc] initWithArrangedSubviews:@[ icon, titleText ]];
    header.spacing = 16;
    header.alignment = UIStackViewAlignmentCenter;
    UILabel* subtitle = BWLabel(17, UIFontWeightRegular, soft);
    _subtitle = subtitle;

    _codeRow = [[BWRowView alloc] init];
    _discRow = [[BWRowView alloc] init];
    _filesRow = [[BWRowView alloc] init];
    _status = BWLabel(15, UIFontWeightRegular, soft);
    _progress = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
    _progress.hidden = YES;

    _choose = BWButton(@"Choose Disc Image…", NO);
    [_choose addTarget:self action:@selector(chooseDisc) forControlEvents:UIControlEventPrimaryActionTriggered];
    _play = BWButton(@"Play", YES);
    [_play addTarget:self action:@selector(play) forControlEvents:UIControlEventPrimaryActionTriggered];
    // A copy without the game (the release's app, installed directly) can never play:
    // in place of Play, the steps for making a copy with the game in it.
    _guide = BWButton(@"How to Add the Game", YES);
    [_guide addAction:[UIAction actionWithHandler:^(__kindof UIAction* a) {
        (void)a;
        [UIApplication.sharedApplication openURL:[NSURL URLWithString:@"https://github.com/chrissotraidis/bluewake#iphone-and-ipad"]
                                         options:@{} completionHandler:nil];
    }] forControlEvents:UIControlEventPrimaryActionTriggered];

    UIStackView* buttons = [[UIStackView alloc] initWithArrangedSubviews:@[ _choose, _play, _guide ]];
    buttons.axis = UILayoutConstraintAxisHorizontal;
    buttons.spacing = 12;
    buttons.distribution = UIStackViewDistributionFillEqually;

    // Where to learn more and where to report problems.
    UIButton* (^link)(NSString*, NSString*, NSString*) = ^UIButton*(NSString* text, NSString* symbol, NSString* url) {
        UIButtonConfiguration* config = [UIButtonConfiguration plainButtonConfiguration];
        config.title = text;
        config.image = [UIImage systemImageNamed:symbol];
        config.imagePadding = 6;
        config.baseForegroundColor = soft;
        config.contentInsets = NSDirectionalEdgeInsetsMake(4, 0, 4, 0);
        return [UIButton buttonWithConfiguration:config primaryAction:[UIAction actionWithHandler:^(__kindof UIAction* a) {
            (void)a;
            [UIApplication.sharedApplication openURL:[NSURL URLWithString:url] options:@{} completionHandler:nil];
        }]];
    };
    UIStackView* links = [[UIStackView alloc] initWithArrangedSubviews:@[
        link(@"GitHub", @"chevron.left.forwardslash.chevron.right", @"https://github.com/chrissotraidis/bluewake"),
        link(@"Setup Guide", @"book", @"https://github.com/chrissotraidis/bluewake#readme"),
        link(@"Report a Problem", @"exclamationmark.bubble", @"https://github.com/chrissotraidis/bluewake/issues"),
    ]];
    links.spacing = 20;
    UILabel* legal = BWLabel(12, UIFontWeightRegular, [UIColor colorWithWhite:1.0 alpha:0.45]);
    legal.text = @"BlueWake is an unofficial fan project, not affiliated with or endorsed by Nintendo.";

    UIStackView* stack = [[UIStackView alloc] initWithArrangedSubviews:@[
        header, subtitle, _codeRow, _discRow, _filesRow, _progress, _status, buttons, links, legal
    ]];
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 18;
    [stack setCustomSpacing:28 afterView:subtitle];
    [stack setCustomSpacing:28 afterView:_status];
    [stack setCustomSpacing:28 afterView:buttons];
    [stack setCustomSpacing:6 afterView:links];
    stack.translatesAutoresizingMaskIntoConstraints = NO;

    UIScrollView* scroll = [[UIScrollView alloc] init];
    scroll.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:scroll];
    [scroll addSubview:stack];
    UILayoutGuide* safe = self.view.safeAreaLayoutGuide;
    NSLayoutConstraint* preferredWidth = [stack.widthAnchor constraintEqualToConstant:560];
    // Above the labels' compression resistance, so long lines wrap instead
    // of widening the column.
    preferredWidth.priority = UILayoutPriorityRequired - 1;
    // The content is at least one screen tall (so a short column stays
    // centred) and grows with the column, so short screens such as a phone in
    // landscape scroll instead of squeezing the rows together.
    UILayoutGuide* content = scroll.contentLayoutGuide;
    NSLayoutConstraint* screenTall = [content.heightAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.heightAnchor];
    screenTall.priority = UILayoutPriorityDefaultLow;
    [NSLayoutConstraint activateConstraints:@[
        [scroll.topAnchor constraintEqualToAnchor:safe.topAnchor],
        [scroll.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor],
        [scroll.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor],
        [scroll.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor],
        [content.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor],
        [content.heightAnchor constraintGreaterThanOrEqualToAnchor:scroll.frameLayoutGuide.heightAnchor],
        screenTall,
        [stack.centerXAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.centerXAnchor],
        [stack.widthAnchor constraintLessThanOrEqualToAnchor:scroll.frameLayoutGuide.widthAnchor constant:-48],
        preferredWidth,
        [stack.topAnchor constraintGreaterThanOrEqualToAnchor:content.topAnchor constant:32],
        [stack.bottomAnchor constraintLessThanOrEqualToAnchor:content.bottomAnchor constant:-32],
        [stack.centerYAnchor constraintEqualToAnchor:content.centerYAnchor],
    ]];
    [self refresh];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    // Test hook: import this path as if it had been picked.
    const char* hook = getenv("BLUEWAKE_IMPORT_DISC");
    if (hook != NULL && hook[0] != '\0' && !_importing && ![self hasDisc])
        [self importDiscAtURL:[NSURL fileURLWithPath:@(hook)] copy:YES];
    else if ([self hasDisc] && ![self hasPreparedFiles] && !_importing)
        [self prepareFromDisc];
}

static void BWRow(BWRowView* row, BWItemState state, NSString* name, NSString* detail) {
    NSString* symbol = state == BWItemReady ? @"checkmark.circle.fill"
                     : state == BWItemBusy  ? @"arrow.triangle.2.circlepath.circle"
                                            : @"circle";
    UIColor* tint = state == BWItemReady ? UIColor.systemGreenColor
                  : state == BWItemBusy  ? UIColor.systemYellowColor
                                         : [UIColor colorWithWhite:1.0 alpha:0.5];
    UIImageSymbolConfiguration* config =
        [UIImageSymbolConfiguration configurationWithPointSize:19 weight:UIImageSymbolWeightSemibold];
    row.icon.image = [[UIImage systemImageNamed:symbol withConfiguration:config]
        imageWithTintColor:tint renderingMode:UIImageRenderingModeAlwaysOriginal];
    row.name.text = name;
    row.detail.text = detail;
}

- (void)refresh {
    const BOOL code = [self hasComposite], disc = [self hasDisc], files = [self hasPreparedFiles];
    _subtitle.text = code
        ? @"BlueWake plays your own copy of the game: an uncompressed disc image of the US release "
           "(GZLE01). Nothing from the game ships with the app. It needs three things before it can start."
        : @"This copy of BlueWake doesn't have the game in it yet, so it can't play. The game's code can't "
           "be shared: each player makes their own from their disc with PadMint, a free app for a Mac with "
           "Apple silicon. PadMint builds a BlueWake with your game inside. Install that one over this one.";
    BWRow(_codeRow, code ? BWItemReady : BWItemMissing, @"Translated game code",
        code ? @"Built into this copy of BlueWake."
             : @"Not in this copy. PadMint makes it from your disc on a Mac.");
    BWRow(_discRow, disc ? BWItemReady : (_importing ? BWItemBusy : BWItemMissing),
        @"Disc image (GZLE01, USA)",
        disc ? @"Imported."
             : @"An uncompressed .iso or .gcm dump of your own disc. Choose it below, or put it in "
               "BlueWake's folder in the Files app as GZLE01.iso.");
    BWRow(_filesRow, files ? BWItemReady : (_importing ? BWItemBusy : BWItemMissing),
        @"Prepared game files", files ? @"Ready." : @"Made from the disc on this iPad in a few seconds.");
    _choose.enabled = !_importing;
    _play.enabled = !_importing && code && disc && files;
    _play.hidden = !code;
    _guide.hidden = code;
}

- (void)setStatus:(NSString*)text error:(BOOL)error {
    _status.text = text;
    _status.textColor = error ? UIColor.systemOrangeColor : [UIColor colorWithWhite:1.0 alpha:0.72];
}

- (void)chooseDisc {
    NSMutableArray<UTType*>* types = [NSMutableArray arrayWithObject:UTTypeData];
    UIDocumentPickerViewController* picker =
        [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:types asCopy:YES];
    picker.delegate = self;
    picker.allowsMultipleSelection = NO;
    [self presentViewController:picker animated:YES completion:nil];
}

- (void)documentPicker:(UIDocumentPickerViewController*)controller
    didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
    if (urls.count > 0)
        [self importDiscAtURL:urls.firstObject copy:NO];
}

// Checks the picked file, moves (or, for the test hook, clones) it into
// place as GZLE01.iso, then prepares main.dol and rels/ from it.
- (void)importDiscAtURL:(NSURL*)url copy:(BOOL)copy {
    char error[512] = "";
    if (bluewake_disc_check(url.fileSystemRepresentation, error, sizeof error) != 0) {
        if (!copy)
            [[NSFileManager defaultManager] removeItemAtURL:url error:nil];
        [self setStatus:[self pickError:url.lastPathComponent detail:@(error)] error:YES];
        return;
    }
    NSFileManager* fm = [NSFileManager defaultManager];
    NSURL* target = [NSURL fileURLWithPath:[self discPath]];
    [fm removeItemAtURL:target error:nil];
    NSError* moveError = nil;
    const BOOL ok = copy ? [fm copyItemAtURL:url toURL:target error:&moveError]
                         : [fm moveItemAtURL:url toURL:target error:&moveError];
    if (!ok) {
        [self setStatus:[NSString stringWithFormat:@"The disc image could not be saved: %@",
                                                   moveError.localizedDescription] error:YES];
        return;
    }
    [target setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:nil];
    [self prepareFromDisc];
}

// Why a picked file was refused, naming it, with the fix for the usual mix-ups:
// a compressed Dolphin image or an archive instead of the plain disc image.
- (NSString*)pickError:(NSString*)name detail:(NSString*)detail {
    NSString* extension = name.pathExtension.lowercaseString;
    NSString* text;
    if ([@[ @"rvz", @"wia", @"gcz", @"ciso" ] containsObject:extension])
        text = [NSString stringWithFormat:@"\u201C%@\u201D is a compressed Dolphin image. Convert it to an ISO "
                                          "first: in Dolphin, right-click the game, choose Convert File\u2026 and "
                                          "pick ISO. Then choose the .iso here.", name];
    else if ([@[ @"zip", @"7z", @"rar" ] containsObject:extension])
        text = [NSString stringWithFormat:@"\u201C%@\u201D is an archive. Unzip it first, then choose the .iso "
                                          "inside.", name];
    else
        text = [NSString stringWithFormat:@"\u201C%@\u201D: %@", name, detail];
    return [self hasDisc] ? [text stringByAppendingString:@" The disc already imported is unchanged."] : text;
}

static void BWProgress(void* context, double fraction, const char* stage) {
    BWFirstRunController* controller = (__bridge BWFirstRunController*)context;
    NSString* text = @(stage);
    dispatch_async(dispatch_get_main_queue(), ^{
        [controller->_progress setProgress:(float)fraction animated:YES];
        [controller setStatus:text error:NO];
    });
}

- (void)prepareFromDisc {
    _importing = YES;
    _progress.hidden = NO;
    _progress.progress = 0;
    [self setStatus:@"Preparing game files…" error:NO];
    [self refresh];
    NSString* disc = [self discPath];
    NSString* dir = self.dataDir;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        char error[512] = "";
        const int rc = bluewake_disc_prepare(disc.fileSystemRepresentation, dir.fileSystemRepresentation,
                                             BWProgress, (__bridge void*)self, error, sizeof error);
        NSString* message = @(error);
        dispatch_async(dispatch_get_main_queue(), ^{
            self->_importing = NO;
            self->_progress.hidden = YES;
            if (rc != 0) {
                // A disc that fails preparation is not kept.
                [[NSFileManager defaultManager] removeItemAtPath:disc error:nil];
                [self setStatus:message error:YES];
                fprintf(stderr, "[first-run] preparation failed: %s\n", message.UTF8String);
            } else {
                for (NSString* name in @[ @"main.dol", @"rels" ])
                    [[NSURL fileURLWithPath:[dir stringByAppendingPathComponent:name]]
                        setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:nil];
                fprintf(stderr, "[first-run] disc prepared\n");
                [self setStatus:([self hasComposite] ? @"Ready." : @"The disc is ready. The game code is still missing.")
                          error:NO];
            }
            [self refresh];
            if (rc == 0 && [self hasComposite])
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.6 * NSEC_PER_SEC)),
                               dispatch_get_main_queue(), ^{ [self play]; });
        });
    });
}

- (void)play {
    if (_play.enabled)
        self.finished = YES;
}

@end

bool bluewake_first_run_needed(const char* data_dir, const char* composite_path) {
    BWFirstRunController* probe = [[BWFirstRunController alloc] init];
    probe.dataDir = @(data_dir);
    probe.compositePath = composite_path != NULL ? @(composite_path) : nil;
    return ![probe hasComposite] || ![probe hasDisc] || ![probe hasPreparedFiles];
}

void bluewake_first_run_present(const char* data_dir, const char* composite_path) {
    BWFirstRunController* controller = [[BWFirstRunController alloc] init];
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
    fprintf(stderr, "[first-run] shown: code=%d disc=%d files=%d\n", [controller hasComposite],
            [controller hasDisc], [controller hasPreparedFiles]);

    // SDL calls our main from inside UIKit's run loop; keep it turning until
    // the player taps Play (or the import finishes and starts the game).
    while (!controller.finished) {
        @autoreleasepool {
            [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                     beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
        }
    }
    window.hidden = YES;
    window.rootViewController = nil;
    fprintf(stderr, "[first-run] done\n");
}
