package dev.bluewake.android;

import android.content.Context;
import android.content.SharedPreferences;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * The player's settings, under the iPhone and iPad app's NSUserDefaults keys
 * (apple/ios/src/BWGameOverlay.mm, controller_settings.h), so the two apps keep
 * the same options with the same names and defaults. Some apply at once
 * (Shell.nativeApplySettings); the rest apply at launch, as environment
 * variables set before the game starts (launchEnvironment, the counterpart of
 * apple/ios/src/ios_entry.m).
 */
final class Settings {
    static final String OPACITY = "BlueWake.ControlOpacity";
    static final String SIZE = "BlueWake.ControlSize";
    static final String HIDE_ON_CONTROLLER = "BlueWake.HideOnController";
    static final String SHOW_TOUCH = "BlueWake.ShowTouchControls";
    static final String SHOW_FPS = "BlueWake.ShowFPS";
    static final String MOVEMENT_EXTRAS = "BlueWake.MovementExtras";
    static final String FAST_TRANSITIONS = "BlueWake.FastTransitions";
    static final String QUICK_DOORS = "BlueWake.QuickDoors";
    static final String ASPECT_MODE = "BlueWake.AspectMode";
    static final String RENDER_SCALE = "BlueWake.RenderScale";
    static final String ANISOTROPY = "BlueWake.Anisotropy";
    static final String SMOOTH_MOTION = "BlueWake.SmoothMotion";
    static final String INVERT_X = "BlueWake.InvertCameraX";
    static final String INVERT_Y = "BlueWake.InvertCameraY";
    static final String BUTTON_MAP = "BlueWake.ButtonMap";
    static final String MOD_WIDESCREEN = "BlueWake.Mod.Widescreen";
    static final String MOD_WIDESCREEN_1610 = "BlueWake.Mod.Widescreen1610";
    static final String MOD_HD_TEXTURES = "BlueWake.Mod.HDTextures";
    static final String MOD_BETTERWW = "BlueWake.Mod.BetterWW";
    static final String OPTION_PREFIX = "BlueWake.Option.";
    private static final String MIGRATED = "BlueWake.Android.MigratedSettingsIni";

    // The controller buttons GameCube A, B, X, Y, Z and Start can come from
    // (SDL gamepad buttons; controller_settings.mm's tables).
    static final String[] REMAP_NAMES = {"A", "B", "X", "Y", "Z", "Start"};
    static final int SDL_SOUTH = 0, SDL_EAST = 1, SDL_WEST = 2, SDL_NORTH = 3, SDL_BACK = 4, SDL_START = 6,
            SDL_LEFT_STICK = 7, SDL_RIGHT_STICK = 8, SDL_LEFT_SHOULDER = 9, SDL_RIGHT_SHOULDER = 10;
    static final int[] REMAP_DEFAULT = {SDL_SOUTH, SDL_EAST, SDL_WEST, SDL_NORTH, SDL_RIGHT_SHOULDER, SDL_START};
    static final int[] NATIVE_CHOICES = {SDL_SOUTH, SDL_EAST, SDL_WEST, SDL_NORTH, SDL_LEFT_SHOULDER,
            SDL_RIGHT_SHOULDER, SDL_LEFT_STICK, SDL_RIGHT_STICK, SDL_START, SDL_BACK};
    static final String[] NATIVE_NAMES = {"Bottom Face (A / Cross)", "Right Face (B / Circle)",
            "Left Face (X / Square)", "Top Face (Y / Triangle)", "Left Shoulder (LB / L1)",
            "Right Shoulder (RB / R1)", "Left Stick Click", "Right Stick Click", "Menu / Start", "View / Select"};

    final SharedPreferences prefs;
    final File dataDir;

    Settings(Context context) {
        prefs = context.getSharedPreferences("BlueWake", Context.MODE_PRIVATE);
        File external = context.getExternalFilesDir(null);
        dataDir = external != null ? external : context.getFilesDir();
    }

    boolean bool(String key, boolean fallback) { return prefs.getBoolean(key, fallback); }
    float number(String key, float fallback) { return prefs.getFloat(key, fallback); }
    int integer(String key, int fallback) { return prefs.getInt(key, fallback); }
    boolean has(String key) { return prefs.contains(key); }
    void set(String key, boolean value) { prefs.edit().putBoolean(key, value).apply(); }
    void set(String key, float value) { prefs.edit().putFloat(key, value).apply(); }
    void set(String key, int value) { prefs.edit().putInt(key, value).apply(); }
    void remove(String key) { prefs.edit().remove(key).apply(); }

    int renderScale() { return integer(RENDER_SCALE, 3); }
    int anisotropy() { return Math.max(1, integer(ANISOTROPY, 1)); }
    int smoothMotion() {
        int smooth = integer(SMOOTH_MOTION, 0);
        return smooth == 1 || smooth == 3 ? smooth : 0;
    }

    // ------------------------------------------------------------ button mapping

    boolean remapped() { return has(BUTTON_MAP); }

    int remapCurrent(int index) {
        try {
            JSONObject map = new JSONObject(prefs.getString(BUTTON_MAP, "{}"));
            return map.has(REMAP_NAMES[index]) ? map.getInt(REMAP_NAMES[index]) : REMAP_DEFAULT[index];
        } catch (JSONException e) {
            return REMAP_DEFAULT[index];
        }
    }

    /** Maps GameCube button index to a controller button; one already in use is swapped. */
    void remapSet(int index, int nativeButton) {
        int[] map = new int[REMAP_NAMES.length];
        for (int i = 0; i < map.length; i++) map[i] = remapCurrent(i);
        int previous = map[index];
        for (int i = 0; i < map.length; i++)
            if (i != index && map[i] == nativeButton) map[i] = previous;
        map[index] = nativeButton;
        JSONObject json = new JSONObject();
        try {
            for (int i = 0; i < map.length; i++) json.put(REMAP_NAMES[i], map[i]);
        } catch (JSONException e) {
            return;
        }
        prefs.edit().putString(BUTTON_MAP, json.toString()).apply();
    }

    void remapReset() { remove(BUTTON_MAP); }

    static String nativeName(int nativeButton) {
        for (int i = 0; i < NATIVE_CHOICES.length; i++)
            if (NATIVE_CHOICES[i] == nativeButton) return NATIVE_NAMES[i];
        return "";
    }

    /** Sends the settings that apply at once to the game (controller_apply.cpp applies them). */
    void applyNow() {
        int[] natives = null;
        if (remapped()) {
            natives = new int[REMAP_NAMES.length];
            for (int i = 0; i < natives.length; i++) natives[i] = remapCurrent(i);
        }
        Shell.nativeApplySettings(renderScale(), integer(ANISOTROPY, 0), bool(INVERT_X, false),
                bool(INVERT_Y, false), natives, smoothMotion());
    }

    // ------------------------------------------------------------ touch layout

    /** Layouts are kept per form factor and schema version (BWGameOverlay.mm BWLayoutKey). */
    static String layoutKey(boolean phone, String name) {
        return "BlueWake." + (phone ? "phone" : "tablet") + ".v1." + name;
    }

    JSONObject json(String key) {
        try {
            return new JSONObject(prefs.getString(key, "{}"));
        } catch (JSONException e) {
            return new JSONObject();
        }
    }

    void putJson(String key, JSONObject value) { prefs.edit().putString(key, value.toString()).apply(); }

    // ------------------------------------------------------------ at launch

    File texturePackDirectory() { return new File(dataDir, "Load/Textures/GZLE01"); }

    /**
     * The launch-time settings as environment variables (apple/ios/src/ios_entry.m's), set before the game
     * starts. android_entry.c fills in the rest; a developer's launch.env still wins over both.
     */
    Map<String, String> launchEnvironment() {
        Map<String, String> env = new LinkedHashMap<>();
        env.put("DOL_AURORA_RENDER_SCALE", Integer.toString(renderScale()));
        if (integer(ASPECT_MODE, 0) == 1) env.put("DOL_AURORA_ASPECT_FIT", "0");
        boolean movement = bool(MOVEMENT_EXTRAS, false);
        env.put("BLUEWAKE_JUMP_BUTTON", movement ? "1" : "0");
        env.put("BLUEWAKE_SPRINT_SPEED", movement ? "1.5" : "1");
        boolean fast = bool(FAST_TRANSITIONS, false);
        env.put("BLUEWAKE_FADE_FRAMES", fast ? "6" : "0");
        env.put("BLUEWAKE_FAST_FORWARD", fast ? "1" : "0");
        env.put("BLUEWAKE_QUICK_DOORS", bool(QUICK_DOORS, false) ? "1" : "0");
        List<String> mods = new ArrayList<>();
        if (bool(MOD_WIDESCREEN_1610, false)) {
            mods.add("widescreen1610");
            env.put("DOL_AURORA_ASPECT_RATIO", "1.6");
        } else if (bool(MOD_WIDESCREEN, false)) {
            mods.add("widescreen");
            env.put("DOL_AURORA_ASPECT_RATIO", "1.7778");
        }
        File pack = texturePackDirectory();
        //noinspection ResultOfMethodCallIgnored
        pack.mkdirs();
        if (bool(MOD_HD_TEXTURES, false)) env.put("DOL_AURORA_TEXTURE_PACK", pack.getAbsolutePath());
        if (bool(MOD_BETTERWW, false)) {
            // android_entry.c keeps it only if the game module was built with
            // Better Wind Waker's options (bluewake_composite_option_count).
            mods.add("betterww");
            List<String> options = new ArrayList<>();
            for (Map.Entry<String, ?> entry : prefs.getAll().entrySet()) {
                if (!entry.getKey().startsWith(OPTION_PREFIX) || !(entry.getValue() instanceof Boolean)) continue;
                String name = entry.getKey().substring(OPTION_PREFIX.length());
                options.add((Boolean) entry.getValue() ? name : "-" + name);
            }
            if (!options.isEmpty()) env.put("BLUEWAKE_OPTIONS", String.join(",", options));
        }
        if (!mods.isEmpty()) env.put("BLUEWAKE_MODS", String.join(",", mods));
        return env;
    }

    /**
     * Once: the choices an earlier Android build kept in the desktop menu's settings.ini become these settings,
     * so an update keeps the player's resolution, filtering, aspect, Smooth Motion and mods. The file is left
     * where it is.
     */
    void migrateSettingsIni() {
        if (bool(MIGRATED, false)) return;
        File ini = new File(dataDir, "settings.ini");
        Map<String, String> v = new HashMap<>();
        if (ini.isFile()) {
            try (BufferedReader reader = new BufferedReader(new FileReader(ini))) {
                for (String line; (line = reader.readLine()) != null; ) {
                    int eq = line.indexOf('=');
                    if (line.startsWith("#") || eq <= 0) continue;
                    v.put(line.substring(0, eq).trim(), line.substring(eq + 1).trim());
                }
            } catch (IOException ignored) {
            }
        }
        SharedPreferences.Editor e = prefs.edit();
        try {
            if (v.containsKey("DOL_AURORA_RENDER_SCALE") && !v.get("DOL_AURORA_RENDER_SCALE").isEmpty())
                e.putInt(RENDER_SCALE, Math.max(0, Math.min(4, Integer.parseInt(v.get("DOL_AURORA_RENDER_SCALE")))));
            if (v.containsKey("DOL_AURORA_FORCE_ANISO") && !v.get("DOL_AURORA_FORCE_ANISO").isEmpty()) {
                int a = Integer.parseInt(v.get("DOL_AURORA_FORCE_ANISO"));
                e.putInt(ANISOTROPY, a <= 1 ? 1 : a <= 4 ? 4 : a <= 8 ? 8 : 16);
            }
        } catch (NumberFormatException ignored) {
        }
        if ("1".equals(v.get("DOL_AURORA_SHOW_FPS"))) e.putBoolean(SHOW_FPS, true);
        if ("0".equals(v.get("DOL_AURORA_ASPECT_FIT"))) e.putInt(ASPECT_MODE, 1);
        if ("1".equals(v.get("DOL_AURORA_FRAME_INTERP")))
            e.putInt(SMOOTH_MOTION, "1".equals(v.get("DOL_AURORA_FRAME_INTERP_STEPS")) ? 1 : 3);
        if ("1".equals(v.get("BLUEWAKE_QUICK_DOORS"))) e.putBoolean(QUICK_DOORS, true);
        if ("1".equals(v.get("BLUEWAKE_JUMP_BUTTON"))) e.putBoolean(MOVEMENT_EXTRAS, true);
        String fade = v.get("BLUEWAKE_FADE_FRAMES");
        if (fade != null && !fade.isEmpty() && !"0".equals(fade)) e.putBoolean(FAST_TRANSITIONS, true);
        String mods = v.get("BLUEWAKE_MODS");
        if (mods != null) {
            for (String mod : mods.split(",")) {
                if (mod.equals("widescreen")) e.putBoolean(MOD_WIDESCREEN, true);
                if (mod.equals("widescreen1610")) e.putBoolean(MOD_WIDESCREEN_1610, true);
                if (mod.equals("betterww")) e.putBoolean(MOD_BETTERWW, true);
            }
        }
        String options = v.get("BLUEWAKE_OPTIONS");
        if (options != null)
            for (String name : options.split(","))
                if (!name.isEmpty() && !name.equals("none") && !name.startsWith("-"))
                    e.putBoolean(OPTION_PREFIX + name, true);
        e.putBoolean(MIGRATED, true).apply();
    }
}
