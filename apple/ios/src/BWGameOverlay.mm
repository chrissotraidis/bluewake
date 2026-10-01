extern "C" {
#include "card_runtime.h"
#include "process_close.h"
}
// BlueWake's mobile shell over the game view: touch controls, the three-dot
// menu, touch settings and the layout editor.
//
// Adapted from SunPad's apple/ios/SunPadGameOverlay.mm (GPL-3.0, commit
// e43f0ea6b797e5110787171957c9dc3c6213269c); see docs/SUNPAD_TRANSFER.md for
// what was kept and what was left out. It keeps SunPad's controls, colors,
// layout defaults, sparse per-control persistence, grouped D-pad editing and
// controller/touch coexistence. It drops Sunshine's FLUDD analog R trigger,
// its water animation, its render-scale and 60 FPS settings and its data
// importer (BlueWake imports on first run, first_run.m). Pad state and pause
// reasons go through touch_controls.cpp.
#import <AVFoundation/AVFoundation.h>
#import <GameController/GameController.h>
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <sys/utsname.h>
#include <stdio.h>

#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <cmath>

#include "controller_settings.h"
#include "dolphin_save_import.h"
#include "touch_controls.h"

#define BW_GITHUB_URL "https://github.com/chrissotraidis/bluewake"

// The game options the game module offers (runtime/host/src/game_options.h):
// Better Wind Waker's settings.
extern "C" const char* bluewake_game_options_describe(uint32_t position, const char** title, bool* default_on,
                                                      bool* on);

// SDL's own entry points for a hardware keyboard (src/events/SDL_keyboard_c.h),
// linked from the static SDL; used only by the BLUEWAKE_KEY_TAPS test hook.
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_timer.h>
extern "C" {
void SDL_AddKeyboard(SDL_KeyboardID keyboardID, const char* name);
bool SDL_SendKeyboardKey(Uint64 timestamp, SDL_KeyboardID keyboardID, int rawcode,
                         SDL_Scancode scancode, bool down);
}

// ---------------------------------------------------------------- settings

static NSString* const kOpacityKey = @"BlueWake.ControlOpacity";
static NSString* const kSizeKey = @"BlueWake.ControlSize";
static NSString* const kHideOnControllerKey = @"BlueWake.HideOnController";
static NSString* const kShowTouchKey = @"BlueWake.ShowTouchControls";
static NSString* const kShowFPSKey = @"BlueWake.ShowFPS";
static NSString* const kMovementExtrasKey = @"BlueWake.MovementExtras";
NSString* const BWAspectModeKey = @"BlueWake.AspectMode";  // read at launch

static BOOL BWIsPhone(UIView* view) {
    return view.traitCollection.userInterfaceIdiom == UIUserInterfaceIdiomPhone;
}

// Layouts are per form factor and schema version, so a phone and a tablet
// keep independent arrangements (PRD FR-015).
static NSString* BWLayoutKey(UIView* view, NSString* name) {
    return [NSString stringWithFormat:@"BlueWake.%@.v1.%@", BWIsPhone(view) ? @"phone" : @"tablet",
                                      name];
}

static double BWDefault(NSString* key, double fallback) {
    NSNumber* value = [[NSUserDefaults standardUserDefaults] objectForKey:key];
    return value != nil ? value.doubleValue : fallback;
}

static BOOL BWBoolDefault(NSString* key, BOOL fallback) {
    NSNumber* value = [[NSUserDefaults standardUserDefaults] objectForKey:key];
    return value != nil ? value.boolValue : fallback;
}

static CGRect BWFrameAtNormalizedCenter(CGRect safe, CGFloat x, CGFloat y, CGFloat w, CGFloat h) {
    return CGRectMake(CGRectGetMinX(safe) + x * safe.size.width - w * 0.5,
                      CGRectGetMinY(safe) + y * safe.size.height - h * 0.5, w, h);
}

// ---------------------------------------------------------------- stick

@interface BWStickView : UIView
@property(nonatomic, copy) void (^valueChanged)(float x, float y);
- (void)applyBaseColor:(UIColor*)base thumbColor:(UIColor*)thumb;
- (void)reset;
@end

@implementation BWStickView {
    UIView* _thumb;
    float _x, _y;
}

- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        self.multipleTouchEnabled = NO;
        self.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.34].CGColor;
        self.layer.borderWidth = 2.0;
        _thumb = [[UIView alloc] initWithFrame:CGRectZero];
        _thumb.userInteractionEnabled = NO;
        [self addSubview:_thumb];
        self.isAccessibilityElement = YES;
        self.accessibilityTraits = UIAccessibilityTraitAllowsDirectInteraction;
    }
    return self;
}

- (void)layoutSubviews {
    [super layoutSubviews];
    CGFloat side = std::min(self.bounds.size.width, self.bounds.size.height);
    self.layer.cornerRadius = side * 0.5;
    CGFloat thumb = side * 0.42;
    _thumb.bounds = CGRectMake(0, 0, thumb, thumb);
    _thumb.layer.cornerRadius = thumb * 0.5;
    [self placeThumb];
}

- (void)placeThumb {
    CGFloat half = self.bounds.size.width * 0.5;
    CGFloat travel = half - _thumb.bounds.size.width * 0.5 - 3.0;
    _thumb.center = CGPointMake(half + _x * travel, half - _y * travel);
}

- (void)applyBaseColor:(UIColor*)base thumbColor:(UIColor*)thumb {
    self.backgroundColor = base;
    _thumb.backgroundColor = thumb;
}

- (void)reset {
    _x = _y = 0.0f;
    [self placeThumb];
    if (self.valueChanged)
        self.valueChanged(0.0f, 0.0f);
}

- (void)handleTouch:(UITouch*)touch {
    CGPoint p = [touch locationInView:self];
    CGPoint c = CGPointMake(CGRectGetMidX(self.bounds), CGRectGetMidY(self.bounds));
    CGFloat radius = std::max<CGFloat>(1.0, std::min(self.bounds.size.width, self.bounds.size.height) * 0.5);
    CGFloat dx = (p.x - c.x) / radius, dy = (p.y - c.y) / radius;
    CGFloat length = hypot(dx, dy);
    if (length > 1.0) {
        dx /= length;
        dy /= length;
    }
    _x = (float)dx;
    _y = (float)-dy;  // +y is up
    [self placeThumb];
    if (self.valueChanged)
        self.valueChanged(_x, _y);
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event { [self handleTouch:touches.anyObject]; }
- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event { [self handleTouch:touches.anyObject]; }
- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event { [self reset]; }
- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event { [self reset]; }
@end

// ---------------------------------------------------------------- buttons

@interface BWGameButton : UIButton
@property(nonatomic) uint16_t inputMask;
@end
@implementation BWGameButton
@end

// The ellipsis button; the game is held while its menu is on screen.
@interface BWMenuButton : UIButton
@end
@implementation BWMenuButton
- (void)contextMenuInteraction:(UIContextMenuInteraction*)interaction
    willDisplayMenuForConfiguration:(UIContextMenuConfiguration*)configuration
                           animator:(id<UIContextMenuInteractionAnimating>)animator {
    [super contextMenuInteraction:interaction willDisplayMenuForConfiguration:configuration animator:animator];
    bluewake_touch_clear();
    bluewake_pause_set(BLUEWAKE_PAUSE_MENU, true);
}
- (void)contextMenuInteraction:(UIContextMenuInteraction*)interaction
       willEndForConfiguration:(UIContextMenuConfiguration*)configuration
                      animator:(id<UIContextMenuInteractionAnimating>)animator {
    [super contextMenuInteraction:interaction willEndForConfiguration:configuration animator:animator];
    bluewake_pause_set(BLUEWAKE_PAUSE_MENU, false);
}
@end

// Taps outside a control fall through to the game view.
@interface BWPassThroughView : UIView
@end
@implementation BWPassThroughView
- (UIView*)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
    UIView* hit = [super hitTest:point withEvent:event];
    return hit == self ? nil : hit;
}
@end

// ---------------------------------------------------------------- overlay

@interface BWGameOverlay : BWPassThroughView <UIGestureRecognizerDelegate, UIDocumentPickerDelegate>
- (void)showSettingsPanel;
- (void)hideSettingsPanel;
- (void)beginLayoutEditing;
- (void)endLayoutEditing;
- (void)tapIdentifier:(NSString*)identifier holdSeconds:(double)hold;
@end

static void BWDumpMenu(UIMenuElement* element, int depth) {
    NSString* state = @"";
    NSString* subtitle = @"";
    if ([element isKindOfClass:[UIAction class]]) {
        UIAction* action = (UIAction*)element;
        state = action.state == UIMenuElementStateOn ? @" [on]" : @" [off]";
        subtitle = action.subtitle ?: @"";
    } else if ([element isKindOfClass:[UIMenu class]]) {
        subtitle = ((UIMenu*)element).subtitle ?: @"";
    }
    fprintf(stderr, "[menu] %*s%s%s%s%s\n", depth * 2, "", element.title.UTF8String, state.UTF8String,
            subtitle.length ? " - " : "", subtitle.UTF8String);
    if ([element isKindOfClass:[UIMenu class]])
        for (UIMenuElement* child in ((UIMenu*)element).children)
            BWDumpMenu(child, depth + 1);
}

@implementation BWGameOverlay {
    BWMenuButton* _menuButton;
    BWStickView* _moveStick;
    BWStickView* _cStick;
    UIView* _dpadGroup;
    NSMutableArray<BWGameButton*>* _buttons;
    NSMutableArray<UIGestureRecognizer*>* _editGestures;

    UIView* _settingsPanel;
    UISlider* _opacitySlider;
    UISlider* _sizeSlider;
    UISwitch* _hideSwitch;
    UISwitch* _editSwitch;
    UIView* _editorBar;
    UILabel* _editorHint;
    UISlider* _selectedSizeSlider;
    __weak UIView* _selected;

    BlueWakeTouchPad _pad;
    BOOL _controllerHidden;
    BOOL _editing;
    NSInteger _launchAspectMode;               // the aspect the renderer started with
    NSDictionary<NSString*, NSValue*>* _columnFrames;  // phone letterbox defaults, if they apply
    UILabel* _fpsLabel;
    NSTimer* _fpsTimer;
    NSDictionary<NSString*, NSNumber*>* _launchMods;  // mod switches as this launch read them
    void (^_pickHandler)(NSArray<NSURL*>* urls);      // the open file picker's handler
}

- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        self.accessibilityIdentifier = @"BlueWakeOverlay";
        _launchAspectMode = (NSInteger)BWDefault(BWAspectModeKey, 0);
        NSUserDefaults* d = [NSUserDefaults standardUserDefaults];
        _launchMods = @{
            @BW_MOD_WIDESCREEN_KEY : @([d boolForKey:@BW_MOD_WIDESCREEN_KEY]),
            @BW_MOD_WIDESCREEN_1610_KEY : @([d boolForKey:@BW_MOD_WIDESCREEN_1610_KEY]),
            @BW_MOD_HD_TEXTURES_KEY : @([d boolForKey:@BW_MOD_HD_TEXTURES_KEY]),
            @BW_MOD_BETTERWW_KEY : @([d boolForKey:@BW_MOD_BETTERWW_KEY]),
        };
        _buttons = [NSMutableArray array];
        _editGestures = [NSMutableArray array];
        [self buildTouchControls];
        [self buildMenuButton];
        [self buildSettingsPanel];
        [self buildFPSLabel];
        [self observeLifecycle];
        [self applyControllerVisibility];
    }
    return self;
}

- (void)dealloc {
    [_fpsTimer invalidate];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

// ---------------------------------------------------------------- FPS display

// Frames shown per second, game speed and the longest frame gap in the last
// second (touch_controls.cpp), shown top left when Display > Show FPS is on.
- (void)buildFPSLabel {
    _fpsLabel = [UILabel new];
    _fpsLabel.font = [UIFont monospacedDigitSystemFontOfSize:13.0 weight:UIFontWeightSemibold];
    _fpsLabel.textColor = UIColor.whiteColor;
    _fpsLabel.backgroundColor = [UIColor colorWithWhite:0.0 alpha:0.55];
    _fpsLabel.layer.cornerRadius = 8.0;
    _fpsLabel.layer.masksToBounds = YES;
    _fpsLabel.textAlignment = NSTextAlignmentCenter;
    _fpsLabel.userInteractionEnabled = NO;
    _fpsLabel.accessibilityIdentifier = @"FPS";
    [self addSubview:_fpsLabel];
    [self applyFPSVisibility];
}

- (void)applyFPSVisibility {
    const BOOL on = BWBoolDefault(kShowFPSKey, NO);
    _fpsLabel.hidden = !on;
    [_fpsTimer invalidate];
    _fpsTimer = nil;
    if (!on) return;
    __weak BWGameOverlay* weakSelf = self;
    _fpsTimer = [NSTimer scheduledTimerWithTimeInterval:0.5 repeats:YES block:^(NSTimer* timer) {
        (void)timer;
        [weakSelf updateFPSLabel];
    }];
    [self updateFPSLabel];
}

- (void)updateFPSLabel {
    float shown = 0, speed = 0, worst = 0;
    bluewake_fps_read(&shown, &speed, &worst);
    const float display = bluewake_fps_display();
    if (display > shown + 5.0f)
        _fpsLabel.text = [NSString stringWithFormat:@"%.0f FPS (game %.0f) · %.0f%% speed · %.0f ms", display, shown,
                                                    speed, worst];
    else
        _fpsLabel.text = [NSString stringWithFormat:@"%.0f FPS · %.0f%% speed · %.0f ms", shown, speed, worst];
    _fpsLabel.textColor = shown < 27.0f || speed < 95.0f ? [UIColor colorWithRed:1.0 green:0.8 blue:0.3 alpha:1.0]
                                                         : UIColor.whiteColor;
}

- (UIViewController*)presenter {
    UIViewController* vc = self.window.rootViewController;
    while (vc.presentedViewController != nil && !vc.presentedViewController.isBeingDismissed)
        vc = vc.presentedViewController;
    return vc;
}

- (void)presentAlert:(UIAlertController*)alert {
    bluewake_touch_clear();
    bluewake_pause_set(BLUEWAKE_PAUSE_ALERT, true);
    [[self presenter] presentViewController:alert animated:YES completion:nil];
}

- (UIAlertAction*)actionTitled:(NSString*)title style:(UIAlertActionStyle)style handler:(void (^)(void))handler {
    return [UIAlertAction actionWithTitle:title style:style handler:^(UIAlertAction* action) {
        (void)action;
        bluewake_pause_set(BLUEWAKE_PAUSE_ALERT, false);
        if (handler)
            handler();
    }];
}

// ---------------------------------------------------------------- menu

- (void)buildMenuButton {
    _menuButton = [BWMenuButton buttonWithType:UIButtonTypeCustom];
    UIImageSymbolConfiguration* symbol =
        [UIImageSymbolConfiguration configurationWithPointSize:19.0 weight:UIImageSymbolWeightBold];
    [_menuButton setImage:[UIImage systemImageNamed:@"ellipsis" withConfiguration:symbol]
                 forState:UIControlStateNormal];
    _menuButton.tintColor = UIColor.whiteColor;
    _menuButton.backgroundColor = [UIColor colorWithWhite:0.06 alpha:0.72];
    _menuButton.layer.cornerRadius = 20.0;
    _menuButton.layer.borderWidth = 1.0;
    _menuButton.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.30].CGColor;
    _menuButton.layer.masksToBounds = YES;
    _menuButton.accessibilityLabel = @"Menu";
    _menuButton.accessibilityIdentifier = @"BlueWakeMenu";
    // PaperPad's fix: a fixed capsule appearance, so iPadOS does not draw its
    // own square selected-state background (and drop the dots) while the menu
    // dismisses.
    UIButtonConfiguration* configuration = [UIButtonConfiguration plainButtonConfiguration];
    configuration.image = [UIImage systemImageNamed:@"ellipsis" withConfiguration:symbol];
    configuration.baseForegroundColor = UIColor.whiteColor;
    configuration.contentInsets = NSDirectionalEdgeInsetsZero;
    configuration.cornerStyle = UIButtonConfigurationCornerStyleCapsule;
    UIBackgroundConfiguration* background = [UIBackgroundConfiguration clearConfiguration];
    background.backgroundColor = [UIColor colorWithWhite:0.06 alpha:0.72];
    background.cornerRadius = 20.0;
    background.strokeColor = [UIColor colorWithWhite:1.0 alpha:0.30];
    background.strokeWidth = 1.0;
    configuration.background = background;
    _menuButton.changesSelectionAsPrimaryAction = NO;
    _menuButton.automaticallyUpdatesConfiguration = NO;
    _menuButton.configuration = configuration;
    _menuButton.backgroundColor = UIColor.clearColor;
    _menuButton.layer.borderWidth = 0.0;
    _menuButton.showsMenuAsPrimaryAction = YES;
    _menuButton.menu = [self buildMenu];
    [self addSubview:_menuButton];
}

- (void)refreshMenu {
    _menuButton.menu = [self buildMenu];
}

- (UIMenu*)buildMenu {
    __weak BWGameOverlay* weakSelf = self;
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    UIAction* showTouch = [UIAction actionWithTitle:@"Show Touch Controls"
                                              image:[UIImage systemImageNamed:@"hand.tap"]
                                         identifier:nil
                                            handler:^(__kindof UIAction* a) {
        (void)a;
        BOOL on = !BWBoolDefault(kShowTouchKey, YES);
        [[NSUserDefaults standardUserDefaults] setBool:on forKey:kShowTouchKey];
        [weakSelf applyControllerVisibility];
        [weakSelf refreshMenu];
    }];
    showTouch.state = BWBoolDefault(kShowTouchKey, YES) ? UIMenuElementStateOn : UIMenuElementStateOff;

    UIMenu* touchMenu = [UIMenu menuWithTitle:@"Touch Controls" image:[UIImage systemImageNamed:@"hand.draw"]
                                   identifier:nil options:0 children:@[
        showTouch,
        [UIAction actionWithTitle:@"Touch Control Settings…" image:[UIImage systemImageNamed:@"slider.horizontal.3"]
                       identifier:nil handler:^(__kindof UIAction* a) { (void)a; [weakSelf showSettingsPanel]; }],
        [UIAction actionWithTitle:@"Move Controls" image:[UIImage systemImageNamed:@"arrow.up.and.down.and.arrow.left.and.right"]
                       identifier:nil handler:^(__kindof UIAction* a) { (void)a; [weakSelf beginLayoutEditing]; }],
    ]];

    // Display: render resolution (applies at once) and aspect ratio.
    NSInteger scale = [defaults objectForKey:@BW_RENDER_SCALE_KEY] != nil
                          ? [defaults integerForKey:@BW_RENDER_SCALE_KEY] : 3;
    UIAction* (^scaleAction)(NSString*, NSInteger) = ^UIAction*(NSString* title, NSInteger value) {
        UIAction* action = [UIAction actionWithTitle:title image:nil identifier:nil handler:^(__kindof UIAction* a) {
            (void)a;
            [[NSUserDefaults standardUserDefaults] setInteger:value forKey:@BW_RENDER_SCALE_KEY];
            bluewake_settings_changed();
            [weakSelf refreshMenu];
        }];
        action.state = scale == value ? UIMenuElementStateOn : UIMenuElementStateOff;
        return action;
    };
    UIMenu* resolutionMenu = [UIMenu menuWithTitle:@"Render Resolution" image:[UIImage systemImageNamed:@"square.resize.up"]
                                        identifier:nil options:0 children:@[
        scaleAction(@"Screen Native", 0),
        scaleAction(@"4× (2560×1920)", 4),
        scaleAction(@"3× (1920×1440), Recommended", 3),
        scaleAction(@"2× (1280×960)", 2),
        scaleAction(@"Original (640×480)", 1),
    ]];

    // Texture filtering: forced anisotropy sharpens the ground and the sea at
    // a glancing angle (applies at once).
    NSInteger aniso = MAX((NSInteger)1, [defaults integerForKey:@BW_ANISOTROPY_KEY]);
    UIAction* (^anisoAction)(NSString*, NSInteger) = ^UIAction*(NSString* title, NSInteger value) {
        UIAction* action = [UIAction actionWithTitle:title image:nil identifier:nil handler:^(__kindof UIAction* a) {
            (void)a;
            [[NSUserDefaults standardUserDefaults] setInteger:value forKey:@BW_ANISOTROPY_KEY];
            bluewake_settings_changed();
            [weakSelf refreshMenu];
        }];
        action.state = aniso == value ? UIMenuElementStateOn : UIMenuElementStateOff;
        return action;
    };
    UIMenu* filteringMenu = [UIMenu menuWithTitle:@"Texture Filtering" image:[UIImage systemImageNamed:@"camera.filters"]
                                       identifier:nil options:0 children:@[
        anisoAction(@"16× Anisotropic", 16),
        anisoAction(@"8× Anisotropic", 8),
        anisoAction(@"4× Anisotropic", 4),
        anisoAction(@"Original", 1),
    ]];

    NSInteger aspect = (NSInteger)BWDefault(BWAspectModeKey, 0);
    UIAction* (^aspectAction)(NSString*, NSInteger) = ^UIAction*(NSString* title, NSInteger mode) {
        UIAction* action = [UIAction actionWithTitle:title image:nil identifier:nil handler:^(__kindof UIAction* a) {
            (void)a;
            if ((NSInteger)BWDefault(BWAspectModeKey, 0) == mode)
                return;
            [[NSUserDefaults standardUserDefaults] setInteger:mode forKey:BWAspectModeKey];
            [weakSelf refreshMenu];
            UIAlertController* alert = [UIAlertController
                alertControllerWithTitle:@"Applies Next Launch"
                                 message:@"The new aspect ratio takes effect the next time BlueWake starts."
                          preferredStyle:UIAlertControllerStyleAlert];
            [alert addAction:[weakSelf actionTitled:@"OK" style:UIAlertActionStyleDefault handler:nil]];
            [weakSelf presentAlert:alert];
        }];
        action.state = aspect == mode ? UIMenuElementStateOn : UIMenuElementStateOff;
        return action;
    };
    UIMenu* aspectMenu = [UIMenu menuWithTitle:@"Aspect Ratio" image:[UIImage systemImageNamed:@"aspectratio"]
                                    identifier:nil options:0 children:@[
        aspectAction(@"Original 4:3", 0),
        aspectAction(@"Fill Screen", 1),
    ]];
    UIAction* showFPS = [UIAction actionWithTitle:@"Show FPS" image:[UIImage systemImageNamed:@"speedometer"]
                                       identifier:nil handler:^(__kindof UIAction* a) {
        (void)a;
        [[NSUserDefaults standardUserDefaults] setBool:!BWBoolDefault(kShowFPSKey, NO) forKey:kShowFPSKey];
        [weakSelf applyFPSVisibility];
        [weakSelf refreshMenu];
    }];
    showFPS.state = BWBoolDefault(kShowFPSKey, NO) ? UIMenuElementStateOn : UIMenuElementStateOff;
    bluewake_settings_changed();
    const int smooth = g_bw_settings.smooth_motion;
    UIAction* (^smoothAction)(NSString*, NSInteger) = ^UIAction*(NSString* title, NSInteger value) {
        UIAction* action = [UIAction actionWithTitle:title image:nil identifier:nil handler:^(__kindof UIAction* a) {
            (void)a;
            [NSUserDefaults.standardUserDefaults setInteger:value forKey:@BW_SMOOTH_MOTION_KEY];
            bluewake_settings_changed();
            [weakSelf refreshMenu];
        }];
        action.state = smooth == value ? UIMenuElementStateOn : UIMenuElementStateOff;
        return action;
    };
    NSMutableArray<UIMenuElement*>* smoothChoices = [NSMutableArray arrayWithObjects:
        smoothAction(@"Off (Original 30 FPS)", 0), smoothAction(@"60 FPS", 1), nil];
    if (self.window.windowScene.screen.maximumFramesPerSecond >= 120 || smooth == 3)
        [smoothChoices addObject:smoothAction(@"120 FPS (ProMotion)", 3)];
    UIMenu* smoothMotion = [UIMenu menuWithTitle:@"Smooth Motion (Experimental)" image:[UIImage systemImageNamed:@"wind"]
                                    identifier:nil options:0 children:smoothChoices];
    UIMenu* displayMenu = [UIMenu menuWithTitle:@"Display" image:[UIImage systemImageNamed:@"display"]
                                     identifier:nil options:0
                                       children:@[ showFPS, smoothMotion, resolutionMenu, filteringMenu, aspectMenu ]];

    // Controller: camera stick direction and face-button mapping.
    UIAction* (^toggle)(NSString*, const char*) = ^UIAction*(NSString* title, const char* key) {
        NSString* k = @(key);
        UIAction* action = [UIAction actionWithTitle:title image:nil identifier:nil handler:^(__kindof UIAction* a) {
            (void)a;
            NSUserDefaults* d = [NSUserDefaults standardUserDefaults];
            [d setBool:![d boolForKey:k] forKey:k];
            bluewake_settings_changed();
            [weakSelf refreshMenu];
        }];
        action.state = [defaults boolForKey:k] ? UIMenuElementStateOn : UIMenuElementStateOff;
        return action;
    };
    UIMenu* cameraMenu = [UIMenu menuWithTitle:@"Camera Stick" image:[UIImage systemImageNamed:@"camera.rotate"]
                                    identifier:nil options:0 children:@[
        toggle(@"Invert Horizontal", BW_INVERT_CAMERA_X_KEY),
        toggle(@"Invert Vertical", BW_INVERT_CAMERA_Y_KEY),
    ]];
    NSMutableArray<UIMenuElement*>* remaps = [NSMutableArray array];
    for (int i = 0; i < BW_REMAP_COUNT; i++) {
        const unsigned current = bluewake_remap_current_native(i);
        NSString* currentName = @"";
        NSMutableArray<UIAction*>* choices = [NSMutableArray array];
        for (int c = 0; c < BW_NATIVE_CHOICES; c++) {
            const unsigned native = bluewake_native_choice(c);
            if (native == current) currentName = @(bluewake_native_choice_name(c));
            UIAction* choice = [UIAction actionWithTitle:@(bluewake_native_choice_name(c)) image:nil identifier:nil
                                                 handler:^(__kindof UIAction* a) {
                (void)a;
                bluewake_remap_set(i, native);
                [weakSelf refreshMenu];
            }];
            choice.state = native == current ? UIMenuElementStateOn : UIMenuElementStateOff;
            [choices addObject:choice];
        }
        UIMenu* button = [UIMenu menuWithTitle:[NSString stringWithFormat:@"GameCube %s", bluewake_remap_name(i)]
                                         image:nil identifier:nil options:0 children:choices];
        button.subtitle = currentName;
        [remaps addObject:button];
    }
    [remaps addObject:[UIAction actionWithTitle:@"Reset Buttons" image:[UIImage systemImageNamed:@"arrow.counterclockwise"]
                                     identifier:nil handler:^(__kindof UIAction* a) {
        (void)a;
        bluewake_remap_reset();
        [weakSelf refreshMenu];
    }]];
    UIMenu* buttonMenu = [UIMenu menuWithTitle:@"Button Mapping" image:[UIImage systemImageNamed:@"circle.grid.2x2"]
                                    identifier:nil options:0 children:remaps];
    UIMenu* controllerMenu = [UIMenu menuWithTitle:@"Controller" image:[UIImage systemImageNamed:@"gamecontroller"]
                                        identifier:nil options:0 children:@[ cameraMenu, buttonMenu ]];

    UIAction* removeDisc = [UIAction actionWithTitle:@"Remove Disc Image…" image:[UIImage systemImageNamed:@"trash"]
                                          identifier:nil handler:^(__kindof UIAction* a) {
        (void)a;
        [weakSelf confirmDataRemoval];
    }];
    removeDisc.attributes = UIMenuElementAttributesDestructive;
    UIMenu* dataMenu = [UIMenu menuWithTitle:@"Game Data & Saves" image:[UIImage systemImageNamed:@"externaldrive"]
                                  identifier:nil options:0 children:@[
        [UIMenu menuWithTitle:@"" image:nil identifier:nil options:UIMenuOptionsDisplayInline children:@[
            [UIAction actionWithTitle:@"Back Up Saves…" image:[UIImage systemImageNamed:@"square.and.arrow.up"]
                           identifier:nil handler:^(__kindof UIAction* a) { (void)a; [weakSelf backUpSaves]; }],
            [UIAction actionWithTitle:@"Restore Saves…" image:[UIImage systemImageNamed:@"clock.arrow.circlepath"]
                           identifier:nil handler:^(__kindof UIAction* a) { (void)a; [weakSelf chooseSavesToRestore]; }],
            [UIAction actionWithTitle:@"Import Dolphin Save…" image:[UIImage systemImageNamed:@"square.and.arrow.down"]
                           identifier:nil handler:^(__kindof UIAction* a) { (void)a; [weakSelf chooseDolphinSave]; }],
            [UIAction actionWithTitle:@"Where Are My Files?" image:[UIImage systemImageNamed:@"folder"]
                           identifier:nil handler:^(__kindof UIAction* a) { (void)a; [weakSelf explainFiles]; }],
        ]],
        removeDisc,
    ]];

    UIMenu* helpMenu = [UIMenu menuWithTitle:@"Help & Feedback" image:[UIImage systemImageNamed:@"questionmark.circle"]
                                  identifier:nil options:0 children:@[
        [UIAction actionWithTitle:@"Report a Problem on GitHub…" image:[UIImage systemImageNamed:@"exclamationmark.bubble"]
                       identifier:nil handler:^(__kindof UIAction* a) { (void)a; [weakSelf reportProblem]; }],
        [UIAction actionWithTitle:@"Share Session Log…" image:[UIImage systemImageNamed:@"doc.text"]
                       identifier:nil handler:^(__kindof UIAction* a) { (void)a; [weakSelf shareSessionLog]; }],
        [UIAction actionWithTitle:@"BlueWake on GitHub" image:[UIImage systemImageNamed:@"safari"]
                       identifier:nil handler:^(__kindof UIAction* a) {
            (void)a;
            [UIApplication.sharedApplication openURL:[NSURL URLWithString:@BW_GITHUB_URL] options:@{} completionHandler:nil];
        }],
    ]];

    // These guest changes apply at the next launch, like the code mods.
    UIAction* (^gameplayAction)(NSString*, NSString*) = ^UIAction*(NSString* title, NSString* key) {
        UIAction* action = [UIAction actionWithTitle:title image:nil identifier:nil handler:^(__kindof UIAction* a) {
            (void)a;
            [NSUserDefaults.standardUserDefaults setBool:!BWBoolDefault(key, NO) forKey:key];
            [weakSelf clearTouchInput];
            [weakSelf setNeedsLayout];
            [weakSelf refreshMenu];
        }];
        action.subtitle = @"Applies when BlueWake restarts";
        action.state = BWBoolDefault(key, NO) ? UIMenuElementStateOn : UIMenuElementStateOff;
        return action;
    };
    UIMenu* gameplayMenu = [UIMenu menuWithTitle:@"Gameplay" children:@[
        gameplayAction(@"Jump & Sprint", kMovementExtrasKey),
        gameplayAction(@"Fast Transitions", @"BlueWake.FastTransitions"),
        gameplayAction(@"Quick Doors", @"BlueWake.QuickDoors"),
    ]];
    UIMenu* root = [UIMenu menuWithTitle:@"BlueWake" children:@[
        displayMenu,
        gameplayMenu,
        [self modsMenu],
        controllerMenu,
        touchMenu,
        dataMenu,
        helpMenu,
    ]];
    // BLUEWAKE_MENU_DUMP=1 writes the menu tree to the session log once, so a
    // device run can check what the ⋯ menu offers without a screenshot.
    static bool dumped;
    const char* dump = getenv("BLUEWAKE_MENU_DUMP");
    if (!dumped && dump != NULL && dump[0] == '1') {
        dumped = true;
        BWDumpMenu(root, 0);
    }
    return root;
}

// Mods. Each applies the next time BlueWake starts: code mods are compiled into
// the game module and switched on before the game runs (BLUEWAKE_MODS, read in
// ios_entry.m), and texture replacement is loaded with the renderer.
// Texture count for the menu, counted once per launch (a pack holds thousands
// of files) and again after an install.
static NSUInteger g_packFiles = NSNotFound;

- (NSString*)texturePackDirectory {
    return [[self dataDirectory] stringByAppendingPathComponent:@"Load/Textures/GZLE01"];
}

// "On", "Off", or the change waiting for the next launch.
- (NSString*)modState:(NSString*)key {
    const BOOL now = [[NSUserDefaults standardUserDefaults] boolForKey:key];
    const BOOL launched = [_launchMods[key] boolValue];
    if (now == launched)
        return now ? @"On" : @"Off";
    return now ? @"On at next launch" : @"Off at next launch";
}

- (UIMenu*)modsMenu {
    __weak BWGameOverlay* weakSelf = self;
    UIAction* (^modToggle)(NSString*, NSString*, NSString*, NSString*) =
        ^UIAction*(NSString* title, NSString* detail, NSString* icon, NSString* key) {
        UIAction* action = [UIAction actionWithTitle:title image:[UIImage systemImageNamed:icon] identifier:nil
                                             handler:^(__kindof UIAction* a) {
            (void)a;
            NSUserDefaults* d = [NSUserDefaults standardUserDefaults];
            [d setBool:![d boolForKey:key] forKey:key];
            // One widescreen shape at a time: the two patch the same code.
            NSDictionary<NSString*, NSString*>* other = @{
                @BW_MOD_WIDESCREEN_KEY : @BW_MOD_WIDESCREEN_1610_KEY,
                @BW_MOD_WIDESCREEN_1610_KEY : @BW_MOD_WIDESCREEN_KEY,
            };
            if ([d boolForKey:key] && other[key] != nil)
                [d setBool:NO forKey:other[key]];
            [weakSelf refreshMenu];
            UIAlertController* alert = [UIAlertController
                alertControllerWithTitle:@"Applies Next Launch"
                                 message:@"Mods take effect the next time BlueWake starts. Your saves are not changed."
                          preferredStyle:UIAlertControllerStyleAlert];
            [alert addAction:[weakSelf actionTitled:@"OK" style:UIAlertActionStyleDefault handler:nil]];
            [weakSelf presentAlert:alert];
        }];
        action.subtitle = [NSString stringWithFormat:@"%@ · %@", detail, [weakSelf modState:key]];
        action.state = [[NSUserDefaults standardUserDefaults] boolForKey:key] ? UIMenuElementStateOn
                                                                              : UIMenuElementStateOff;
        return action;
    };
    if (g_packFiles == NSNotFound) {
        g_packFiles = 0;
        NSDirectoryEnumerator* walk = [[NSFileManager defaultManager] enumeratorAtPath:[self texturePackDirectory]];
        for (NSString* path in walk) {
            if ([path.pathExtension caseInsensitiveCompare:@"png"] == NSOrderedSame ||
                [path.pathExtension caseInsensitiveCompare:@"dds"] == NSOrderedSame)
                g_packFiles++;
        }
    }
    NSString* packDetail = g_packFiles > 0
        ? [NSString stringWithFormat:@"%lu textures installed", (unsigned long)g_packFiles]
        : @"No pack installed yet";
    NSString* bwwDetail = @"Swift Sail, instant text and more";
    UIMenu* toggles = [UIMenu menuWithTitle:@"" image:nil identifier:nil options:UIMenuOptionsDisplayInline
                                   children:@[
        modToggle(@"Widescreen 16:9", @"Shows more of the world on wide screens", @"rectangle.expand.vertical",
                  @BW_MOD_WIDESCREEN_KEY),
        modToggle(@"Widescreen 16:10", @"For 16:10 screens, such as a Mac or an iPad", @"rectangle.expand.vertical",
                  @BW_MOD_WIDESCREEN_1610_KEY),
        modToggle(@"HD Texture Pack", packDetail, @"photo.stack", @BW_MOD_HD_TEXTURES_KEY),
        modToggle(@"Better Wind Waker", bwwDetail, @"sailboat", @BW_MOD_BETTERWW_KEY),
    ]];
    UIAction* installPack = [UIAction actionWithTitle:@"Install Texture Pack…"
                                                image:[UIImage systemImageNamed:@"square.and.arrow.down"]
                                           identifier:nil handler:^(__kindof UIAction* a) {
        (void)a;
        [weakSelf chooseTexturePack];
    }];
    installPack.subtitle = @"Pick a folder of PNG or DDS textures";
    UIMenu* add = [UIMenu menuWithTitle:@"Add Mods" image:nil identifier:nil options:UIMenuOptionsDisplayInline
                               children:@[ installPack ]];
    NSMutableArray<UIMenuElement*>* children = [NSMutableArray arrayWithObjects:toggles, add, nil];
    UIMenu* bww = [self betterWWSettingsMenu];
    if (bww != nil)
        [children insertObject:bww atIndex:1];
    return [UIMenu menuWithTitle:@"Mods" image:[UIImage systemImageNamed:@"wand.and.stars"] identifier:nil options:0
                        children:children];
}

// Better Wind Waker's settings, one switch each, from the game module's
// option table. A switch the player changes is stored under
// BW_OPTION_KEY_PREFIX + its name and applies at the next launch (ios_entry.m
// passes it as BLUEWAKE_OPTIONS); the settings need Better Wind Waker on.
- (UIMenu*)betterWWSettingsMenu {
    __weak BWGameOverlay* weakSelf = self;
    NSMutableArray<UIMenuElement*>* items = [NSMutableArray array];
    const BOOL modOn = [[NSUserDefaults standardUserDefaults] boolForKey:@BW_MOD_BETTERWW_KEY];
    for (uint32_t i = 0;; ++i) {
        const char* title = NULL;
        bool defaultOn = false, running = false;
        const char* cname = bluewake_game_options_describe(i, &title, &defaultOn, &running);
        if (cname == NULL)
            break;
        NSString* name = @(cname);
        NSString* key = [@BW_OPTION_KEY_PREFIX stringByAppendingString:name];
        NSUserDefaults* d = [NSUserDefaults standardUserDefaults];
        const BOOL on = [d objectForKey:key] != nil ? [d boolForKey:key] : defaultOn;
        UIAction* action = [UIAction actionWithTitle:@(title ?: cname) image:nil identifier:nil
                                             handler:^(__kindof UIAction* a) {
            (void)a;
            NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
            [defaults setBool:!on forKey:key];
            // Swift Sail and Brisk Sail are two tunings of one sail.
            NSDictionary<NSString*, NSString*>* other = @{@"swift_sail" : @"brisk_sail",
                                                          @"brisk_sail" : @"swift_sail"};
            if (!on && other[name] != nil)
                [defaults setBool:NO forKey:[@BW_OPTION_KEY_PREFIX stringByAppendingString:other[name]]];
            [weakSelf refreshMenu];
        }];
        action.state = on ? UIMenuElementStateOn : UIMenuElementStateOff;
        if (modOn && on != running)
            action.subtitle = on ? @"On at next launch" : @"Off at next launch";
        [items addObject:action];
    }
    if (items.count == 0)
        return nil;
    UIMenu* menu = [UIMenu menuWithTitle:@"Better Wind Waker Settings" image:[UIImage systemImageNamed:@"slider.horizontal.3"]
                              identifier:nil options:0 children:items];
    menu.subtitle = modOn ? @"Apply at next launch" : @"Turn on Better Wind Waker to use them";
    return menu;
}

// ---------------------------------------------------------------- files

// The system file picker; the handler gets the picked URLs.
- (void)presentPicker:(UIDocumentPickerViewController*)picker handler:(void (^)(NSArray<NSURL*>*))handler {
    _pickHandler = [handler copy];
    picker.delegate = self;
    picker.allowsMultipleSelection = NO;
    bluewake_touch_clear();
    bluewake_pause_set(BLUEWAKE_PAUSE_ALERT, true);
    [[self presenter] presentViewController:picker animated:YES completion:nil];
}

- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
    (void)controller;
    bluewake_pause_set(BLUEWAKE_PAUSE_ALERT, false);
    void (^handler)(NSArray<NSURL*>*) = _pickHandler;
    _pickHandler = nil;
    if (handler != nil && urls.count > 0)
        handler(urls);
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller {
    (void)controller;
    bluewake_pause_set(BLUEWAKE_PAUSE_ALERT, false);
    _pickHandler = nil;
}

// Shows a wait alert while work runs off the main thread, then calls done on
// the main thread once the alert is gone. work may update the alert's message.
- (void)waitTitled:(NSString*)title message:(NSString*)message
              work:(void (^)(UIAlertController* wait))work done:(void (^)(void))done {
    UIAlertController* wait = [UIAlertController alertControllerWithTitle:title message:message
                                                           preferredStyle:UIAlertControllerStyleAlert];
    bluewake_touch_clear();
    bluewake_pause_set(BLUEWAKE_PAUSE_ALERT, true);
    [[self presenter] presentViewController:wait animated:YES completion:^{
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
            work(wait);
            dispatch_async(dispatch_get_main_queue(), ^{
                [wait dismissViewControllerAnimated:YES completion:^{
                    bluewake_pause_set(BLUEWAKE_PAUSE_ALERT, false);
                    done();
                }];
            });
        });
    }];
}

- (void)offerToTurnOn:(NSString*)key title:(NSString*)title message:(NSString*)message {
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:title message:message
                                                            preferredStyle:UIAlertControllerStyleAlert];
    __weak BWGameOverlay* weakSelf = self;
    if ([[NSUserDefaults standardUserDefaults] boolForKey:key]) {
        [alert addAction:[self actionTitled:@"OK" style:UIAlertActionStyleDefault handler:nil]];
    } else {
        [alert addAction:[self actionTitled:@"Not Now" style:UIAlertActionStyleCancel handler:nil]];
        UIAlertAction* on = [self actionTitled:@"Turn On" style:UIAlertActionStyleDefault handler:^{
            [[NSUserDefaults standardUserDefaults] setBool:YES forKey:key];
            [weakSelf refreshMenu];
        }];
        [alert addAction:on];
        alert.preferredAction = on;
    }
    [self presentAlert:alert];
}

- (void)chooseTexturePack {
    __weak BWGameOverlay* weakSelf = self;
    UIDocumentPickerViewController* picker =
        [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[ UTTypeFolder ]];
    [self presentPicker:picker handler:^(NSArray<NSURL*>* urls) { [weakSelf installTexturePackFrom:urls.firstObject]; }];
}

// Copies every PNG and DDS texture under the picked folder into
// Load/Textures/GZLE01, keeping its subfolders (the renderer searches them).
- (void)installTexturePackFrom:(NSURL*)source {
    __weak BWGameOverlay* weakSelf = self;
    NSString* dest = [self texturePackDirectory];
    NSString* sourcePath = source.URLByResolvingSymlinksInPath.path;
    NSString* destPath = [NSURL fileURLWithPath:dest].URLByResolvingSymlinksInPath.path;
    if ([sourcePath isEqualToString:destPath] || [sourcePath hasPrefix:[destPath stringByAppendingString:@"/"]]) {
        g_packFiles = NSNotFound;
        [self refreshMenu];
        [self offerToTurnOn:@BW_MOD_HD_TEXTURES_KEY title:@"Already Installed"
                    message:@"That folder is already in BlueWake's texture folder. HD Texture Pack uses it after "
                            @"BlueWake restarts."];
        return;
    }
    const BOOL scoped = [source startAccessingSecurityScopedResource];
    __block NSUInteger installed = 0, failed = 0;
    [self waitTitled:@"Installing Texture Pack" message:@"Copying textures…" work:^(UIAlertController* wait) {
        NSFileManager* fm = [NSFileManager new];
        [fm createDirectoryAtPath:dest withIntermediateDirectories:YES attributes:nil error:nil];
        NSDirectoryEnumerator* walk = [fm enumeratorAtPath:source.path];
        for (NSString* rel in walk) {
            NSString* ext = rel.pathExtension.lowercaseString;
            if (![ext isEqualToString:@"png"] && ![ext isEqualToString:@"dds"])
                continue;
            NSString* from = [source.path stringByAppendingPathComponent:rel];
            NSString* to = [dest stringByAppendingPathComponent:rel];
            [fm createDirectoryAtPath:[to stringByDeletingLastPathComponent] withIntermediateDirectories:YES
                           attributes:nil error:nil];
            [fm removeItemAtPath:to error:nil];
            if ([fm copyItemAtPath:from toPath:to error:nil])
                installed++;
            else
                failed++;
            if (installed % 250 == 0) {
                NSString* text = [NSString stringWithFormat:@"%lu textures copied…", (unsigned long)installed];
                dispatch_async(dispatch_get_main_queue(), ^{ wait.message = text; });
            }
        }
    } done:^{
        if (scoped)
            [source stopAccessingSecurityScopedResource];
        fprintf(stderr, "[mods] texture pack install: %lu copied, %lu failed\n", (unsigned long)installed,
                (unsigned long)failed);
        g_packFiles = NSNotFound;
        [weakSelf refreshMenu];
        if (installed == 0) {
            UIAlertController* alert = [UIAlertController
                alertControllerWithTitle:@"No Textures Found"
                                 message:@"That folder has no PNG or DDS textures. Pick the pack's folder of tex1_… "
                                         @"images for The Wind Waker (USA), often named GZLE01. If the pack is a .zip, "
                                         @"open it in the Files app first to unzip it."
                          preferredStyle:UIAlertControllerStyleAlert];
            [alert addAction:[weakSelf actionTitled:@"OK" style:UIAlertActionStyleDefault handler:nil]];
            [weakSelf presentAlert:alert];
            return;
        }
        NSString* message = [NSString stringWithFormat:@"%lu textures were installed.%@ HD Texture Pack uses them "
                                                       @"after BlueWake restarts.",
                                                       (unsigned long)installed,
                                                       failed ? [NSString stringWithFormat:@" %lu could not be copied.",
                                                                                           (unsigned long)failed]
                                                              : @""];
        [weakSelf offerToTurnOn:@BW_MOD_HD_TEXTURES_KEY title:@"Texture Pack Installed" message:message];
    }];
}

- (NSString*)dataDirectory {
    NSString* docs = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    return [docs stringByAppendingPathComponent:@"BlueWake"];
}

- (NSString*)cardPath {
    const char* env = getenv("BLUEWAKE_CARD_PATH");
    return env != NULL && env[0] != '\0' ? @(env) : [[self dataDirectory] stringByAppendingPathComponent:@"GZLE01.card"];
}

static NSString* BWDateStamp(NSString* format) {
    NSDateFormatter* f = [NSDateFormatter new];
    f.locale = [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
    f.dateFormat = format;
    return [f stringFromDate:[NSDate date]];
}

- (void)showMessage:(NSString*)title text:(NSString*)message {
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:title message:message
                                                            preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[self actionTitled:@"OK" style:UIAlertActionStyleDefault handler:nil]];
    [self presentAlert:alert];
}

// The card file holds the saves as of the last in-game save; this exports a copy.
- (void)backUpSaves {
    __weak BWGameOverlay* weakSelf = self;
    NSString* card = [self cardPath];
    if (![[NSFileManager defaultManager] fileExistsAtPath:card]) {
        [self showMessage:@"No Saves Yet" text:@"BlueWake has no memory card yet. Save in the game first."];
        return;
    }
    NSString* name = [NSString stringWithFormat:@"BlueWake-saves-%@.card", BWDateStamp(@"yyyy-MM-dd")];
    NSString* copy = [NSTemporaryDirectory() stringByAppendingPathComponent:name];
    [[NSFileManager defaultManager] removeItemAtPath:copy error:nil];
    NSError* error = nil;
    if (![[NSFileManager defaultManager] copyItemAtPath:card toPath:copy error:&error]) {
        [self showMessage:@"Could Not Back Up" text:error.localizedDescription];
        return;
    }
    UIDocumentPickerViewController* picker =
        [[UIDocumentPickerViewController alloc] initForExportingURLs:@[ [NSURL fileURLWithPath:copy] ] asCopy:YES];
    [self presentPicker:picker handler:^(NSArray<NSURL*>* urls) {
        (void)urls;
        [[NSFileManager defaultManager] removeItemAtPath:copy error:nil];
        [weakSelf showMessage:@"Saves Backed Up"
                         text:[NSString stringWithFormat:@"%@ holds your saves as of your last in-game save. "
                                                         @"Use Restore Saves to bring them back.", name]];
    }];
}

- (void)chooseSavesToRestore {
    __weak BWGameOverlay* weakSelf = self;
    UIDocumentPickerViewController* picker =
        [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[ UTTypeData ] asCopy:YES];
    [self presentPicker:picker handler:^(NSArray<NSURL*>* urls) { [weakSelf confirmRestoreFrom:urls.firstObject]; }];
}

- (void)confirmRestoreFrom:(NSURL*)source {
    __weak BWGameOverlay* weakSelf = self;
    if (!dol_card_validate(source.path.fileSystemRepresentation)) {
        [self showMessage:@"Not a Valid BlueWake Save File"
                     text:@"This card is damaged or incomplete. It has been kept unchanged. Choose another backup."];
        return;
    }
    NSString* stamp = BWDateStamp(@"yyyyMMdd-HHmmss");
    UIAlertController* alert = [UIAlertController
        alertControllerWithTitle:@"Replace Your Saves?"
                         message:[NSString stringWithFormat:
                                     @"Your current saves will be replaced by %@. A copy of them is kept in "
                                     @"BlueWake › Backups as GZLE01-%@.card.\n\nDo this at the title screen or "
                                     @"right after saving. BlueWake then closes so the restored saves load when "
                                     @"you open it again.",
                                     source.lastPathComponent, stamp]
                  preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[self actionTitled:@"Cancel" style:UIAlertActionStyleCancel handler:^{
    }]];
    [alert addAction:[self actionTitled:@"Replace Saves" style:UIAlertActionStyleDestructive handler:^{
        [weakSelf restoreSavesFrom:source stamp:stamp];
    }]];
    [self presentAlert:alert];
}

- (void)restoreSavesFrom:(NSURL*)source stamp:(NSString*)stamp {
    if (![self replaceCardWith:source.path stamp:stamp])
        return;
    fprintf(stderr, "[shell] saves restored from %s; previous card kept in Backups\n",
            source.lastPathComponent.UTF8String);
    [self closeForSavesTitled:@"Saves Restored"
                      message:@"BlueWake will close now. Open it again to play with the restored saves."];
}

// Keeps a copy of the current card in Backups, then puts the file at path in
// its place. The file at path is used up either way. Says why and returns NO
// if the card was not changed.
- (BOOL)replaceCardWith:(NSString*)path stamp:(NSString*)stamp {
    if (!dol_card_validate(path.fileSystemRepresentation)) {
        [self showMessage:@"Saves Not Changed" text:@"The replacement card is damaged or incomplete. It has been kept unchanged."];
        return NO;
    }
    bluewake_card_runtime_suspend_writes(true);
    NSFileManager* fm = [NSFileManager defaultManager];
    NSString* card = [self cardPath];
    NSString* backups = [[self dataDirectory] stringByAppendingPathComponent:@"Backups"];
    NSError* error = nil;
    if ([fm fileExistsAtPath:card]) {
        [fm createDirectoryAtPath:backups withIntermediateDirectories:YES attributes:nil error:nil];
        NSString* backup = [backups stringByAppendingPathComponent:[NSString stringWithFormat:@"GZLE01-%@.card", stamp]];
        if (![fm copyItemAtPath:card toPath:backup error:&error]) {
            [self showMessage:@"Saves Not Changed"
                         text:[NSString stringWithFormat:@"Your current saves could not be backed up, so nothing "
                                                         @"was replaced. %@", error.localizedDescription]];
            bluewake_card_runtime_suspend_writes(false);
            return NO;
        }
    }
    // Staged beside the card and renamed over it, so the card is never half written.
    NSString* staged = [card stringByAppendingString:@".restore"];
    [fm removeItemAtPath:staged error:nil];
    if (![fm moveItemAtPath:path toPath:staged error:&error] || rename(staged.fileSystemRepresentation,
                                                                       card.fileSystemRepresentation) != 0) {
        [fm removeItemAtPath:staged error:nil];
        [self showMessage:@"Saves Not Changed"
                     text:[NSString stringWithFormat:@"The save file could not be put in place. %@",
                                                     error.localizedDescription ?: @""]];
        bluewake_card_runtime_suspend_writes(false);
        return NO;
    }
    return YES;
}

// The running game keeps the old card in memory and writes all of it on its
// next save, so BlueWake closes before that can happen.
- (void)closeForSavesTitled:(NSString*)title message:(NSString*)message {
    UIAlertController* alert = [UIAlertController
        alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[self actionTitled:@"Close BlueWake" style:UIAlertActionStyleDefault handler:^{ bw_process_close(bluewake_card_runtime_close, 0); }]];
    [self presentAlert:alert];
}

// ---------------------------------------------------------------- Dolphin saves

// "Link · 6¾ hearts · 180 rupees" for one quest log.
static NSString* BWQuestLogSummary(const BWQuestLog& log) {
    static NSString* const quarters[] = { @"", @"¼", @"½", @"¾" };
    unsigned whole = log.max_life / 4;
    NSString* hearts = [NSString stringWithFormat:@"%u%@ heart%@", whole, quarters[log.max_life % 4],
                                                  log.max_life == 4 ? @"" : @"s"];
    NSString* name = log.name[0] != 0 ? @(log.name) : @"No name";
    return [NSString stringWithFormat:@"%@ · %@ · %u rupee%@", name, hearts, log.rupees, log.rupees == 1 ? @"" : @"s"];
}

// Dolphin keeps a save as a .gci (Memory Card Manager › Export) or inside a
// memory card file (.raw); either works.
- (void)chooseDolphinSave {
    __weak BWGameOverlay* weakSelf = self;
    UIDocumentPickerViewController* picker =
        [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[ UTTypeData ] asCopy:YES];
    [self presentPicker:picker handler:^(NSArray<NSURL*>* urls) { [weakSelf readDolphinSave:urls.firstObject]; }];
}

- (void)readDolphinSave:(NSURL*)source {
    NSData* file = [NSData dataWithContentsOfURL:source options:NSDataReadingMappedIfSafe error:nil];
    [[NSFileManager defaultManager] removeItemAtURL:source error:nil];
    NSData* card = [NSData dataWithContentsOfFile:[self cardPath]];
    if (card == nil) {
        [self showMessage:@"No Memory Card Yet"
                     text:@"BlueWake makes its memory card when the game starts. Open the game once, then try again."];
        return;
    }
    BWDolphinSave save = {};
    const char* error = file != nil ? bw_dolphin_save_parse((const uint8_t*)file.bytes, file.length, &save)
                                    : "The file could not be read.";
    BWQuestLog here[BW_QUEST_LOGS];
    bool hasSaves = false;
    if (error == NULL)
        error = bw_card_quest_logs((const uint8_t*)card.bytes, card.length, &hasSaves, here);
    if (error != NULL) {
        bw_dolphin_save_free(&save);
        [self showMessage:@"Could Not Import" text:@(error)];
        return;
    }
    BWQuestLog theirs[BW_QUEST_LOGS];
    bw_gczelda_quest_logs(save.data, theirs);
    NSData* dolphin = [NSData dataWithBytes:save.entry length:sizeof save.entry];
    NSMutableData* both = [dolphin mutableCopy];
    [both appendBytes:save.data length:save.length];
    bw_dolphin_save_free(&save);
    [self chooseQuestLogFrom:both named:source.lastPathComponent theirs:theirs card:card
                        here:hasSaves ? [NSData dataWithBytes:here length:sizeof here] : nil];
}

// save holds the 64-byte directory entry and then the gczelda file, the same
// layout as a .gci, so it can be parsed again when the import runs. here holds
// BlueWake's quest logs, or is nil when its card has no saves yet.
- (void)chooseQuestLogFrom:(NSData*)save named:(NSString*)name theirs:(const BWQuestLog*)theirs card:(NSData*)card
                      here:(NSData*)here {
    __weak BWGameOverlay* weakSelf = self;
    UIAlertController* sheet = [UIAlertController alertControllerWithTitle:@"Import Which Quest Log?"
                                                                    message:name
                                                             preferredStyle:UIAlertControllerStyleActionSheet];
    NSUInteger usable = 0;
    for (int i = 0; i < BW_QUEST_LOGS; i++) {
        if (theirs[i].empty)
            continue;
        NSString* summary = BWQuestLogSummary(theirs[i]);
        UIAlertAction* action;
        if (here == nil) {
            // Nothing on BlueWake's card yet, so the whole Dolphin file goes in.
            action = [self actionTitled:summary style:UIAlertActionStyleDefault handler:^{
                [weakSelf confirmImport:save named:name source:0 summary:nil card:card destination:0 replacing:nil];
            }];
        } else {
            action = [self actionTitled:summary style:UIAlertActionStyleDefault handler:^{
                [weakSelf chooseDestinationFor:save named:name source:i + 1 summary:summary card:card
                                          here:(const BWQuestLog*)here.bytes];
            }];
        }
        action.enabled = theirs[i].checksum_ok;
        if (theirs[i].checksum_ok)
            usable++;
        [sheet addAction:action];
    }
    if (usable == 0) {
        [self showMessage:@"Nothing to Import" text:@"This Dolphin save has no quest logs BlueWake can read."];
        return;
    }
    [sheet addAction:[self actionTitled:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    [self presentSheet:sheet];
}

- (void)chooseDestinationFor:(NSData*)save named:(NSString*)name source:(int)source summary:(NSString*)summary
                        card:(NSData*)card here:(const BWQuestLog*)here {
    __weak BWGameOverlay* weakSelf = self;
    UIAlertController* sheet = [UIAlertController
        alertControllerWithTitle:@"Put It in Which Quest Log?"
                         message:[NSString stringWithFormat:@"%@ goes into the quest log you pick and replaces "
                                                            @"what is there now.", summary]
                  preferredStyle:UIAlertControllerStyleActionSheet];
    for (int i = 0; i < BW_QUEST_LOGS; i++) {
        NSString* there = here[i].empty ? nil : BWQuestLogSummary(here[i]);
        NSString* title = [NSString stringWithFormat:@"Quest Log %d: %@", i + 1, there ?: @"Empty"];
        [sheet addAction:[self actionTitled:title style:UIAlertActionStyleDefault handler:^{
            [weakSelf confirmImport:save named:name source:source summary:summary card:card destination:i + 1
                          replacing:there];
        }]];
    }
    [sheet addAction:[self actionTitled:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    [self presentSheet:sheet];
}

// source and destination are 0 when BlueWake has no saves yet and the whole
// Dolphin file is added.
- (void)confirmImport:(NSData*)save named:(NSString*)name source:(int)source summary:(NSString*)summary
                 card:(NSData*)card destination:(int)destination replacing:(NSString*)replacing {
    __weak BWGameOverlay* weakSelf = self;
    NSString* stamp = BWDateStamp(@"yyyyMMdd-HHmmss");
    NSString* what;
    if (destination == 0)
        what = [NSString stringWithFormat:@"BlueWake has no saves yet, so all quest logs in %@ are copied in.", name];
    else if (replacing != nil)
        what = [NSString stringWithFormat:@"Quest Log %d (%@) will be replaced by %@ from %@.", destination, replacing,
                                          summary, name];
    else
        what = [NSString stringWithFormat:@"%@ from %@ goes into Quest Log %d.", summary, name, destination];
    UIAlertController* alert = [UIAlertController
        alertControllerWithTitle:replacing != nil ? [NSString stringWithFormat:@"Replace Quest Log %d?", destination]
                                                  : @"Import Save?"
                         message:[NSString stringWithFormat:
                                     @"%@ A copy of your current saves is kept in BlueWake › Backups as "
                                     @"GZLE01-%@.card.\n\nDo this at the title screen or right after saving. "
                                     @"BlueWake then closes so the imported save loads when you open it again.",
                                     what, stamp]
                  preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[self actionTitled:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    NSString* go = replacing != nil ? [NSString stringWithFormat:@"Replace Quest Log %d", destination] : @"Import";
    [alert addAction:[self actionTitled:go style:UIAlertActionStyleDestructive handler:^{
        [weakSelf importSave:save named:name source:source card:card destination:destination stamp:stamp];
    }]];
    [self presentAlert:alert];
}

- (void)importSave:(NSData*)save named:(NSString*)name source:(int)source card:(NSData*)card
       destination:(int)destination stamp:(NSString*)stamp {
    // The card is read again here in case the game saved while the menus were up.
    NSData* now = [NSData dataWithContentsOfFile:[self cardPath]] ?: card;
    BWDolphinSave parsed;
    uint8_t* bytes = NULL;
    size_t size = 0;
    const char* error = bw_dolphin_save_parse((const uint8_t*)save.bytes, save.length, &parsed);
    if (error == NULL)
        error = bw_card_import((const uint8_t*)now.bytes, now.length, &parsed, source, destination, &bytes, &size);
    bw_dolphin_save_free(&parsed);
    if (error != NULL) {
        [self showMessage:@"Could Not Import" text:@(error)];
        return;
    }
    NSData* result = [NSData dataWithBytesNoCopy:bytes length:size freeWhenDone:YES];
    NSString* path = [[self cardPath] stringByAppendingString:@".import"];
    NSError* writeError = nil;
    if (![result writeToFile:path options:NSDataWritingAtomic error:&writeError]) {
        [self showMessage:@"Saves Not Changed" text:writeError.localizedDescription];
        return;
    }
    if (![self replaceCardWith:path stamp:stamp])
        return;
    fprintf(stderr, "[shell] Dolphin save imported from %s (quest log %d into %d); previous card kept in Backups\n",
            name.UTF8String, source, destination);
    [self closeForSavesTitled:@"Save Imported"
                      message:@"BlueWake will close now. Open it again and pick the quest log to play."];
}

// Action sheets on iPad point at the menu button.
- (void)presentSheet:(UIAlertController*)sheet {
    sheet.popoverPresentationController.sourceView = _menuButton;
    sheet.popoverPresentationController.sourceRect = _menuButton.bounds;
    [self presentAlert:sheet];
}

- (void)explainFiles {
    UIAlertController* alert = [UIAlertController
        alertControllerWithTitle:@"Your Files"
                         message:@"In the Files app, open On My iPad › BlueWake › BlueWake.\n\n"
                                  "Your live saves are protected in BlueWake’s private storage. Back Up Saves makes a copy you can keep anywhere, "
                                  "and Restore Saves brings one back; earlier cards are kept in Backups. "
                                  "Import Dolphin Save brings in a quest log from a Dolphin .gci or memory card file. "
                                  "GZLE01.iso is your disc image, and main.dol and rels are made from it. "
                                  "Mods and texture packs live in Mods and Load."
                  preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[self actionTitled:@"OK" style:UIAlertActionStyleDefault handler:nil]];
    [self presentAlert:alert];
}

- (void)confirmDataRemoval {
    __weak BWGameOverlay* weakSelf = self;
    UIAlertController* alert = [UIAlertController
        alertControllerWithTitle:@"Remove Disc Image?"
                         message:@"This deletes your disc image (GZLE01.iso) and the game files made from it "
                                  "(main.dol and rels). Your saves, mods and settings are kept.\n\n"
                                  "Do this at the title screen or right after saving. BlueWake then closes and asks "
                                  "for your disc the next time it starts."
                  preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[self actionTitled:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    [alert addAction:[self actionTitled:@"Remove Disc Image" style:UIAlertActionStyleDestructive handler:^{
        NSFileManager* fm = [NSFileManager defaultManager];
        NSString* dir = [weakSelf dataDirectory];
        for (NSString* name in @[ @"GZLE01.iso", @"main.dol", @"rels" ])
            [fm removeItemAtPath:[dir stringByAppendingPathComponent:name] error:nil];
        fprintf(stderr, "[shell] game data removed; saves kept\n");
        // The game reads from these files as it plays, so it cannot go on.
        UIAlertController* done = [UIAlertController
            alertControllerWithTitle:@"Disc Image Removed"
                             message:@"Your saves, mods and settings are still here. BlueWake will close now and "
                                     @"ask for your disc when you open it again."
                      preferredStyle:UIAlertControllerStyleAlert];
        [done addAction:[weakSelf actionTitled:@"Close BlueWake" style:UIAlertActionStyleDefault
                                       handler:^{ bw_process_close(bluewake_card_runtime_close, 0); }]];
        [weakSelf presentAlert:done];
    }]];
    [self presentAlert:alert];
}

- (NSString*)diagnosticReport {
    struct utsname sys;
    uname(&sys);
    NSFileManager* fm = [NSFileManager defaultManager];
    NSString* dir = [self dataDirectory];
    NSDictionary* info = NSBundle.mainBundle.infoDictionary;
    NSMutableString* report = [NSMutableString string];
    [report appendFormat:@"BlueWake %@ (%@)\n", info[@"CFBundleShortVersionString"], info[@"CFBundleVersion"]];
    [report appendFormat:@"Device %s, %@ %@\n", sys.machine, UIDevice.currentDevice.systemName,
                         UIDevice.currentDevice.systemVersion];
#if TARGET_OS_SIMULATOR
    [report appendString:@"Simulator\n"];
#endif
    for (NSString* name in @[ @"GZLE01.iso", @"main.dol", @"rels", @"GZLE01.card", @"sram.bin" ])
        [report appendFormat:@"%@: %@\n", name,
                             [fm fileExistsAtPath:[dir stringByAppendingPathComponent:name]] ? @"present" : @"missing"];
    [report appendFormat:@"Controllers: %lu\n", (unsigned long)GCController.controllers.count];
    NSUserDefaults* d = [NSUserDefaults standardUserDefaults];
    [report appendFormat:@"Render scale: %@, anisotropy: %ld, smooth motion: %d, aspect: %ld, camera invert x=%d y=%d, "
                          "buttons remapped: %d\n",
                         [d objectForKey:@BW_RENDER_SCALE_KEY] ?: @"3 (default)",
                         (long)MAX((NSInteger)1, [d integerForKey:@BW_ANISOTROPY_KEY]),
                         g_bw_settings.smooth_motion, (long)[d integerForKey:BWAspectModeKey],
                         [d boolForKey:@BW_INVERT_CAMERA_X_KEY], [d boolForKey:@BW_INVERT_CAMERA_Y_KEY],
                         [d dictionaryForKey:@BW_BUTTON_MAP_KEY] != nil];
    [report appendFormat:@"Thermal state: %ld, Low Power Mode: %d\n",
                         (long)NSProcessInfo.processInfo.thermalState,
                         NSProcessInfo.processInfo.lowPowerModeEnabled];
    return report;
}

- (void)reportProblem {
    // A marker in the session log at the moment the player reported it.
    fprintf(stderr, "[mark] Report a Problem opened\n");
    NSString* report = [self diagnosticReport];
    // A new GitHub issue with the device report filled in. The session log is
    // too long for a URL; the body asks for it (Help & Feedback > Share
    // Session Log, or Files > BlueWake > logs).
    NSString* body = [NSString stringWithFormat:
        @"**What happened?**\n\n\n**What were you doing (scene, menu, controls)?**\n\n\n"
        @"**About when (the time on your iPad)?**\n\n\n"
        @"Please attach the session log: BlueWake menu > Help & Feedback > Share Session Log "
        @"(or Files > On My iPad > BlueWake > BlueWake > logs).\n\n"
        @"<details><summary>Device report</summary>\n\n```\n%@```\n</details>\n", report];
    NSURLComponents* url = [NSURLComponents componentsWithString:@BW_GITHUB_URL "/issues/new"];
    url.queryItems = @[
        [NSURLQueryItem queryItemWithName:@"title" value:@""],
        [NSURLQueryItem queryItemWithName:@"body" value:body],
    ];
    [UIApplication.sharedApplication openURL:url.URL options:@{} completionHandler:nil];
}

- (NSURL*)latestSessionLog {
    NSString* dir = [[self dataDirectory] stringByAppendingPathComponent:@"logs"];
    NSArray<NSString*>* logs = [[[[NSFileManager defaultManager] contentsOfDirectoryAtPath:dir error:nil]
        filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"SELF BEGINSWITH 'session-'"]]
        sortedArrayUsingSelector:@selector(compare:)];
    return logs.count ? [NSURL fileURLWithPath:[dir stringByAppendingPathComponent:logs.lastObject]] : nil;
}

- (void)shareSessionLog {
    fprintf(stderr, "[mark] Share Session Log opened\n");
    NSURL* log = [self latestSessionLog];
    NSArray* items = log ? @[ log ] : @[ [self diagnosticReport] ];
    UIActivityViewController* share = [[UIActivityViewController alloc] initWithActivityItems:items
                                                                       applicationActivities:nil];
    share.popoverPresentationController.sourceView = _menuButton;
    share.popoverPresentationController.sourceRect = _menuButton.bounds;
    share.completionWithItemsHandler = ^(UIActivityType type, BOOL done, NSArray* items, NSError* error) {
        (void)type; (void)done; (void)items; (void)error;
        bluewake_pause_set(BLUEWAKE_PAUSE_ALERT, false);
    };
    bluewake_touch_clear();
    bluewake_pause_set(BLUEWAKE_PAUSE_ALERT, true);
    [[self presenter] presentViewController:share animated:YES completion:nil];
}

// ---------------------------------------------------------------- controls

- (void)buildTouchControls {
    _moveStick = [self makeStick];
    _moveStick.accessibilityLabel = @"Move stick";
    _moveStick.accessibilityIdentifier = @"move";
    [_moveStick applyBaseColor:[UIColor colorWithWhite:0.13 alpha:0.86]
                    thumbColor:[UIColor colorWithWhite:0.58 alpha:0.94]];
    _cStick = [self makeStick];
    _cStick.accessibilityLabel = @"Camera stick";
    _cStick.accessibilityIdentifier = @"c";
    [_cStick applyBaseColor:[UIColor colorWithRed:0.91 green:0.66 blue:0.08 alpha:0.90]
                 thumbColor:[UIColor colorWithRed:1.00 green:0.84 blue:0.25 alpha:0.98]];

    [self addButton:@"A" mask:BLUEWAKE_TOUCH_A identifier:@"A"];
    [self addButton:@"B" mask:BLUEWAKE_TOUCH_B identifier:@"B"];
    [self addButton:@"X" mask:BLUEWAKE_TOUCH_X identifier:@"X"];
    [self addButton:@"Y" mask:BLUEWAKE_TOUCH_Y identifier:@"Y"];
    [self addButton:@"Z" mask:BLUEWAKE_TOUCH_Z identifier:@"Z"];
    [self addButton:@"START" mask:BLUEWAKE_TOUCH_START identifier:@"Start"];
    [self addButton:@"L" mask:BLUEWAKE_TOUCH_L identifier:@"L"];
    [self addButton:@"R" mask:BLUEWAKE_TOUCH_R identifier:@"R"];
    [self addButton:@"Jump" mask:BLUEWAKE_TOUCH_JUMP identifier:@"Jump"];
    [self addButton:@"Run" mask:BLUEWAKE_TOUCH_SPRINT identifier:@"Sprint"];
    [self buttonWithMask:BLUEWAKE_TOUCH_SPRINT].accessibilityLabel = @"Sprint (hold)";
    [self addButton:@"▲" mask:BLUEWAKE_TOUCH_DPAD_UP identifier:@"D_U"];
    [self addButton:@"▼" mask:BLUEWAKE_TOUCH_DPAD_DOWN identifier:@"D_D"];
    [self addButton:@"◀" mask:BLUEWAKE_TOUCH_DPAD_LEFT identifier:@"D_L"];
    [self addButton:@"▶" mask:BLUEWAKE_TOUCH_DPAD_RIGHT identifier:@"D_R"];
    [self buttonWithMask:BLUEWAKE_TOUCH_DPAD_UP].accessibilityLabel = @"D-pad up";
    [self buttonWithMask:BLUEWAKE_TOUCH_DPAD_DOWN].accessibilityLabel = @"D-pad down";
    [self buttonWithMask:BLUEWAKE_TOUCH_DPAD_LEFT].accessibilityLabel = @"D-pad left";
    [self buttonWithMask:BLUEWAKE_TOUCH_DPAD_RIGHT].accessibilityLabel = @"D-pad right";

    // The D-pad moves and resizes as one object in the editor while its four
    // directions keep their own hit regions in play.
    _dpadGroup = [UIView new];
    _dpadGroup.accessibilityLabel = @"D-pad";
    _dpadGroup.accessibilityIdentifier = @"DPad";
    _dpadGroup.backgroundColor = UIColor.clearColor;
    _dpadGroup.layer.cornerRadius = 14.0;
    _dpadGroup.hidden = YES;
    _dpadGroup.userInteractionEnabled = NO;
    [self addSubview:_dpadGroup];
    [self addEditGestures:_dpadGroup];
}

- (BWStickView*)makeStick {
    BWStickView* stick = [[BWStickView alloc] initWithFrame:CGRectMake(0, 0, 128, 128)];
    __weak BWGameOverlay* weakSelf = self;
    __weak BWStickView* weakStick = stick;
    stick.valueChanged = ^(float x, float y) {
        BWStickView* strong = weakStick;
        if (strong != nil)
            [weakSelf stick:strong movedX:x y:y];
    };
    [self addSubview:stick];
    [self addEditGestures:stick];
    return stick;
}

- (void)addButton:(NSString*)label mask:(uint16_t)mask identifier:(NSString*)identifier {
    BWGameButton* button = [BWGameButton buttonWithType:UIButtonTypeSystem];
    [button setTitle:label forState:UIControlStateNormal];
    UIColor* fill = [UIColor colorWithWhite:0.22 alpha:0.88];
    UIColor* title = UIColor.whiteColor;
    switch (mask) {
    case BLUEWAKE_TOUCH_A: fill = [UIColor colorWithRed:0.08 green:0.56 blue:0.29 alpha:0.92]; break;
    case BLUEWAKE_TOUCH_B: fill = [UIColor colorWithRed:0.78 green:0.10 blue:0.13 alpha:0.92]; break;
    case BLUEWAKE_TOUCH_X:
    case BLUEWAKE_TOUCH_Y:
        fill = [UIColor colorWithWhite:0.72 alpha:0.92];
        title = [UIColor colorWithWhite:0.12 alpha:1.0];
        break;
    case BLUEWAKE_TOUCH_Z: fill = [UIColor colorWithRed:0.38 green:0.18 blue:0.58 alpha:0.94]; break;
    case BLUEWAKE_TOUCH_START: fill = [UIColor colorWithWhite:0.28 alpha:0.92]; break;
    default: break;
    }
    [button setTitleColor:title forState:UIControlStateNormal];
    button.titleLabel.font = [UIFont systemFontOfSize:18.0 weight:UIFontWeightBold];
    button.backgroundColor = fill;
    button.layer.borderWidth = 2.0;
    button.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.36].CGColor;
    button.accessibilityLabel = label;
    button.accessibilityIdentifier = identifier;
    button.inputMask = mask;
    [button addTarget:self action:@selector(buttonDown:) forControlEvents:UIControlEventTouchDown];
    [button addTarget:self action:@selector(buttonUp:)
     forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside | UIControlEventTouchCancel];
    [_buttons addObject:button];
    [self addSubview:button];
    [self addEditGestures:button];
}

- (BWGameButton*)buttonWithMask:(uint16_t)mask {
    for (BWGameButton* button in _buttons)
        if (button.inputMask == mask)
            return button;
    return nil;
}

- (void)publish {
    bluewake_touch_publish(&_pad);
}

- (void)stick:(BWStickView*)stick movedX:(float)x y:(float)y {
    if (_editing)
        return;
    const int8_t xi = (int8_t)std::lround(x * 127.0f), yi = (int8_t)std::lround(y * 127.0f);
    if (stick == _moveStick) {
        _pad.stick_x = xi;
        _pad.stick_y = yi;
    } else {
        _pad.c_stick_x = xi;
        _pad.c_stick_y = yi;
    }
    [self publish];
}

- (void)buttonDown:(BWGameButton*)button {
    if (_editing)
        return;
    _pad.buttons |= button.inputMask;
    button.transform = CGAffineTransformMakeScale(0.92, 0.92);
    [self publish];
}

- (void)buttonUp:(BWGameButton*)button {
    if (_editing)
        return;
    _pad.buttons &= (uint16_t)~button.inputMask;
    button.transform = CGAffineTransformIdentity;
    [self publish];
}

// Test hook support: press and release a control through the same handlers a
// finger uses (sendActionsForControlEvents), so an unattended simulator run
// exercises the UIKit path into the pad.
- (void)tapIdentifier:(NSString*)identifier holdSeconds:(double)hold {
    for (BWGameButton* button in _buttons) {
        if (![button.accessibilityIdentifier isEqualToString:identifier])
            continue;
        [button sendActionsForControlEvents:UIControlEventTouchDown];
        fprintf(stderr, "[shell] test tap %s down\n", identifier.UTF8String);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(hold * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            [button sendActionsForControlEvents:UIControlEventTouchUpInside];
            fprintf(stderr, "[shell] test tap %s up\n", identifier.UTF8String);
        });
        return;
    }
    if ([identifier hasPrefix:@"move:"] || [identifier hasPrefix:@"c:"]) {
        // move:x,y or c:x,y holds a stick at that deflection (-1..1, +y up).
        NSArray<NSString*>* parts = [[identifier substringFromIndex:[identifier rangeOfString:@":"].location + 1]
            componentsSeparatedByString:@","];
        BWStickView* stick = [identifier hasPrefix:@"move:"] ? _moveStick : _cStick;
        if (parts.count == 2) {
            [self stick:stick movedX:parts[0].floatValue y:parts[1].floatValue];
            fprintf(stderr, "[shell] test stick %s\n", identifier.UTF8String);
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(hold * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                [self stick:stick movedX:0 y:0];
                fprintf(stderr, "[shell] test stick released\n");
            });
        }
    }
}

- (void)clearTouchInput {
    for (BWGameButton* button in _buttons)
        button.transform = CGAffineTransformIdentity;
    _pad = BlueWakeTouchPad{};
    [_moveStick reset];
    [_cStick reset];
    _pad = BlueWakeTouchPad{};
    bluewake_touch_clear();
}

// ---------------------------------------------------------------- layout

- (CGRect)safeArea {
    return UIEdgeInsetsInsetRect(self.bounds, self.safeAreaInsets);
}

- (void)layoutSubviews {
    [super layoutSubviews];
    const CGRect safe = [self safeArea];
    // SunPad's defaults: a fixed set on iPads at least 1000 points wide, a
    // reference 800x380 area scaled down elsewhere, normalized phone positions.
    const BOOL phone = BWIsPhone(self);
    const BOOL pad = !phone && safe.size.width >= 1000.0;
    const CGFloat base = pad ? 1.0 : std::min<CGFloat>(1.0, std::min(safe.size.width / 800.0, safe.size.height / 380.0));
    const CGFloat size = BWDefault(kSizeKey, 1.0);
    const CGFloat scale = base * size;
    const CGFloat margin = pad ? 34.0 : std::max<CGFloat>(8.0, 18.0 * base);
    const CGFloat stick = (pad ? 172.0 : 126.0 * base) * size;
    const CGFloat small = (pad ? 62.0 : 46.0 * base) * size;
    const CGFloat medium = (pad ? 76.0 : 58.0 * base) * size;
    const CGFloat large = (pad ? 104.0 : 78.0 * base) * size;
    const CGFloat camera = (pad ? 112.0 : 86.0 * base) * size;
    _columnFrames = phone ? [self phoneColumnFramesInSafeArea:safe size:size] : nil;

    [self place:_moveStick identifier:@"move" defaultFrame:
        phone ? BWFrameAtNormalizedCenter(safe, 0.1234722222, 0.7803490991, stick, stick)
        : pad ? BWFrameAtNormalizedCenter(safe, 0.1310395315, 0.7905894519, stick, stick)
              : CGRectMake(CGRectGetMinX(safe) + margin, CGRectGetMaxY(safe) - stick - margin, stick, stick)];
    [self place:_cStick identifier:@"c" defaultFrame:
        phone ? BWFrameAtNormalizedCenter(safe, 0.9233055556, 0.8130067568, camera, camera)
        // SunPad's tablet defaults were tuned on a larger iPad; on an
        // 11-inch screen the C-stick, L and Start overlapped their
        // neighbours, so those sit slightly further out.
        : pad ? BWFrameAtNormalizedCenter(safe, 0.9062957540, 0.8800000000, camera, camera)
              : CGRectMake(CGRectGetMaxX(safe) - margin - camera, CGRectGetMaxY(safe) - margin - camera, camera, camera)];

    BWGameButton* a = [self buttonWithMask:BLUEWAKE_TOUCH_A];
    [self place:a identifier:@"A" defaultFrame:
        pad ? BWFrameAtNormalizedCenter(safe, 0.8916544656, 0.7409513961, large, large)
            : CGRectMake(CGRectGetMaxX(safe) - margin - large,
                         CGRectGetMaxY(safe) - margin - camera - large - 18.0 * scale, large, large)];
    [self place:[self buttonWithMask:BLUEWAKE_TOUCH_B] identifier:@"B" defaultFrame:
        phone ? BWFrameAtNormalizedCenter(safe, 0.8398611111, 0.6898648649, medium, medium)
        : pad ? BWFrameAtNormalizedCenter(safe, 0.8360175695, 0.8092037229, medium, medium)
              : CGRectMake(CGRectGetMinX(a.frame) - medium - 12.0 * scale, CGRectGetMidY(a.frame) + 8.0, medium, medium)];
    [self place:[self buttonWithMask:BLUEWAKE_TOUCH_X] identifier:@"X" defaultFrame:
        phone ? BWFrameAtNormalizedCenter(safe, 0.9034166667, 0.4258445946, small, small)
        : pad ? BWFrameAtNormalizedCenter(safe, 0.9593704246, 0.7156153051, small, small)
              : CGRectMake(CGRectGetMidX(a.frame) - small * 0.5, CGRectGetMinY(a.frame) - small - 10.0 * scale, small, small)];
    [self place:[self buttonWithMask:BLUEWAKE_TOUCH_Y] identifier:@"Y" defaultFrame:
        phone ? BWFrameAtNormalizedCenter(safe, 0.8452500000, 0.5268581081, small, small)
        : pad ? BWFrameAtNormalizedCenter(safe, 0.9542459736, 0.7869700103, small, small)
              : CGRectMake(CGRectGetMinX(a.frame) - small - 8.0 * scale, CGRectGetMinY(a.frame) - small + 8.0, small, small)];

    const CGFloat shoulder = (pad ? 132.0 : 94.0 * base) * size;
    const CGFloat shoulderY = CGRectGetMinY(safe) + (pad ? 92.0 : 68.0 * base);
    [self place:[self buttonWithMask:BLUEWAKE_TOUCH_L] identifier:@"L" defaultFrame:
        phone ? BWFrameAtNormalizedCenter(safe, 0.0905833333, 0.2539977477, shoulder, small)
        : pad ? BWFrameAtNormalizedCenter(safe, 0.1281112738, 0.6400000000, shoulder, small)
              : CGRectMake(CGRectGetMinX(safe) + margin, shoulderY, shoulder, small)];
    // SunPad widened R for Sunshine's analog FLUDD trigger; here it is an
    // ordinary shoulder button the same size as L.
    [self place:[self buttonWithMask:BLUEWAKE_TOUCH_R] identifier:@"R" defaultFrame:
        phone ? BWFrameAtNormalizedCenter(safe, 0.8687500000, 0.2729166667, shoulder, small)
        : pad ? BWFrameAtNormalizedCenter(safe, 0.8960468521, 0.6400000000, shoulder, small)
              : CGRectMake(CGRectGetMaxX(safe) - margin - shoulder, shoulderY, shoulder, small)];
    [self place:[self buttonWithMask:BLUEWAKE_TOUCH_Z] identifier:@"Z" defaultFrame:
        phone ? BWFrameAtNormalizedCenter(safe, 0.9712500000, 0.4350788288, small, small)
        : pad ? BWFrameAtNormalizedCenter(safe, 0.8275988287, 0.7213029990, small, small)
              : CGRectMake(CGRectGetMaxX(safe) - margin - shoulder - small - 12.0 * scale, shoulderY, small, small)];
    const CGFloat startWidth = (pad ? 116.0 : 92.0 * base) * size;
    [self place:[self buttonWithMask:BLUEWAKE_TOUCH_START] identifier:@"Start" defaultFrame:
        phone ? BWFrameAtNormalizedCenter(safe, 0.0902222222, 0.1128941441, startWidth, small)
        : pad ? BWFrameAtNormalizedCenter(safe, 0.8967789165, 0.5600000000, startWidth, small)
              : CGRectMake(CGRectGetMidX(safe) - startWidth * 0.5, CGRectGetMinY(safe) + margin, startWidth, small)];

    const CGFloat d = (pad ? 48.0 : 36.0 * base) * size;
    [self place:[self buttonWithMask:BLUEWAKE_TOUCH_JUMP] identifier:@"Jump" defaultFrame:
        BWFrameAtNormalizedCenter(safe, 0.73, 0.78, medium, small)];
    [self place:[self buttonWithMask:BLUEWAKE_TOUCH_SPRINT] identifier:@"Sprint" defaultFrame:
        BWFrameAtNormalizedCenter(safe, 0.25, 0.60, medium, small)];
    const CGRect dpadDefault =
        phone ? BWFrameAtNormalizedCenter(safe, 0.0812777778, 0.4677364865, 3.0 * d, 3.0 * d)
        : pad ? BWFrameAtNormalizedCenter(safe, 0.2686676428, 0.7947259566, 3.0 * d, 3.0 * d)
              : CGRectMake(CGRectGetMaxX(_moveStick.frame) + 18.0 * scale, CGRectGetMidY(_moveStick.frame) - 1.5 * d, 3.0 * d, 3.0 * d);
    [self place:_dpadGroup identifier:@"DPad" defaultFrame:dpadDefault];
    [self layoutDPadButtons];

    for (BWGameButton* button in _buttons)
        button.layer.cornerRadius = std::min(button.bounds.size.width, button.bounds.size.height) * 0.5;

    _menuButton.frame = CGRectMake(CGRectGetMaxX(safe) - 40.0 - 12.0, CGRectGetMinY(safe) + 12.0, 40.0, 40.0);
    _fpsLabel.frame = CGRectMake(CGRectGetMinX(safe) + 12.0, CGRectGetMinY(safe) + 12.0, 250.0, 28.0);
    const CGFloat panelWidth = MIN(360.0, CGRectGetWidth(safe) - 32.0);
    _settingsPanel.frame = CGRectMake(CGRectGetMaxX(safe) - panelWidth - 12.0, CGRectGetMinY(safe) + 60.0,
                                      panelWidth, MIN(330.0, CGRectGetHeight(safe) * 0.62));
    const CGFloat editorWidth = MIN(560.0, CGRectGetWidth(safe) - 24.0);
    _editorBar.frame = CGRectMake(CGRectGetMidX(safe) - editorWidth * 0.5, CGRectGetMaxY(safe) - 72.0, editorWidth, 60.0);
    [self updateAppearance];
    [self bringSubviewToFront:_settingsPanel];
    [self bringSubviewToFront:_editorBar];
    [self bringSubviewToFront:_menuButton];
}

// On a phone the original 4:3 picture leaves a black bar on each side (about
// 100 points of an iPhone 17's 874 after the safe area). SunPad's phone
// defaults were placed for a full-width game and cover Wind Waker's minimap and
// item HUD, so while the 4:3 picture is on and the bars are wide enough the
// defaults are two columns in the bars: movement, D-pad, L and Start on the
// left; Z, R, the face buttons and the camera stick on the right. Returns nil
// when they do not apply (Fill Screen, narrow bars); a saved position still
// wins over either default.
- (NSDictionary<NSString*, NSValue*>*)phoneColumnFramesInSafeArea:(CGRect)safe size:(CGFloat)size {
    if (_launchAspectMode != 0)
        return nil;
    const CGFloat picture = self.bounds.size.height * 4.0 / 3.0;
    const CGFloat pictureMinX = CGRectGetMidX(self.bounds) - picture * 0.5;
    const CGFloat pictureMaxX = CGRectGetMidX(self.bounds) + picture * 0.5;
    const CGFloat gap = 4.0;
    const CGFloat leftMin = CGRectGetMinX(safe) + gap, leftMax = pictureMinX - gap;
    const CGFloat rightMin = pictureMaxX + gap, rightMax = CGRectGetMaxX(safe) - gap;
    const CGFloat w = MIN(leftMax - leftMin, rightMax - rightMin);
    if (w < 84.0 || safe.size.height < 300.0)
        return nil;
    const CGFloat col = MIN(w, 120.0) * size;  // wider bars (Max models) keep sizes sane
    const CGFloat lx = (leftMin + leftMax) * 0.5, rx = (rightMin + rightMax) * 0.5;
    const CGFloat top = CGRectGetMinY(safe), bottom = CGRectGetMaxY(safe), h = safe.size.height;
    auto at = [](CGFloat x, CGFloat y, CGFloat fw, CGFloat fh) {
        return [NSValue valueWithCGRect:CGRectMake(x - fw * 0.5, y - fh * 0.5, fw, fh)];
    };
    const CGFloat stick = col, camera = col * 0.80, d = col * 0.30;
    const CGFloat a = col * 0.60, b = col * 0.42, face = col * 0.40, z = col * 0.36;
    const CGFloat pillH = col * 0.38;
    return @{
        // left column, top to bottom
        @"Start": at(lx, top + 24.0, col * 0.72, col * 0.32),
        @"L": at(lx, top + 24.0 + col * 0.46, col * 0.86, pillH),
        @"DPad": at(lx, top + h * 0.47, 3.0 * d, 3.0 * d),
        @"move": at(lx, bottom - stick * 0.5 - 6.0, stick, stick),
        // right column below the menu button: Z and R, then Y and X, then B and A
        @"Z": at(rx - col * 0.5 + z * 0.5, top + 80.0, z, z),
        @"R": at(rx + col * 0.5 - col * 0.29, top + 80.0, col * 0.58, pillH),
        @"Y": at(rx - col * 0.25, top + h * 0.34, face, face),
        @"X": at(rx + col * 0.27, top + h * 0.34 + col * 0.10, face, face),
        @"A": at(rx + col * 0.16, top + h * 0.53, a, a),
        @"B": at(rx - col * 0.30, top + h * 0.53 + col * 0.30, b, b),
        @"c": at(rx, bottom - camera * 0.5 - 6.0, camera, camera),
    };
}

// Sparse persistence: a control with no saved position keeps its form
// factor's default, and a saved center is clamped into the safe area.
- (void)place:(UIView*)control identifier:(NSString*)identifier defaultFrame:(CGRect)frame {
    if (NSValue* column = _columnFrames[identifier])
        frame = column.CGRectValue;
    NSDictionary* scales = [[NSUserDefaults standardUserDefaults] dictionaryForKey:BWLayoutKey(self, @"ControlSizeScales")];
    const CGFloat individual = scales[identifier] != nil ? std::clamp([scales[identifier] doubleValue], 0.6, 1.75) : 1.0;
    control.bounds = CGRectMake(0, 0, frame.size.width * individual, frame.size.height * individual);
    NSDictionary* origins = [[NSUserDefaults standardUserDefaults] dictionaryForKey:BWLayoutKey(self, @"ControlOrigins")];
    NSString* saved = origins[identifier];
    const CGRect safe = [self safeArea];
    CGPoint center = CGPointMake(CGRectGetMidX(frame), CGRectGetMidY(frame));
    if ([saved isKindOfClass:NSString.class]) {
        CGPoint n = CGPointFromString(saved);
        center = CGPointMake(CGRectGetMinX(safe) + std::clamp<CGFloat>(n.x, 0.0, 1.0) * safe.size.width,
                             CGRectGetMinY(safe) + std::clamp<CGFloat>(n.y, 0.0, 1.0) * safe.size.height);
    }
    const CGFloat hw = MIN(control.bounds.size.width * 0.5, safe.size.width * 0.5);
    const CGFloat hh = MIN(control.bounds.size.height * 0.5, safe.size.height * 0.5);
    center.x = std::clamp(center.x, CGRectGetMinX(safe) + hw, CGRectGetMaxX(safe) - hw);
    center.y = std::clamp(center.y, CGRectGetMinY(safe) + hh, CGRectGetMaxY(safe) - hh);
    control.center = center;
}

- (void)layoutDPadButtons {
    const CGFloat cell = _dpadGroup.bounds.size.width / 3.0;
    const CGPoint c = _dpadGroup.center;
    const struct { uint16_t mask; CGFloat x, y; } cells[] = {
        {BLUEWAKE_TOUCH_DPAD_UP, 0, -1}, {BLUEWAKE_TOUCH_DPAD_DOWN, 0, 1},
        {BLUEWAKE_TOUCH_DPAD_LEFT, -1, 0}, {BLUEWAKE_TOUCH_DPAD_RIGHT, 1, 0},
    };
    for (const auto& cellInfo : cells) {
        BWGameButton* button = [self buttonWithMask:cellInfo.mask];
        button.bounds = CGRectMake(0, 0, cell, cell);
        button.center = CGPointMake(c.x + cellInfo.x * cell, c.y + cellInfo.y * cell);
    }
}

- (NSArray<UIView*>*)gameplayControls {
    NSMutableArray<UIView*>* controls = [NSMutableArray arrayWithArray:_buttons];
    [controls addObject:_moveStick];
    [controls addObject:_cStick];
    return controls;
}

- (BOOL)isDPadButton:(UIView*)view {
    if (![view isKindOfClass:BWGameButton.class])
        return NO;
    const uint16_t mask = ((BWGameButton*)view).inputMask;
    return mask == BLUEWAKE_TOUCH_DPAD_UP || mask == BLUEWAKE_TOUCH_DPAD_DOWN ||
           mask == BLUEWAKE_TOUCH_DPAD_LEFT || mask == BLUEWAKE_TOUCH_DPAD_RIGHT;
}

- (void)updateAppearance {
    const BOOL hidden = (_controllerHidden || !BWBoolDefault(kShowTouchKey, YES)) && !_editing;
    const CGFloat alpha = _editing ? 1.0 : BWDefault(kOpacityKey, 0.8);
    for (UIView* control in [self gameplayControls]) {
        BOOL extraHidden = NO;
        if ([control isKindOfClass:BWGameButton.class]) {
            const uint16_t mask = ((BWGameButton*)control).inputMask;
            extraHidden = (mask == BLUEWAKE_TOUCH_JUMP || mask == BLUEWAKE_TOUCH_SPRINT)
                && !BWBoolDefault(kMovementExtrasKey, NO) && !_editing;
        }
        control.hidden = hidden || extraHidden;
        control.userInteractionEnabled = !control.hidden;
        control.alpha = alpha;
        UIColor* border = [UIColor colorWithWhite:1.0 alpha:0.68];
        CGFloat width = 2.0;
        if (_editing && ![self isDPadButton:control]) {
            border = control == _selected ? [UIColor colorWithRed:0.20 green:0.78 blue:1.0 alpha:1.0]
                                          : [UIColor colorWithRed:1.0 green:0.78 blue:0.20 alpha:0.95];
            width = control == _selected ? 4.0 : 3.0;
        }
        control.layer.borderColor = border.CGColor;
        control.layer.borderWidth = width;
    }
    const BOOL showGroup = _editing;
    _dpadGroup.hidden = !showGroup;
    _dpadGroup.userInteractionEnabled = showGroup;
    _dpadGroup.layer.borderColor = (_selected == _dpadGroup ? [UIColor colorWithRed:0.20 green:0.78 blue:1.0 alpha:1.0]
                                                            : [UIColor colorWithRed:1.0 green:0.78 blue:0.20 alpha:0.95]).CGColor;
    _dpadGroup.layer.borderWidth = _selected == _dpadGroup ? 4.0 : 3.0;
    if (showGroup)
        [self bringSubviewToFront:_dpadGroup];
}

// ---------------------------------------------------------------- settings panel

- (UIView*)rowTitled:(NSString*)title control:(UIView*)control {
    UILabel* label = [UILabel new];
    label.text = title;
    label.textColor = UIColor.whiteColor;
    label.font = [UIFont preferredFontForTextStyle:UIFontTextStyleSubheadline];
    label.adjustsFontForContentSizeCategory = YES;
    [control setContentHuggingPriority:UILayoutPriorityDefaultLow forAxis:UILayoutConstraintAxisHorizontal];
    [label setContentHuggingPriority:UILayoutPriorityDefaultHigh forAxis:UILayoutConstraintAxisHorizontal];
    UIStackView* row = [[UIStackView alloc] initWithArrangedSubviews:@[ label, control ]];
    row.spacing = 12.0;
    row.alignment = UIStackViewAlignmentCenter;
    return row;
}

- (void)buildSettingsPanel {
    _settingsPanel = [UIView new];
    _settingsPanel.backgroundColor = [UIColor colorWithWhite:0.035 alpha:0.94];
    _settingsPanel.layer.cornerRadius = 16.0;
    _settingsPanel.layer.borderWidth = 1.0;
    _settingsPanel.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.22].CGColor;
    _settingsPanel.hidden = YES;
    _settingsPanel.accessibilityIdentifier = @"TouchSettings";
    [self addSubview:_settingsPanel];

    UILabel* title = [UILabel new];
    title.text = @"Touch Controls";
    title.textColor = UIColor.whiteColor;
    title.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    UIButton* close = [UIButton buttonWithType:UIButtonTypeCustom];
    [close setImage:[UIImage systemImageNamed:@"xmark"
                            withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:16.0 weight:UIImageSymbolWeightBold]]
           forState:UIControlStateNormal];
    close.tintColor = UIColor.whiteColor;
    close.backgroundColor = [UIColor colorWithWhite:1.0 alpha:0.14];
    close.layer.cornerRadius = 16.0;
    close.accessibilityLabel = @"Close touch control settings";
    [close addTarget:self action:@selector(hideSettingsPanel) forControlEvents:UIControlEventTouchUpInside];
    UIStackView* header = [[UIStackView alloc] initWithArrangedSubviews:@[ title, close ]];
    header.alignment = UIStackViewAlignmentCenter;
    header.translatesAutoresizingMaskIntoConstraints = NO;
    [_settingsPanel addSubview:header];

    _opacitySlider = [UISlider new];
    _opacitySlider.minimumValue = 0.25;
    _opacitySlider.maximumValue = 1.0;
    _opacitySlider.accessibilityLabel = @"Control opacity";
    [_opacitySlider addTarget:self action:@selector(opacityChanged:) forControlEvents:UIControlEventValueChanged];
    _sizeSlider = [UISlider new];
    _sizeSlider.minimumValue = 0.70;
    _sizeSlider.maximumValue = 1.35;
    _sizeSlider.accessibilityLabel = @"Control size";
    [_sizeSlider addTarget:self action:@selector(sizeChanged:) forControlEvents:UIControlEventValueChanged];
    _hideSwitch = [UISwitch new];
    _hideSwitch.accessibilityLabel = @"Hide touch controls when a controller is connected";
    [_hideSwitch addTarget:self action:@selector(hideChanged:) forControlEvents:UIControlEventValueChanged];
    _editSwitch = [UISwitch new];
    _editSwitch.accessibilityLabel = @"Move touch controls";
    [_editSwitch addTarget:self action:@selector(editChanged:) forControlEvents:UIControlEventValueChanged];
    UIButton* reset = [UIButton buttonWithType:UIButtonTypeSystem];
    [reset setTitle:@"Reset This Device's Layout" forState:UIControlStateNormal];
    [reset setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    reset.backgroundColor = [UIColor colorWithWhite:0.18 alpha:0.88];
    reset.layer.cornerRadius = 10.0;
    reset.accessibilityIdentifier = @"ResetLayout";
    [reset addTarget:self action:@selector(confirmResetLayout) forControlEvents:UIControlEventTouchUpInside];

    UIStackView* stack = [[UIStackView alloc] initWithArrangedSubviews:@[
        [self rowTitled:@"Opacity" control:_opacitySlider],
        [self rowTitled:@"Size" control:_sizeSlider],
        [self rowTitled:@"Hide with a controller" control:_hideSwitch],
        [self rowTitled:@"Move controls" control:_editSwitch],
        reset,
    ]];
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 10.0;
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    [_settingsPanel addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[
        [header.leadingAnchor constraintEqualToAnchor:_settingsPanel.leadingAnchor constant:16.0],
        [header.trailingAnchor constraintEqualToAnchor:_settingsPanel.trailingAnchor constant:-12.0],
        [header.topAnchor constraintEqualToAnchor:_settingsPanel.topAnchor constant:8.0],
        [header.heightAnchor constraintEqualToConstant:40.0],
        [close.widthAnchor constraintEqualToConstant:32.0],
        [close.heightAnchor constraintEqualToConstant:32.0],
        [stack.leadingAnchor constraintEqualToAnchor:_settingsPanel.leadingAnchor constant:16.0],
        [stack.trailingAnchor constraintEqualToAnchor:_settingsPanel.trailingAnchor constant:-16.0],
        [stack.topAnchor constraintEqualToAnchor:header.bottomAnchor constant:8.0],
        [reset.heightAnchor constraintEqualToConstant:40.0],
    ]];

    _editorBar = [UIView new];
    _editorBar.backgroundColor = [UIColor colorWithWhite:0.035 alpha:0.95];
    _editorBar.layer.cornerRadius = 16.0;
    _editorBar.layer.borderWidth = 1.0;
    _editorBar.layer.borderColor = [UIColor colorWithRed:1.0 green:0.78 blue:0.20 alpha:0.95].CGColor;
    _editorBar.hidden = YES;
    _editorBar.accessibilityIdentifier = @"LayoutEditor";
    [self addSubview:_editorBar];
    _editorHint = [UILabel new];
    _editorHint.text = @"Drag controls • tap one to resize";
    _editorHint.textColor = UIColor.whiteColor;
    _editorHint.font = [UIFont systemFontOfSize:14.0 weight:UIFontWeightSemibold];
    _editorHint.adjustsFontSizeToFitWidth = YES;
    _selectedSizeSlider = [UISlider new];
    _selectedSizeSlider.minimumValue = 0.60;
    _selectedSizeSlider.maximumValue = 1.75;
    _selectedSizeSlider.value = 1.0;
    _selectedSizeSlider.enabled = NO;
    [_selectedSizeSlider addTarget:self action:@selector(selectedSizeChanged:) forControlEvents:UIControlEventValueChanged];
    UIButton* done = [UIButton buttonWithType:UIButtonTypeSystem];
    [done setTitle:@"Done" forState:UIControlStateNormal];
    [done setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    done.titleLabel.font = [UIFont systemFontOfSize:15.0 weight:UIFontWeightBold];
    done.backgroundColor = [UIColor colorWithRed:0.12 green:0.48 blue:0.82 alpha:1.0];
    done.layer.cornerRadius = 10.0;
    done.accessibilityLabel = @"Finish moving touch controls";
    [done addTarget:self action:@selector(endLayoutEditing) forControlEvents:UIControlEventTouchUpInside];
    UIStackView* editor = [[UIStackView alloc] initWithArrangedSubviews:@[ _editorHint, _selectedSizeSlider, done ]];
    editor.alignment = UIStackViewAlignmentCenter;
    editor.spacing = 12.0;
    editor.translatesAutoresizingMaskIntoConstraints = NO;
    [_editorBar addSubview:editor];
    [NSLayoutConstraint activateConstraints:@[
        [editor.leadingAnchor constraintEqualToAnchor:_editorBar.leadingAnchor constant:14.0],
        [editor.trailingAnchor constraintEqualToAnchor:_editorBar.trailingAnchor constant:-10.0],
        [editor.topAnchor constraintEqualToAnchor:_editorBar.topAnchor constant:8.0],
        [editor.bottomAnchor constraintEqualToAnchor:_editorBar.bottomAnchor constant:-8.0],
        [_selectedSizeSlider.widthAnchor constraintGreaterThanOrEqualToConstant:150.0],
        [done.widthAnchor constraintEqualToConstant:68.0],
        [done.heightAnchor constraintEqualToConstant:40.0],
    ]];
}

- (void)showSettingsPanel {
    [self endLayoutEditing];
    _opacitySlider.value = BWDefault(kOpacityKey, 0.8);
    _sizeSlider.value = BWDefault(kSizeKey, 1.0);
    _hideSwitch.on = BWBoolDefault(kHideOnControllerKey, YES);
    _editSwitch.on = NO;
    _settingsPanel.hidden = NO;
    bluewake_touch_clear();
    bluewake_pause_set(BLUEWAKE_PAUSE_SETTINGS, true);
    [self setNeedsLayout];
}

- (void)hideSettingsPanel {
    _settingsPanel.hidden = YES;
    bluewake_pause_set(BLUEWAKE_PAUSE_SETTINGS, false);
}

- (void)opacityChanged:(UISlider*)slider {
    [[NSUserDefaults standardUserDefaults] setDouble:slider.value forKey:kOpacityKey];
    [self updateAppearance];
}

- (void)sizeChanged:(UISlider*)slider {
    [[NSUserDefaults standardUserDefaults] setDouble:slider.value forKey:kSizeKey];
    [self setNeedsLayout];
}

- (void)hideChanged:(UISwitch*)sw {
    [[NSUserDefaults standardUserDefaults] setBool:sw.on forKey:kHideOnControllerKey];
    [self applyControllerVisibility];
}

- (void)editChanged:(UISwitch*)sw {
    if (sw.on)
        [self beginLayoutEditing];
    else
        [self endLayoutEditing];
}

- (void)confirmResetLayout {
    __weak BWGameOverlay* weakSelf = self;
    UIAlertController* alert = [UIAlertController
        alertControllerWithTitle:@"Reset the Touch Layout?"
                         message:@"Every control on this kind of device, including the D-pad, returns to its default position and size."
                  preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[self actionTitled:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    [alert addAction:[self actionTitled:@"Reset" style:UIAlertActionStyleDestructive handler:^{
        NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
        [defaults removeObjectForKey:BWLayoutKey(weakSelf, @"ControlOrigins")];
        [defaults removeObjectForKey:BWLayoutKey(weakSelf, @"ControlSizeScales")];
        [weakSelf setNeedsLayout];
    }]];
    [self presentAlert:alert];
}

// ---------------------------------------------------------------- layout editor

- (void)addEditGestures:(UIView*)control {
    UIPanGestureRecognizer* drag = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(dragged:)];
    drag.enabled = NO;
    [control addGestureRecognizer:drag];
    [_editGestures addObject:drag];
    UITapGestureRecognizer* tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tapped:)];
    tap.enabled = NO;
    [control addGestureRecognizer:tap];
    [_editGestures addObject:tap];
}

- (void)beginLayoutEditing {
    _editing = YES;
    _settingsPanel.hidden = YES;
    bluewake_pause_set(BLUEWAKE_PAUSE_SETTINGS, false);
    _editorBar.hidden = NO;
    _selected = nil;
    _selectedSizeSlider.enabled = NO;
    _editorHint.text = @"Drag controls • tap one to resize";
    [self clearTouchInput];
    bluewake_pause_set(BLUEWAKE_PAUSE_LAYOUT, true);
    for (UIGestureRecognizer* g in _editGestures)
        g.enabled = ![self isDPadButton:g.view];
    [self setNeedsLayout];
}

- (void)endLayoutEditing {
    if (!_editing)
        return;
    [self clearTouchInput];
    _editing = NO;
    _editSwitch.on = NO;
    _editorBar.hidden = YES;
    for (UIGestureRecognizer* g in _editGestures)
        g.enabled = NO;
    _selected = nil;
    bluewake_pause_set(BLUEWAKE_PAUSE_LAYOUT, false);
    [self setNeedsLayout];
}

- (void)dragged:(UIPanGestureRecognizer*)drag {
    UIView* control = drag.view;
    if (!_editing || control == nil)
        return;
    if (drag.state == UIGestureRecognizerStateBegan)
        [self select:control];
    CGPoint t = [drag translationInView:self];
    [drag setTranslation:CGPointZero inView:self];
    const CGRect safe = [self safeArea];
    const CGFloat hw = MIN(control.bounds.size.width * 0.5, safe.size.width * 0.5);
    const CGFloat hh = MIN(control.bounds.size.height * 0.5, safe.size.height * 0.5);
    CGPoint c = CGPointMake(std::clamp(control.center.x + t.x, CGRectGetMinX(safe) + hw, CGRectGetMaxX(safe) - hw),
                            std::clamp(control.center.y + t.y, CGRectGetMinY(safe) + hh, CGRectGetMaxY(safe) - hh));
    control.center = c;
    if (control == _dpadGroup)
        [self layoutDPadButtons];
    if (drag.state == UIGestureRecognizerStateEnded || drag.state == UIGestureRecognizerStateCancelled) {
        NSString* identifier = control.accessibilityIdentifier;
        if (identifier.length == 0 || safe.size.width <= 0 || safe.size.height <= 0)
            return;
        NSMutableDictionary* origins = [[[NSUserDefaults standardUserDefaults]
            dictionaryForKey:BWLayoutKey(self, @"ControlOrigins")] mutableCopy] ?: [NSMutableDictionary dictionary];
        origins[identifier] = NSStringFromCGPoint(CGPointMake((c.x - CGRectGetMinX(safe)) / safe.size.width,
                                                              (c.y - CGRectGetMinY(safe)) / safe.size.height));
        [[NSUserDefaults standardUserDefaults] setObject:origins forKey:BWLayoutKey(self, @"ControlOrigins")];
    }
}

- (void)tapped:(UITapGestureRecognizer*)tap {
    if (tap.state == UIGestureRecognizerStateEnded)
        [self select:tap.view];
}

- (void)select:(UIView*)control {
    if (!_editing || control.accessibilityIdentifier.length == 0)
        return;
    _selected = control;
    NSDictionary* scales = [[NSUserDefaults standardUserDefaults] dictionaryForKey:BWLayoutKey(self, @"ControlSizeScales")];
    NSNumber* scale = scales[control.accessibilityIdentifier];
    _selectedSizeSlider.value = scale != nil ? scale.floatValue : 1.0f;
    _selectedSizeSlider.enabled = YES;
    _editorHint.text = [NSString stringWithFormat:@"%@ size", control.accessibilityLabel];
    [self updateAppearance];
}

- (void)selectedSizeChanged:(UISlider*)slider {
    NSString* identifier = _selected.accessibilityIdentifier;
    if (!_editing || identifier.length == 0)
        return;
    NSMutableDictionary* scales = [[[NSUserDefaults standardUserDefaults]
        dictionaryForKey:BWLayoutKey(self, @"ControlSizeScales")] mutableCopy] ?: [NSMutableDictionary dictionary];
    scales[identifier] = @(std::clamp<double>(slider.value, 0.6, 1.75));
    [[NSUserDefaults standardUserDefaults] setObject:scales forKey:BWLayoutKey(self, @"ControlSizeScales")];
    [self setNeedsLayout];
}

// ---------------------------------------------------------------- controller and lifecycle

- (void)applyControllerVisibility {
    BOOL connected = NO;
#if !TARGET_OS_SIMULATOR
    // Only hardware controllers hide the touch controls; the simulator can
    // report virtual ones that would hide them during testing.
    for (GCController* controller in GCController.controllers)
        if (controller.extendedGamepad != nil)
            connected = YES;
#endif
    _controllerHidden = connected && BWBoolDefault(kHideOnControllerKey, YES);
    if (connected)
        [self clearTouchInput];
    [self setNeedsLayout];
}

- (void)observeLifecycle {
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self selector:@selector(applyControllerVisibility) name:GCControllerDidConnectNotification object:nil];
    [center addObserver:self selector:@selector(applyControllerVisibility) name:GCControllerDidDisconnectNotification object:nil];
    // Every held touch is released when the app stops being active.
    [center addObserver:self selector:@selector(clearTouchInput) name:UIApplicationWillResignActiveNotification object:nil];
    [center addObserver:self selector:@selector(clearTouchInput) name:UISceneWillDeactivateNotification object:nil];
    // A call or Siri interrupts the audio session: SDL stops and restarts its
    // own output, and the game is held for the length of the interruption so
    // it does not run on silently (PRD FR-014).
    [center addObserver:self selector:@selector(audioInterruption:)
                   name:AVAudioSessionInterruptionNotification object:nil];
}

- (void)audioInterruption:(NSNotification*)note {
    NSNumber* type = note.userInfo[AVAudioSessionInterruptionTypeKey];
    const BOOL began = type != nil && type.unsignedIntegerValue == AVAudioSessionInterruptionTypeBegan;
    fprintf(stderr, "[shell] audio interruption %s\n", began ? "began" : "ended");
    if (began)
        [self clearTouchInput];
    bluewake_pause_set(BLUEWAKE_PAUSE_AUDIO, began);
}

@end

// ---------------------------------------------------------------- install

extern "C" void bluewake_shell_install_overlay(void) {
    void (^install)(void) = ^{
        int count = 0;
        SDL_Window** windows = SDL_GetWindows(&count);
        UIWindow* window = nil;
        if (windows != NULL && count > 0)
            window = (__bridge UIWindow*)SDL_GetPointerProperty(SDL_GetWindowProperties(windows[0]),
                                                                SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, NULL);
        SDL_free(windows);
        UIView* host = window.rootViewController.view;
        if (host == nil) {
            fprintf(stderr, "[shell] no game view to attach to\n");
            return;
        }
        const char* force = getenv("BLUEWAKE_TOUCH_CONTROLS");
        if (force != NULL && force[0] != '\0')
            [[NSUserDefaults standardUserDefaults] setBool:force[0] != '0' forKey:kShowTouchKey];
        BWGameOverlay* overlay = [[BWGameOverlay alloc] initWithFrame:host.bounds];
        overlay.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        [host addSubview:overlay];
        fprintf(stderr, "[shell] touch controls and menu attached (%.0fx%.0f)\n",
                host.bounds.size.width, host.bounds.size.height);
        // Test hook for unattended simulator runs: open the settings panel or
        // the layout editor, which hold the game through the same pause set
        // as the menu, then close it again.
        const char* demo = getenv("BLUEWAKE_SHELL_DEMO");
        // Test hook: BLUEWAKE_TOUCH_TAPS="seconds:control:hold,..." presses
        // touch controls at those times after launch (control is A, B, X, Y,
        // Z, Start, L, R, D_U/D_D/D_L/D_R, move:x,y or c:x,y).
        const char* taps = getenv("BLUEWAKE_TOUCH_TAPS");
        if (taps != NULL && taps[0] != '\0') {
            for (NSString* item in [@(taps) componentsSeparatedByString:@";"]) {
                NSArray<NSString*>* f = [item componentsSeparatedByString:@"@"];
                if (f.count != 3)
                    continue;
                NSString* control = f[1];
                const double at = f[0].doubleValue, hold = f[2].doubleValue;
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(at * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                    [overlay tapIdentifier:control holdSeconds:hold];
                });
            }
        }
        // Test hook: BLUEWAKE_KEY_TAPS="seconds@key@hold;..." presses a
        // hardware-keyboard key at those times after launch (key is a letter,
        // "return", or an SDL scancode number). It registers a keyboard and
        // sends the key the way SDL's GCKeyboard handler does for a real
        // keyboard (SDL_uikitevents.m: SDL_AddKeyboard, then
        // SDL_SendKeyboardKey), so the path from there - SDL's key state,
        // Aurora's keyboard bindings (W/A/S/D stick, J/K/U/I A/B/X/Y, Return
        // Start) and the pad the guest reads - is the one a keyboard drives.
        const char* keys = getenv("BLUEWAKE_KEY_TAPS");
        if (keys != NULL && keys[0] != '\0') {
            const SDL_KeyboardID test_keyboard = 0x424B4559u;  // "BKEY"
            SDL_AddKeyboard(test_keyboard, "BlueWake test keyboard");
            for (NSString* item in [@(keys) componentsSeparatedByString:@";"]) {
                NSArray<NSString*>* f = [item componentsSeparatedByString:@"@"];
                if (f.count != 3)
                    continue;
                NSString* name = f[1].lowercaseString;
                int scancode = -1;
                if (name.length == 1 && [name characterAtIndex:0] >= 'a' && [name characterAtIndex:0] <= 'z')
                    scancode = SDL_SCANCODE_A + ([name characterAtIndex:0] - 'a');
                else if ([name isEqualToString:@"return"])
                    scancode = SDL_SCANCODE_RETURN;
                else if (name.intValue > 0)
                    scancode = name.intValue;
                if (scancode < 0)
                    continue;
                const double at = f[0].doubleValue, hold = f[2].doubleValue;
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(at * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                    SDL_SendKeyboardKey(SDL_GetTicksNS(), test_keyboard, 0, (SDL_Scancode)scancode, true);
                    fprintf(stderr, "[shell] test key %d down\n", scancode);
                    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(hold * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                        SDL_SendKeyboardKey(SDL_GetTicksNS(), test_keyboard, 0, (SDL_Scancode)scancode, false);
                        fprintf(stderr, "[shell] test key %d up\n", scancode);
                    });
                });
            }
        }
        if (demo != NULL && demo[0] != '\0') {
            NSString* which = @(demo);
            // BLUEWAKE_SHELL_DEMO_AT moves the demo from 4 s to that many seconds.
            const char* demo_at = getenv("BLUEWAKE_SHELL_DEMO_AT");
            const double demo_seconds = demo_at != NULL && atof(demo_at) > 0.0 ? atof(demo_at) : 4.0;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(demo_seconds * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                if ([which isEqualToString:@"interruption"]) {
                    // The notifications the system posts for a call or Siri.
                    AVAudioSession* session = [AVAudioSession sharedInstance];
                    [[NSNotificationCenter defaultCenter]
                        postNotificationName:AVAudioSessionInterruptionNotification object:session
                                    userInfo:@{AVAudioSessionInterruptionTypeKey : @(AVAudioSessionInterruptionTypeBegan)}];
                    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(6 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                        [[NSNotificationCenter defaultCenter]
                            postNotificationName:AVAudioSessionInterruptionNotification object:session
                                        userInfo:@{AVAudioSessionInterruptionTypeKey : @(AVAudioSessionInterruptionTypeEnded)}];
                        fprintf(stderr, "[shell] demo interruption ended, pause reasons 0x%x\n", bluewake_pause_reasons());
                    });
                    return;
                }
                if ([which isEqualToString:@"layout"])
                    [overlay beginLayoutEditing];
                else
                    [overlay showSettingsPanel];
                fprintf(stderr, "[shell] demo %s open, pause reasons 0x%x\n", which.UTF8String, bluewake_pause_reasons());
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(8 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                    [overlay endLayoutEditing];
                    [overlay hideSettingsPanel];
                    fprintf(stderr, "[shell] demo closed, pause reasons 0x%x\n", bluewake_pause_reasons());
                });
            });
        }
    };
    if (NSThread.isMainThread)
        install();
    else
        dispatch_async(dispatch_get_main_queue(), install);
}
