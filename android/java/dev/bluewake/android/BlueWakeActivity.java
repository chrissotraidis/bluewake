package dev.bluewake.android;

import android.content.Intent;
import android.graphics.Color;
import android.hardware.input.InputManager;
import android.os.Bundle;
import android.os.PowerManager;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ViewGroup;
import android.view.WindowManager;

import org.libsdl.app.SDLActivity;

import java.util.Map;

/**
 * SDL's activity, with SDL3 linked statically into libmain.so (the host,
 * GXRuntime, Aurora and Dawn): only that one library is loaded. The game
 * module, libgGZLE01_recomp.so, is opened by the host itself (dlopen).
 *
 * Adds BlueWake's shell over SDL's surface (Overlay: the touch controls, the
 * ⋯ menu and its panels, as in the iPhone and iPad app), and passes the
 * player's settings to the game: the ones that apply at launch as environment
 * variables before SDL_main starts, the rest at once (Settings).
 */
public class BlueWakeActivity extends SDLActivity implements InputManager.InputDeviceListener {
    private Settings settings;
    private Overlay overlay;
    private InputManager inputManager;
    private PowerManager power;
    private final PowerManager.OnThermalStatusChangedListener thermal = status -> {
        try {
            Shell.nativeThermal(status);
        } catch (UnsatisfiedLinkError ignored) {
        }
    };

    @Override
    protected String[] getLibraries() {
        return new String[] {"main"};
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        applyLockScreenSwitch(getIntent());
        super.onCreate(savedInstanceState);
        if (mBrokenLibraries || mLayout == null)
            return;  // SDL has said why and closes
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        settings = new Settings(this);
        settings.migrateSettingsIni();
        // The settings that apply at launch, before SDL_main runs; a
        // developer's launch.env still wins (android_entry.c).
        for (Map.Entry<String, String> entry : settings.launchEnvironment().entrySet())
            nativeSetenv(entry.getKey(), entry.getValue());
        settings.applyNow();
        applyRefreshRate();
        mLayout.setBackgroundColor(Color.BLACK);
        overlay = new Overlay(this, settings);
        mLayout.addView(overlay, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        inputManager = (InputManager) getSystemService(INPUT_SERVICE);
        if (inputManager != null)
            inputManager.registerInputDeviceListener(this, null);
        power = (PowerManager) getSystemService(POWER_SERVICE);
        if (power != null) {
            power.addThermalStatusListener(thermal);
            thermal.onThermalStatusChanged(power.getCurrentThermalStatus());
        }
        overlay.setControllerConnected(gamepadCount() > 0);
    }

    // Development over adb only: `am start ... --ez showWhenLocked true` shows
    // the game over the lock screen, so a locked test phone can run it. The
    // switch counts only while files/launch.env exists: the builder and
    // install.py write that file over adb, and no other app can write in this
    // app's storage, so another app's intent cannot bring the game over the
    // lock screen. Every launch decides again: the activity is
    // singleInstance, so a later launch arrives through onNewIntent and an
    // ordinary one turns the switch off.
    private void applyLockScreenSwitch(Intent intent) {
        java.io.File files = getExternalFilesDir(null);
        boolean show = intent != null && intent.getBooleanExtra("showWhenLocked", false)
                && files != null && new java.io.File(files, "launch.env").isFile();
        setShowWhenLocked(show);
        setTurnScreenOn(show);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        applyLockScreenSwitch(intent);
    }

    @Override
    protected void onDestroy() {
        if (inputManager != null)
            inputManager.unregisterInputDeviceListener(this);
        if (power != null)
            power.removeThermalStatusListener(thermal);
        if (overlay != null)
            overlay.onDestroy();
        super.onDestroy();
    }

    @Override
    protected void onPause() {
        if (overlay != null)
            overlay.clearTouchInput();
        super.onPause();
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        if (overlay != null && overlay.onActivityResult(request, result, data))
            return;
        super.onActivityResult(request, result, data);
    }

    /**
     * The Back button or gesture works the ⋯ menu (out of a submenu, the
     * layout editor or the settings panel; with nothing open it opens the
     * menu) and never closes the game. A controller's buttons are left to SDL.
     */
    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (overlay != null && event.getKeyCode() == KeyEvent.KEYCODE_BACK && !fromController(event)) {
            if (event.getAction() == KeyEvent.ACTION_UP && !event.isCanceled())
                overlay.onBack();
            return true;
        }
        if (overlay != null && fromController(event) && event.getAction() == KeyEvent.ACTION_DOWN)
            overlay.onControllerInput();
        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {
        if (overlay != null && controllerMoved(event))
            overlay.onControllerInput();
        return super.dispatchGenericMotionEvent(event);
    }

    /** A stick or the D-pad pushed well off center, not the drift some sticks report at rest. */
    private static boolean controllerMoved(MotionEvent event) {
        if ((event.getSource() & InputDevice.SOURCE_JOYSTICK) != InputDevice.SOURCE_JOYSTICK)
            return false;
        final int[] axes = {MotionEvent.AXIS_X, MotionEvent.AXIS_Y, MotionEvent.AXIS_Z, MotionEvent.AXIS_RZ,
                MotionEvent.AXIS_HAT_X, MotionEvent.AXIS_HAT_Y};
        for (int axis : axes)
            if (Math.abs(event.getAxisValue(axis)) > 0.5f)
                return true;
        return false;
    }

    private static boolean fromController(KeyEvent event) {
        int source = event.getSource();
        return (source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
    }

    /**
     * The game draws 30 frames a second (60 with Smooth Motion at 60): ask
     * the display for 60 Hz instead of its 120, which saves the panel's and
     * the compositor's power. On a phone that power is heat, and heat is what
     * the system's thermal manager lowers the game thread's clock for. Smooth
     * Motion at 120 asks for 120 Hz.
     */
    void applyRefreshRate() {
        WindowManager.LayoutParams attributes = getWindow().getAttributes();
        float rate = settings != null && settings.smoothMotion() == 3 ? 120f : 60f;
        if (attributes.preferredRefreshRate == rate)
            return;
        attributes.preferredRefreshRate = rate;
        getWindow().setAttributes(attributes);
    }

    /** Hardware controllers connected (the built-in screen and keys are not). */
    static int gamepadCount() {
        int count = 0;
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice device = InputDevice.getDevice(id);
            if (device == null || device.isVirtual())
                continue;
            int sources = device.getSources();
            if ((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                    || (sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)
                count++;
        }
        return count;
    }

    private void updateControllers() {
        if (overlay != null)
            overlay.setControllerConnected(gamepadCount() > 0);
    }

    @Override
    public void onInputDeviceAdded(int deviceId) { updateControllers(); }

    @Override
    public void onInputDeviceRemoved(int deviceId) { updateControllers(); }

    @Override
    public void onInputDeviceChanged(int deviceId) { updateControllers(); }
}
