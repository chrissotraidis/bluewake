// BlueWake Android shell, native side.
//
// The Android app is the iPhone and iPad app's shell: the same ⋯ menu, touch
// controls, layout editor and Game Data & Saves actions, drawn by the Java
// classes in android/java/dev/bluewake/android (the counterpart of
// apple/ios/src/BWGameOverlay.mm). Underneath it uses the iPhone app's own glue,
// compiled unchanged: apple/ios/src/touch_controls.cpp (the touch pad, the
// minimum hold, the pause reasons, the FPS numbers and the [fps] log line) and
// controller_apply.cpp (the render settings and a controller's camera and
// buttons). This file stands in for controller_settings.mm (NSUserDefaults)
// and for BWGameOverlay.mm's C entry point, and gives the Java shell its
// native calls (class dev.bluewake.android.Shell).
//
// Settings live in Android's SharedPreferences under the iPhone app's
// NSUserDefaults keys (Settings.java). The menu runs on Android's UI thread
// and the game on SDL's, so a change is handed over with SDL_RunOnMainThread
// and applied there, as the iPhone app applies it on its main thread.
#include <jni.h>

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_init.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>

extern "C" {
#include "atomic_file.h"
#include "card_runtime.h"
#include "gxruntime/memory_card.h"
#include "process_close.h"
}
#include "controller_settings.h"
#include "dolphin_save_import.h"
#include "touch_controls.h"

extern "C" const char* bluewake_game_options_describe(uint32_t position, const char** title, bool* default_on,
                                                      bool* on);

// ---------------------------------------------------------------- settings

static const char* const kRemapNames[BW_REMAP_COUNT] = {"A", "B", "X", "Y", "Z", "Start"};
// Aurora's standard defaults: face buttons by position, right shoulder Z
// (the iPhone app's, controller_settings.mm).
static const unsigned kRemapDefault[BW_REMAP_COUNT] = {
    SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_WEST,
    SDL_GAMEPAD_BUTTON_NORTH, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_START};
static const unsigned kNative[BW_NATIVE_CHOICES] = {
    SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_WEST,
    SDL_GAMEPAD_BUTTON_NORTH, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
    SDL_GAMEPAD_BUTTON_LEFT_STICK, SDL_GAMEPAD_BUTTON_RIGHT_STICK, SDL_GAMEPAD_BUTTON_START,
    SDL_GAMEPAD_BUTTON_BACK};
static const char* const kNativeNames[BW_NATIVE_CHOICES] = {
    "Bottom Face (A / Cross)", "Right Face (B / Circle)", "Left Face (X / Square)",
    "Top Face (Y / Triangle)", "Left Shoulder (LB / L1)", "Right Shoulder (RB / R1)",
    "Left Stick Click", "Right Stick Click", "Menu / Start", "View / Select"};

// What the Java shell last sent; copied into g_bw_settings on SDL's thread.
static std::mutex g_pending_mutex;
static BWSettingsSnapshot g_pending = {0u, 3, 0, false, false,
                                       {SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST,
                                        SDL_GAMEPAD_BUTTON_WEST, SDL_GAMEPAD_BUTTON_NORTH,
                                        SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_START},
                                       false, 0};
static std::atomic<int> g_thermal{0};
static std::atomic<bool> g_shell_ready{false};

BWSettingsSnapshot g_bw_settings = {1u, 3, 0, false, false, {0}, false, 0};

const char* bluewake_remap_name(int i) { return kRemapNames[i]; }
unsigned bluewake_remap_default_native(int i) { return kRemapDefault[i]; }
unsigned bluewake_native_choice(int i) { return kNative[i]; }
const char* bluewake_native_choice_name(int i) { return kNativeNames[i]; }

unsigned bluewake_remap_current_native(int i) {
    std::lock_guard<std::mutex> lock(g_pending_mutex);
    return g_pending.remapped ? g_pending.native[i] : kRemapDefault[i];
}

// The Java menu changes the mapping itself (Settings.java remaps and swaps, as
// controller_settings.mm does) and sends the result; these keep the header's
// contract for code that calls them.
void bluewake_remap_set(int index, unsigned native) {
    {
        std::lock_guard<std::mutex> lock(g_pending_mutex);
        unsigned map[BW_REMAP_COUNT];
        for (int i = 0; i < BW_REMAP_COUNT; i++)
            map[i] = g_pending.remapped ? g_pending.native[i] : kRemapDefault[i];
        const unsigned previous = map[index];
        for (int i = 0; i < BW_REMAP_COUNT; i++)
            if (i != index && map[i] == native)
                map[i] = previous;  // swap, so no button is doubled
        map[index] = native;
        std::memcpy(g_pending.native, map, sizeof map);
        g_pending.remapped = true;
    }
    bluewake_settings_changed();
}

void bluewake_remap_reset(void) {
    {
        std::lock_guard<std::mutex> lock(g_pending_mutex);
        std::memcpy(g_pending.native, kRemapDefault, sizeof kRemapDefault);
        g_pending.remapped = false;
    }
    bluewake_settings_changed();
}

// On SDL's thread: the frame tick (controller_apply.cpp) reads g_bw_settings
// there, and applies it when the generation moves.
void bluewake_settings_changed(void) {
    std::lock_guard<std::mutex> lock(g_pending_mutex);
    const unsigned generation = g_bw_settings.generation;
    g_bw_settings = g_pending;
    g_bw_settings.generation = generation + 1u;
}

// Android's thermal status (PowerManager.getCurrentThermalStatus), sent by the
// activity: 0 none ... 6 shutdown, logged on [fps] and [late] lines.
extern "C" int bluewake_thermal_state(void) { return g_thermal.load(); }

// touch_controls.cpp calls this on the first presented frame: the iPhone app
// attaches its UIKit overlay here. The Java overlay is already up; it only
// learns that the game runs (Better Wind Waker's settings come from the game
// module through the host).
extern "C" void bluewake_shell_install_overlay(void) {
    g_shell_ready = true;
    std::fprintf(stderr, "[android] shell attached to the game\n");
}

static void SDLCALL apply_on_sdl_thread(void*) { bluewake_settings_changed(); }

// ---------------------------------------------------------------- JNI

#define SHELL(name) JNICALL Java_dev_bluewake_android_Shell_##name

static const char* utf(JNIEnv* env, jstring s) { return s != nullptr ? env->GetStringUTFChars(s, nullptr) : nullptr; }

extern "C" {

JNIEXPORT void SHELL(nativeTouchPublish)(JNIEnv*, jclass, jint buttons, jint sx, jint sy, jint cx, jint cy) {
    auto clamp = [](jint v) { return (int8_t)(v > 127 ? 127 : v < -127 ? -127 : v); };
    BlueWakeTouchPad pad{};
    pad.buttons = (uint16_t)buttons;
    pad.stick_x = clamp(sx);
    pad.stick_y = clamp(sy);
    pad.c_stick_x = clamp(cx);
    pad.c_stick_y = clamp(cy);
    bluewake_touch_publish(&pad);
}

JNIEXPORT void SHELL(nativeTouchClear)(JNIEnv*, jclass) { bluewake_touch_clear(); }

JNIEXPORT void SHELL(nativePauseSet)(JNIEnv*, jclass, jint reason, jboolean on) {
    bluewake_pause_set((unsigned)reason, on == JNI_TRUE);
}

JNIEXPORT jint SHELL(nativePauseReasons)(JNIEnv*, jclass) { return (jint)bluewake_pause_reasons(); }

// shown, speed, worst_ms, display, game, smooth_paused (1 or 0).
JNIEXPORT void SHELL(nativeFps)(JNIEnv* env, jclass, jfloatArray out) {
    float v[6] = {0};
    bluewake_fps_read(&v[0], &v[1], &v[2]);
    v[3] = bluewake_fps_display();
    v[4] = bluewake_fps_game();
    v[5] = bluewake_fps_smooth_paused() ? 1.0f : 0.0f;
    if (out != nullptr && env->GetArrayLength(out) >= 6)
        env->SetFloatArrayRegion(out, 0, 6, v);
}

// natives is the GameCube A, B, X, Y, Z, Start mapping (SDL gamepad buttons),
// or null for the controller's defaults.
JNIEXPORT void SHELL(nativeApplySettings)(JNIEnv* env, jclass, jint render_scale, jint anisotropy,
                                          jboolean invert_x, jboolean invert_y, jintArray natives,
                                          jint smooth_motion) {
    {
        std::lock_guard<std::mutex> lock(g_pending_mutex);
        g_pending.render_scale = render_scale;
        g_pending.anisotropy = anisotropy;
        g_pending.invert_x = invert_x == JNI_TRUE;
        g_pending.invert_y = invert_y == JNI_TRUE;
        g_pending.smooth_motion = smooth_motion == 1 || smooth_motion == 3 ? smooth_motion : 0;
        g_pending.remapped = natives != nullptr && env->GetArrayLength(natives) == BW_REMAP_COUNT;
        if (g_pending.remapped) {
            jint map[BW_REMAP_COUNT];
            env->GetIntArrayRegion(natives, 0, BW_REMAP_COUNT, map);
            for (int i = 0; i < BW_REMAP_COUNT; i++)
                g_pending.native[i] = (unsigned)map[i];
        } else {
            std::memcpy(g_pending.native, kRemapDefault, sizeof kRemapDefault);
        }
    }
    // Before the game starts SDL's thread is not pumping events yet; its first
    // frame reads the pending settings anyway (controller_apply.cpp).
    if (g_shell_ready.load() && !SDL_RunOnMainThread(apply_on_sdl_thread, nullptr, false))
        std::fprintf(stderr, "[android] settings change not handed to SDL: %s\n", SDL_GetError());
}

JNIEXPORT void SHELL(nativeThermal)(JNIEnv*, jclass, jint status) { g_thermal = status; }

JNIEXPORT jboolean SHELL(nativeGameRunning)(JNIEnv*, jclass) { return g_shell_ready.load() ? JNI_TRUE : JNI_FALSE; }

// Better Wind Waker's option at position: {name, title, default_on, on}, or
// null past the last one (runtime/host/src/game_options.h).
JNIEXPORT jobjectArray SHELL(nativeGameOption)(JNIEnv* env, jclass, jint position) {
    const char* title = nullptr;
    bool default_on = false, on = false;
    const char* name = bluewake_game_options_describe((uint32_t)position, &title, &default_on, &on);
    if (name == nullptr)
        return nullptr;
    jobjectArray out = env->NewObjectArray(4, env->FindClass("java/lang/String"), nullptr);
    env->SetObjectArrayElement(out, 0, env->NewStringUTF(name));
    env->SetObjectArrayElement(out, 1, env->NewStringUTF(title != nullptr ? title : name));
    env->SetObjectArrayElement(out, 2, env->NewStringUTF(default_on ? "1" : "0"));
    env->SetObjectArrayElement(out, 3, env->NewStringUTF(on ? "1" : "0"));
    return out;
}

JNIEXPORT jboolean SHELL(nativeCardValidate)(JNIEnv* env, jclass, jstring path) {
    const char* p = utf(env, path);
    const bool ok = p != nullptr && dol_card_validate(p);
    if (p != nullptr) env->ReleaseStringUTFChars(path, p);
    return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void SHELL(nativeSuspendCardWrites)(JNIEnv*, jclass, jboolean suspend) {
    bluewake_card_runtime_suspend_writes(suspend == JNI_TRUE);
}

JNIEXPORT jboolean SHELL(nativeFlushPath)(JNIEnv* env, jclass, jstring path) {
    const char* p = utf(env, path);
    const bool ok = p != nullptr && bw_atomic_flush_path(p);
    if (p != nullptr) env->ReleaseStringUTFChars(path, p);
    return ok ? JNI_TRUE : JNI_FALSE;
}

// The running game keeps the card in memory and writes all of it on its next
// save, so after a restore or an import the app closes (the iPhone app's
// "Close BlueWake"): the card's pending writes finish, then the process ends.
JNIEXPORT void SHELL(nativeCloseForSaves)(JNIEnv*, jclass) {
    std::fprintf(stderr, "[android] closing for the replaced saves\n");
    bw_process_close(bluewake_card_runtime_close, 0);
}

JNIEXPORT void SHELL(nativeLog)(JNIEnv* env, jclass, jstring line) {
    const char* s = utf(env, line);
    if (s != nullptr) {
        std::fprintf(stderr, "%s\n", s);
        env->ReleaseStringUTFChars(line, s);
    }
}

// Quest logs as {empty, checksum_ok, name, max_life, rupees} rows, one per
// slot: for a Dolphin file (dolphin != 0: .gci or .raw) or for BlueWake's card.
// Returns null with the reason in error[0] when the bytes cannot be read; for
// a card with no saves yet, an empty array.
static jobjectArray quest_logs(JNIEnv* env, const BWQuestLog logs[BW_QUEST_LOGS]) {
    jobjectArray out = env->NewObjectArray(BW_QUEST_LOGS, env->FindClass("java/lang/String"), nullptr);
    for (int i = 0; i < BW_QUEST_LOGS; i++) {
        char row[96];
        std::snprintf(row, sizeof row, "%d\t%d\t%u\t%u\t%s", logs[i].empty ? 1 : 0, logs[i].checksum_ok ? 1 : 0,
                      logs[i].max_life, logs[i].rupees, logs[i].name);
        env->SetObjectArrayElement(out, i, env->NewStringUTF(row));
    }
    return out;
}

JNIEXPORT jobjectArray SHELL(nativeQuestLogs)(JNIEnv* env, jclass, jbyteArray bytes, jboolean dolphin,
                                              jobjectArray error) {
    const jsize size = bytes != nullptr ? env->GetArrayLength(bytes) : 0;
    jbyte* data = bytes != nullptr ? env->GetByteArrayElements(bytes, nullptr) : nullptr;
    BWQuestLog logs[BW_QUEST_LOGS] = {};
    const char* reason = nullptr;
    jobjectArray out = nullptr;
    if (data == nullptr) {
        reason = "The file could not be read.";
    } else if (dolphin == JNI_TRUE) {
        BWDolphinSave save = {};
        reason = bw_dolphin_save_parse((const uint8_t*)data, (size_t)size, &save);
        if (reason == nullptr) {
            bw_gczelda_quest_logs(save.data, logs);
            out = quest_logs(env, logs);
        }
        bw_dolphin_save_free(&save);
    } else {
        bool has_saves = false;
        reason = bw_card_quest_logs((const uint8_t*)data, (size_t)size, &has_saves, logs);
        if (reason == nullptr)
            out = has_saves ? quest_logs(env, logs)
                            : env->NewObjectArray(0, env->FindClass("java/lang/String"), nullptr);
    }
    if (data != nullptr) env->ReleaseByteArrayElements(bytes, data, JNI_ABORT);
    if (reason != nullptr && error != nullptr && env->GetArrayLength(error) > 0)
        env->SetObjectArrayElement(error, 0, env->NewStringUTF(reason));
    return reason != nullptr ? nullptr : out;
}

// The card with Dolphin quest log source put in slot destination (both 0: the
// card has no saves yet and the whole Dolphin file goes in), or null with the
// reason in error[0].
JNIEXPORT jbyteArray SHELL(nativeCardImport)(JNIEnv* env, jclass, jbyteArray card_bytes, jbyteArray save_bytes,
                                             jint source, jint destination, jobjectArray error) {
    jbyte* card = env->GetByteArrayElements(card_bytes, nullptr);
    jbyte* file = env->GetByteArrayElements(save_bytes, nullptr);
    BWDolphinSave save = {};
    uint8_t* result = nullptr;
    size_t result_size = 0;
    const char* reason = bw_dolphin_save_parse((const uint8_t*)file, (size_t)env->GetArrayLength(save_bytes), &save);
    if (reason == nullptr)
        reason = bw_card_import((const uint8_t*)card, (size_t)env->GetArrayLength(card_bytes), &save, source,
                                destination, &result, &result_size);
    bw_dolphin_save_free(&save);
    env->ReleaseByteArrayElements(card_bytes, card, JNI_ABORT);
    env->ReleaseByteArrayElements(save_bytes, file, JNI_ABORT);
    if (reason != nullptr) {
        if (error != nullptr && env->GetArrayLength(error) > 0)
            env->SetObjectArrayElement(error, 0, env->NewStringUTF(reason));
        std::free(result);
        return nullptr;
    }
    jbyteArray out = env->NewByteArray((jsize)result_size);
    env->SetByteArrayRegion(out, 0, (jsize)result_size, (const jbyte*)result);
    std::free(result);
    return out;
}

}  // extern "C"
