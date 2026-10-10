package dev.bluewake.android;

/**
 * The native side of the shell (android/src/android_shell.cpp): the iPhone app's touch pad, pause reasons, FPS
 * numbers and settings glue (apple/ios/src/touch_controls.cpp, controller_apply.cpp), and the memory card and
 * Dolphin save functions Game Data & Saves uses.
 */
final class Shell {
    private Shell() {}

    // Touch button bits (apple/ios/src/touch_controls.h).
    static final int DPAD_LEFT = 1, DPAD_RIGHT = 1 << 1, DPAD_DOWN = 1 << 2, DPAD_UP = 1 << 3;
    static final int Z = 1 << 4, R = 1 << 5, L = 1 << 6;
    static final int A = 1 << 8, B = 1 << 9, X = 1 << 10, Y = 1 << 11, START = 1 << 12;
    static final int JUMP = 1 << 13, SPRINT = 1 << 14;

    // Why the game is held; it runs only while none is set (touch_controls.h).
    static final int PAUSE_INACTIVE = 1, PAUSE_MENU = 1 << 1, PAUSE_SETTINGS = 1 << 2, PAUSE_LAYOUT = 1 << 3,
            PAUSE_ALERT = 1 << 4, PAUSE_AUDIO = 1 << 5;

    /** Sticks are -127..127 with +y up. */
    static native void nativeTouchPublish(int buttons, int stickX, int stickY, int cX, int cY);
    static native void nativeTouchClear();
    static native void nativePauseSet(int reason, boolean on);
    static native int nativePauseReasons();
    /** shown, speed, worst_ms, display, game, smooth_paused (1 or 0). */
    static native void nativeFps(float[] out);
    /** natives: GameCube A, B, X, Y, Z, Start as SDL gamepad buttons, or null for the defaults. */
    static native void nativeApplySettings(int renderScale, int anisotropy, boolean invertX, boolean invertY,
                                           int[] natives, int smoothMotion);
    static native void nativeThermal(int status);
    static native boolean nativeGameRunning();
    /** Better Wind Waker's option at a position: {name, title, default_on, on} ("1" or "0"), or null. */
    static native String[] nativeGameOption(int position);
    static native boolean nativeCardValidate(String path);
    static native void nativeSuspendCardWrites(boolean suspend);
    static native boolean nativeFlushPath(String path);
    static native void nativeCloseForSaves();
    static native void nativeLog(String line);
    /** Rows "empty\tchecksum_ok\tmax_life\trupees\tname" for each quest log, or null with error[0] set. */
    static native String[] nativeQuestLogs(byte[] bytes, boolean dolphin, String[] error);
    static native byte[] nativeCardImport(byte[] card, byte[] save, int source, int destination, String[] error);

    static void log(String line) {
        try {
            nativeLog(line);
        } catch (UnsatisfiedLinkError ignored) {
            // Before libmain.so is loaded.
        }
    }
}
