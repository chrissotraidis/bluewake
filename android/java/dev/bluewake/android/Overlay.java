package dev.bluewake.android;

import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Insets;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.PowerManager;
import android.util.TypedValue;
import android.view.ContextThemeWrapper;
import android.view.Display;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowInsets;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * BlueWake's shell over the game, as the iPhone and iPad app has it (apple/ios/src/BWGameOverlay.mm): the touch
 * controls, the ⋯ menu with the same sections, names and order, the FPS label, the Touch Controls panel, the
 * layout editor, and the alerts. Each one holds the game while it is open, through the same pause reasons
 * (touch_controls.h).
 *
 * One thing the iPhone app does not need: on a near-square screen (a foldable's inner screen) the 4:3 picture
 * leaves no room beside it for the controls, so while they are shown the game's surface is narrowed until each
 * side has a column, and the controls never cover the picture or its HUD.
 */
final class Overlay extends FrameLayout implements TouchControlsView.Listener, MenuView.Listener {
    static final String GITHUB_URL = "https://github.com/chrissotraidis/bluewake";

    /** A choice from a list of options. */
    interface Choice {
        void chose(int index);
    }

    /** A wait alert while work runs off the main thread. */
    final class Wait {
        private final AlertDialog dialog;
        Wait(AlertDialog dialog) { this.dialog = dialog; }
        void setMessage(String message) { post(() -> dialog.setMessage(message)); }
        void dismiss() { post(dialog::dismiss); }
    }

    private final BlueWakeActivity activity;
    private final Settings settings;
    private final GameData data;
    private final Context themed;
    private final float dp;
    private final Handler handler = new Handler(Looper.getMainLooper());

    private final TouchControlsView touch;
    private final TextView fpsLabel;
    private final MenuButton menuButton;
    private final ScrollView settingsPanel;
    private final SeekBar opacitySlider, sizeSlider;
    private final Switch hideSwitch, editSwitch;
    private final LinearLayout editorBar;
    private final TextView editorHint;
    private final SeekBar selectedSizeSlider;
    private final MenuView menu;

    private final int launchAspectMode;      // the aspect the renderer started with
    private final float launchRatio;         // the picture's shape this launch (4:3, or a widescreen mod's)
    private final Map<String, Boolean> launchMods = new HashMap<>();  // mod switches as this launch read them
    private final RectF safe = new RectF(), picture = new RectF();
    private boolean phone = true, controllerConnected, controllerHidden, settingsOpen, menuDumped, updatingSliders;
    private int alertDepth;
    private int packFiles = -1;
    private int panelMaxHeight;
    private boolean fpsWasPaused;
    private long fpsPauseEnded;

    private final Runnable fpsTick = new Runnable() {
        @Override
        public void run() {
            updateFPSLabel();
            handler.postDelayed(this, 500);
        }
    };

    Overlay(BlueWakeActivity activity, Settings settings) {
        super(activity);
        this.activity = activity;
        this.settings = settings;
        themed = new ContextThemeWrapper(activity, android.R.style.Theme_DeviceDefault_NoActionBar);
        dp = getResources().getDisplayMetrics().density;
        data = new GameData(activity, this, settings);
        launchAspectMode = settings.integer(Settings.ASPECT_MODE, 0);
        launchRatio = settings.bool(Settings.MOD_WIDESCREEN_1610, false) ? 1.6f
                : settings.bool(Settings.MOD_WIDESCREEN, false) ? 16f / 9f : 4f / 3f;
        for (String key : new String[] {Settings.MOD_WIDESCREEN, Settings.MOD_WIDESCREEN_1610,
                Settings.MOD_HD_TEXTURES, Settings.MOD_BETTERWW})
            launchMods.put(key, settings.bool(key, false));

        touch = new TouchControlsView(activity, settings);
        touch.setListener(this);
        addView(touch, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));

        fpsLabel = new TextView(themed);
        fpsLabel.setTextSize(TypedValue.COMPLEX_UNIT_DIP, 13f);
        fpsLabel.setTypeface(Typeface.create("sans-serif-medium", Typeface.NORMAL));
        fpsLabel.setFontFeatureSettings("tnum");
        fpsLabel.setTextColor(Color.WHITE);
        fpsLabel.setGravity(Gravity.CENTER);
        fpsLabel.setSingleLine(true);
        fpsLabel.setEllipsize(android.text.TextUtils.TruncateAt.END);
        fpsLabel.setPadding(px(12), 0, px(12), 0);
        fpsLabel.setBackground(rounded(Color.argb(140, 0, 0, 0), 8f, 0, 0));
        fpsLabel.setVisibility(GONE);
        // As wide as its text, at the top center.
        LayoutParams fpsParams = new LayoutParams(LayoutParams.WRAP_CONTENT, px(28));
        fpsParams.gravity = Gravity.TOP | Gravity.CENTER_HORIZONTAL;
        addView(fpsLabel, fpsParams);

        menuButton = new MenuButton(activity);
        menuButton.setContentDescription("Menu");
        menuButton.setOnClickListener(v -> openMenu());
        addView(menuButton, topLeft());

        // Touch Controls settings
        LinearLayout stack = new LinearLayout(themed);
        stack.setOrientation(LinearLayout.VERTICAL);
        stack.setPadding(px(16), px(8), px(12), px(14));
        LinearLayout header = new LinearLayout(themed);
        header.setGravity(Gravity.CENTER_VERTICAL);
        TextView title = label("Touch Controls", 17f, true);
        header.addView(title, new LinearLayout.LayoutParams(0, px(40), 1f));
        TextView close = label("✕", 15f, true);
        close.setGravity(Gravity.CENTER);
        close.setBackground(rounded(Color.argb(36, 255, 255, 255), 16f, 0, 0));
        close.setContentDescription("Close touch control settings");
        close.setOnClickListener(v -> hideSettingsPanel());
        header.addView(close, new LinearLayout.LayoutParams(px(32), px(32)));
        stack.addView(header);
        opacitySlider = slider();
        opacitySlider.setContentDescription("Control opacity");
        opacitySlider.setOnSeekBarChangeListener(sliderListener(value -> {
            settings.set(Settings.OPACITY, 0.25f + value * 0.75f);
            touch.invalidate();
        }));
        sizeSlider = slider();
        sizeSlider.setContentDescription("Control size");
        sizeSlider.setOnSeekBarChangeListener(sliderListener(value -> {
            settings.set(Settings.SIZE, 0.70f + value * 0.65f);
            touch.relayout();
        }));
        hideSwitch = switchControl();
        hideSwitch.setContentDescription("Hide touch controls when a controller is connected");
        hideSwitch.setOnCheckedChangeListener((b, on) -> {
            if (updatingSliders) return;
            settings.set(Settings.HIDE_ON_CONTROLLER, on);
            applyControllerVisibility();
        });
        editSwitch = switchControl();
        editSwitch.setContentDescription("Move touch controls");
        editSwitch.setOnCheckedChangeListener((b, on) -> {
            if (updatingSliders) return;
            if (on) beginLayoutEditing();
            else endLayoutEditing();
        });
        stack.addView(row("Opacity", opacitySlider));
        stack.addView(row("Size", sizeSlider));
        stack.addView(row("Hide with a controller", hideSwitch));
        stack.addView(row("Move controls", editSwitch));
        Button reset = new Button(themed);
        reset.setText("Reset This Device's Layout");
        reset.setAllCaps(false);
        reset.setTextColor(Color.WHITE);
        reset.setBackground(rounded(Color.argb(224, 46, 46, 46), 10f, 0, 0));
        reset.setOnClickListener(v -> confirmResetLayout());
        LinearLayout.LayoutParams resetParams = new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, px(40));
        resetParams.topMargin = px(10);
        stack.addView(reset, resetParams);
        // As tall as its rows, up to what the screen has room for; it scrolls past that.
        settingsPanel = new ScrollView(themed) {
            @Override
            protected void onMeasure(int widthSpec, int heightSpec) {
                int most = MeasureSpec.makeMeasureSpec(Math.max(0, panelMaxHeight), MeasureSpec.AT_MOST);
                super.onMeasure(widthSpec, most);
            }
        };
        settingsPanel.addView(stack);
        settingsPanel.setBackground(rounded(Color.argb(240, 9, 9, 9), 16f, 1f, Color.argb(56, 255, 255, 255)));
        settingsPanel.setClickable(true);
        settingsPanel.setVisibility(GONE);
        addView(settingsPanel, topLeft());

        // The layout editor's bar
        editorBar = new LinearLayout(themed);
        editorBar.setGravity(Gravity.CENTER_VERTICAL);
        editorBar.setPadding(px(14), px(8), px(10), px(8));
        editorBar.setBackground(rounded(Color.argb(242, 9, 9, 9), 16f, 1f, Color.argb(242, 255, 199, 51)));
        editorBar.setClickable(true);
        editorHint = label("Drag controls • tap one to resize", 14f, true);
        editorHint.setSingleLine(true);
        editorHint.setEllipsize(android.text.TextUtils.TruncateAt.END);
        editorBar.addView(editorHint, new LinearLayout.LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f));
        selectedSizeSlider = slider();
        selectedSizeSlider.setEnabled(false);
        selectedSizeSlider.setOnSeekBarChangeListener(sliderListener(value ->
                touch.setSelectedScale(0.60f + value * 1.15f)));
        LinearLayout.LayoutParams sliderParams = new LinearLayout.LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f);
        sliderParams.leftMargin = px(12);
        sliderParams.rightMargin = px(12);
        editorBar.addView(selectedSizeSlider, sliderParams);
        Button done = new Button(themed);
        done.setText("Done");
        done.setAllCaps(false);
        done.setTextColor(Color.WHITE);
        done.setTextSize(TypedValue.COMPLEX_UNIT_DIP, 15f);
        done.setTypeface(Typeface.DEFAULT_BOLD);
        done.setPadding(0, 0, 0, 0);
        done.setBackground(rounded(Color.rgb(31, 122, 209), 10f, 0, 0));
        done.setContentDescription("Finish moving touch controls");
        done.setOnClickListener(v -> endLayoutEditing());
        editorBar.addView(done, new LinearLayout.LayoutParams(px(68), px(40)));
        editorBar.setVisibility(GONE);
        addView(editorBar, topLeft());

        menu = new MenuView(activity);
        menu.setListener(this);
        addView(menu, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));

        setOnApplyWindowInsetsListener((v, insets) -> {
            post(this::arrange);
            return insets;
        });
        applyFPSVisibility();
    }

    // ---------------------------------------------------------------- views

    private int px(float dips) { return Math.round(dips * dp); }

    private static LayoutParams topLeft() {
        LayoutParams params = new LayoutParams(0, 0);
        params.gravity = Gravity.TOP | Gravity.START;
        return params;
    }

    private GradientDrawable rounded(int fill, float radius, float stroke, int strokeColor) {
        GradientDrawable d = new GradientDrawable();
        d.setColor(fill);
        d.setCornerRadius(radius * dp);
        if (stroke > 0f) d.setStroke(Math.max(1, Math.round(stroke * dp)), strokeColor);
        return d;
    }

    private TextView label(String text, float size, boolean bold) {
        TextView view = new TextView(themed);
        view.setText(text);
        view.setTextColor(Color.WHITE);
        view.setTextSize(TypedValue.COMPLEX_UNIT_DIP, size);
        if (bold) view.setTypeface(Typeface.DEFAULT_BOLD);
        return view;
    }

    private View row(String title, View control) {
        LinearLayout row = new LinearLayout(themed);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setMinimumHeight(px(44));
        TextView label = label(title, 15f, false);
        row.addView(label, new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
        LinearLayout.LayoutParams params = control instanceof SeekBar
                ? new LinearLayout.LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f)
                : new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        params.leftMargin = px(12);
        if (!(control instanceof SeekBar)) {
            View gap = new View(themed);
            row.addView(gap, new LinearLayout.LayoutParams(0, 1, 1f));
        }
        row.addView(control, params);
        return row;
    }

    /** A switch in the iPhone's colors: green when on, a dim track when off, whatever the phone's theme. */
    private Switch switchControl() {
        Switch control = new Switch(themed);
        int[][] states = {{android.R.attr.state_checked}, {}};
        control.setThumbTintList(new android.content.res.ColorStateList(states,
                new int[] {Color.WHITE, Color.rgb(210, 210, 210)}));
        control.setTrackTintList(new android.content.res.ColorStateList(states,
                new int[] {Color.rgb(52, 199, 89), Color.argb(110, 255, 255, 255)}));
        return control;
    }

    private SeekBar slider() {
        SeekBar bar = new SeekBar(themed);
        bar.setMax(1000);
        return bar;
    }

    private interface SliderValue {
        void changed(float value);
    }

    private SeekBar.OnSeekBarChangeListener sliderListener(SliderValue onChange) {
        return new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar bar, int progress, boolean fromUser) {
                if (fromUser && !updatingSliders) onChange.changed(progress / 1000f);
            }
            @Override
            public void onStartTrackingTouch(SeekBar bar) {}
            @Override
            public void onStopTrackingTouch(SeekBar bar) {}
        };
    }

    private static void setFraction(SeekBar bar, float value, float min, float max) {
        bar.setProgress(Math.round(Math.max(0f, Math.min(1f, (value - min) / (max - min))) * 1000f));
    }

    /** Puts a view at (x, y) with that size; a height of LayoutParams.WRAP_CONTENT fits its content. */
    private static void place(View view, float x, float y, float width, float height) {
        LayoutParams params = (LayoutParams) view.getLayoutParams();
        int l = Math.round(x), t = Math.round(y), w = Math.max(0, Math.round(width));
        int h = height == LayoutParams.WRAP_CONTENT ? LayoutParams.WRAP_CONTENT : Math.max(0, Math.round(height));
        if (params.leftMargin == l && params.topMargin == t && params.width == w && params.height == h) return;
        params.leftMargin = l;
        params.topMargin = t;
        params.width = w;
        params.height = h;
        view.setLayoutParams(params);
    }

    /** The ⋯ button: a dark capsule with three white dots and a thin light border. */
    private final class MenuButton extends View {
        private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG), stroke = new Paint(Paint.ANTI_ALIAS_FLAG),
                dots = new Paint(Paint.ANTI_ALIAS_FLAG);
        MenuButton(Context context) {
            super(context);
            fill.setColor(Color.argb(184, 15, 15, 15));
            stroke.setStyle(Paint.Style.STROKE);
            stroke.setStrokeWidth(Math.max(1f, dp));
            stroke.setColor(Color.argb(77, 255, 255, 255));
            dots.setColor(Color.WHITE);
            setClickable(true);
        }
        @Override
        protected void onDraw(Canvas canvas) {
            float r = Math.min(getWidth(), getHeight()) * 0.5f, cx = getWidth() * 0.5f, cy = getHeight() * 0.5f;
            canvas.drawCircle(cx, cy, r - stroke.getStrokeWidth() * 0.5f, fill);
            canvas.drawCircle(cx, cy, r - stroke.getStrokeWidth() * 0.5f, stroke);
            float dot = 2.3f * dp, gap = 7f * dp;
            for (int i = -1; i <= 1; i++) canvas.drawCircle(cx + i * gap, cy, dot, dots);
        }
    }

    // ---------------------------------------------------------------- layout

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        post(this::arrange);
    }

    /** Folding or unfolding changes the form factor (phone or tablet layout) and the screen's shape. */
    @Override
    protected void onConfigurationChanged(Configuration configuration) {
        super.onConfigurationChanged(configuration);
        post(this::arrange);
    }

    /** Whether the controls are on screen in play: then they get columns beside the picture. */
    private boolean controlsShown() { return touch.visibleInPlay(); }

    /** Lays out everything for the current screen, as layoutSubviews does on the iPhone. */
    void arrange() {
        final int w = getWidth(), h = getHeight();
        if (w <= 0 || h <= 0) return;
        phone = getResources().getConfiguration().smallestScreenWidthDp < 600;
        Insets in = Insets.NONE;
        WindowInsets root = getRootWindowInsets();
        if (root != null) in = root.getInsets(WindowInsets.Type.displayCutout() | WindowInsets.Type.systemBars());
        safe.set(in.left, in.top, w - in.right, h - in.bottom);

        // Where the picture is. The original picture keeps its shape, centered; with the controls shown and no
        // column's room beside it, the surface narrows until there is.
        int roomW = 0, roomH = 0;
        if (launchAspectMode == 0) {
            float pw = Math.min(w, h * launchRatio), ph = pw / launchRatio;
            float bar = Math.min((w - pw) * 0.5f - safe.left, safe.right - (w + pw) * 0.5f) - 4f * dp;
            if (bar < 84f * dp && controlsShown()) {
                float margin = Math.max(safe.left, w - safe.right) + 4f * dp + 92f * dp;
                float rw = w - 2f * margin, rh = rw / launchRatio;
                if (rh >= 300f * dp && rh <= h) {
                    pw = rw;
                    ph = rh;
                    roomW = Math.round(rw);
                    roomH = Math.round(rh);
                }
            }
            picture.set((w - pw) * 0.5f, (h - ph) * 0.5f, (w + pw) * 0.5f, (h + ph) * 0.5f);
        } else {
            picture.set(0, 0, w, h);
        }
        activity.setSurfaceRoom(roomW, roomH);
        touch.configure(phone, safe, picture, launchAspectMode);

        place(menuButton, safe.right - 52f * dp, safe.top + 12f * dp, 40f * dp, 40f * dp);
        // The FPS label sits at the top center rather than the iPhone app's top left, where the Start button is.
        LayoutParams fpsParams = (LayoutParams) fpsLabel.getLayoutParams();
        int fpsTop = Math.round(safe.top + 12f * dp);
        if (fpsParams.topMargin != fpsTop) {
            fpsParams.topMargin = fpsTop;
            fpsLabel.setLayoutParams(fpsParams);
        }
        fpsLabel.setMaxWidth(Math.max(px(120), Math.round(safe.width() - 140f * dp)));
        float panelWidth = Math.min(360f * dp, safe.width() - 32f * dp);
        panelMaxHeight = Math.round(Math.min(330f * dp, safe.height() - 72f * dp));
        place(settingsPanel, safe.right - panelWidth - 12f * dp, safe.top + 60f * dp, panelWidth,
                LayoutParams.WRAP_CONTENT);
        float editorWidth = Math.min(560f * dp, safe.width() - 24f * dp);
        place(editorBar, safe.centerX() - editorWidth * 0.5f, safe.bottom - 72f * dp, editorWidth, 60f * dp);
    }

    // ---------------------------------------------------------------- FPS

    private void applyFPSVisibility() {
        boolean on = settings.bool(Settings.SHOW_FPS, false);
        fpsLabel.setVisibility(on ? VISIBLE : GONE);
        handler.removeCallbacks(fpsTick);
        if (on) fpsTick.run();
    }

    /** Frames shown per second, game speed and the longest frame gap in the last second (touch_controls.cpp). */
    private void updateFPSLabel() {
        float[] f = new float[6];
        try {
            // While the game is held, and for the second that spans the hold, the numbers describe the hold, not
            // the game: the label keeps the last reading until a whole second of play has passed.
            if (Shell.nativePauseReasons() != 0) {
                fpsWasPaused = true;
                return;
            }
            final long now = android.os.SystemClock.uptimeMillis();
            if (fpsWasPaused) {
                fpsWasPaused = false;
                fpsPauseEnded = now;
            }
            if (now - fpsPauseEnded < 2000) return;
            Shell.nativeFps(f);
        } catch (UnsatisfiedLinkError e) {
            return;
        }
        float shown = f[0], speed = f[1], worst = f[2], display = f[3], game = f[4];
        boolean paused = f[5] != 0f;
        String text;
        if (paused)
            // Each game frame shown twice: the picture moves at the game's rate.
            text = String.format(Locale.US, "%.0f FPS (Smooth Motion paused) · %.0f%% speed · %.0f ms", game, speed,
                    worst);
        else if (display > shown + 5f)
            text = String.format(Locale.US, "%.0f FPS (game %.0f) · %.0f%% speed · %.0f ms", display, shown, speed,
                    worst);
        else
            text = String.format(Locale.US, "%.0f FPS · %.0f%% speed · %.0f ms", shown, speed, worst);
        fpsLabel.setText(text);
        fpsLabel.setTextColor(paused || shown < 27f || speed < 95f ? Color.rgb(255, 204, 77) : Color.WHITE);
    }

    // ---------------------------------------------------------------- pause and alerts

    private static void pause(int reason, boolean on) {
        try {
            Shell.nativePauseSet(reason, on);
        } catch (UnsatisfiedLinkError ignored) {
        }
    }

    private void alertBegin() {
        touch.clearTouchInput();
        if (alertDepth++ == 0) pause(Shell.PAUSE_ALERT, true);
    }

    private void alertEnd() {
        if (alertDepth > 0 && --alertDepth == 0) pause(Shell.PAUSE_ALERT, false);
    }

    private AlertDialog.Builder builder() {
        return new AlertDialog.Builder(activity, android.R.style.Theme_DeviceDefault_Dialog_Alert);
    }

    private void present(AlertDialog dialog) {
        alertBegin();
        dialog.setOnDismissListener(d -> alertEnd());
        dialog.show();
    }

    void showMessage(String title, String message) { alert(title, message, null, "OK", false, null); }

    /** A question with a cancel choice (or none: then only action closes it, as "Close BlueWake" does). */
    void alert(String title, String message, String cancel, String action, boolean destructive, Runnable onAction) {
        AlertDialog.Builder b = builder().setTitle(title).setMessage(message);
        if (cancel != null) b.setNegativeButton(cancel, null);
        b.setPositiveButton(action, (d, which) -> {
            if (onAction != null) onAction.run();
        });
        b.setCancelable(cancel != null || onAction == null);
        AlertDialog dialog = b.create();
        present(dialog);
        if (destructive) dialog.getButton(AlertDialog.BUTTON_POSITIVE).setTextColor(Color.rgb(255, 69, 58));
    }

    void confirm(String title, String message, String cancel, String action, boolean destructive, Runnable onAction) {
        alert(title, message, cancel, action, destructive, onAction);
    }

    /** A list to choose from (an action sheet); a disabled row is shown dimmed. */
    void sheet(String title, String message, List<String> titles, List<Boolean> enabled, Choice choice) {
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(themed, android.R.layout.select_dialog_item, titles) {
            @Override
            public boolean isEnabled(int position) { return enabled.get(position); }
            @Override
            public View getView(int position, View convert, ViewGroup parent) {
                View view = super.getView(position, convert, parent);
                view.setAlpha(enabled.get(position) ? 1f : 0.4f);
                return view;
            }
        };
        LinearLayout head = new LinearLayout(themed);
        head.setOrientation(LinearLayout.VERTICAL);
        head.setPadding(px(24), px(20), px(24), px(4));
        TextView t = label(title, 18f, true);
        head.addView(t);
        if (message != null && !message.isEmpty()) {
            TextView m = label(message, 14f, false);
            m.setAlpha(0.75f);
            m.setPadding(0, px(6), 0, 0);
            head.addView(m);
        }
        AlertDialog dialog = builder().setCustomTitle(head)
                .setAdapter(adapter, (d, which) -> choice.chose(which))
                .setNegativeButton("Cancel", null).create();
        present(dialog);
    }

    Wait wait(String title, String message) {
        AlertDialog dialog = builder().setTitle(title).setMessage(message).setCancelable(false).create();
        present(dialog);
        return new Wait(dialog);
    }

    /** Offers to switch a mod on after its files were installed (offerToTurnOn). */
    void offerToTurnOn(String key, String title, String message) {
        if (settings.bool(key, false)) {
            showMessage(title, message);
            return;
        }
        alert(title, message, "Not Now", "Turn On", false, () -> settings.set(key, true));
    }

    // ---------------------------------------------------------------- pickers and other apps

    private int pickerRequest = -1;

    void startPicker(Intent intent, int request) {
        alertBegin();
        pickerRequest = request;
        try {
            activity.startActivityForResult(intent, request);
        } catch (ActivityNotFoundException e) {
            pickerRequest = -1;
            alertEnd();
            showMessage("No File Picker", "This device has no app to pick files with.");
        }
    }

    boolean onActivityResult(int request, int result, Intent intent) {
        if (request != pickerRequest) return false;
        pickerRequest = -1;
        alertEnd();
        return data.onActivityResult(request, result, intent);
    }

    /**
     * Closes BlueWake after its saves or game files were replaced: the task leaves the recent apps, so the system
     * does not start the game again by itself, then the card's pending writes finish and the process ends.
     */
    void closeApp() {
        activity.finishAndRemoveTask();
        Shell.nativeCloseForSaves();
    }

    private void open(Uri uri) {
        try {
            activity.startActivity(new Intent(Intent.ACTION_VIEW, uri));
        } catch (ActivityNotFoundException e) {
            showMessage("No Browser", uri.toString());
        }
    }

    // ---------------------------------------------------------------- menu

    private void openMenu() {
        if (menu.isOpen()) {
            menu.close();
            return;
        }
        touch.clearTouchInput();
        pause(Shell.PAUSE_MENU, true);
        menu.open(buildMenu(), menuButton.getRight(), menuButton.getBottom() + 6f * dp);
    }

    @Override
    public void onMenuClosed() { pause(Shell.PAUSE_MENU, false); }

    /** Back: out of a submenu, the editor or the panel; with nothing open it opens the ⋯ menu. */
    void onBack() {
        if (menu.isOpen()) menu.back();
        else if (touch.editing()) endLayoutEditing();
        else if (settingsOpen) hideSettingsPanel();
        else openMenu();
    }

    private MenuView.Item buildMenu() {
        MenuView.Item root = MenuView.Item.menu("BlueWake", Arrays.asList(
                displayMenu(), gameplayMenu(), modsMenu(), controllerMenu(), touchMenu(), dataMenu(), helpMenu()));
        // BLUEWAKE_MENU_DUMP=1 writes the menu tree to the session log once, so a device run can check what the
        // ⋯ menu offers without a screenshot.
        if (!menuDumped && "1".equals(System.getenv("BLUEWAKE_MENU_DUMP"))) {
            menuDumped = true;
            dump(root, 0);
        }
        return root;
    }

    private static void dump(MenuView.Item item, int depth) {
        StringBuilder line = new StringBuilder("[menu] ");
        for (int i = 0; i < depth; i++) line.append("  ");
        line.append(item.title);
        if (item.children == null) line.append(item.checked ? " [on]" : " [off]");
        if (item.subtitle != null && !item.subtitle.isEmpty()) line.append(" - ").append(item.subtitle);
        if (!item.inline) Shell.log(line.toString());
        if (item.children != null)
            for (MenuView.Item child : item.children) dump(child, item.inline ? depth : depth + 1);
    }

    private static MenuView.Item action(String title, Runnable run) { return MenuView.Item.action(title, run); }

    private static MenuView.Item menu(String title, MenuView.Item... children) {
        return MenuView.Item.menu(title, new ArrayList<>(Arrays.asList(children)));
    }

    private void alertNextLaunch(String message) {
        showMessage("Applies Next Launch", message);
    }

    private MenuView.Item displayMenu() {
        MenuView.Item showFPS = action("Show FPS", () -> {
            settings.set(Settings.SHOW_FPS, !settings.bool(Settings.SHOW_FPS, false));
            applyFPSVisibility();
        }).checked(settings.bool(Settings.SHOW_FPS, false));

        final int smooth = settings.smoothMotion();
        List<MenuView.Item> smoothChoices = new ArrayList<>();
        smoothChoices.add(smoothAction("Off (Original 30 FPS)", 0, smooth));
        smoothChoices.add(smoothAction("60 FPS", 1, smooth));
        if (maxRefreshRate() >= 119f || smooth == 3) smoothChoices.add(smoothAction("120 FPS", 3, smooth));
        MenuView.Item smoothMotion = MenuView.Item.menu("Smooth Motion (Experimental)", smoothChoices);

        final int scale = settings.renderScale();
        MenuView.Item resolution = menu("Render Resolution",
                scaleAction("Screen Native", 0, scale),
                scaleAction("4× (2560×1920)", 4, scale),
                scaleAction("3× (1920×1440), Recommended", 3, scale),
                scaleAction("2× (1280×960)", 2, scale),
                scaleAction("Original (640×480)", 1, scale));

        // Forced anisotropy sharpens the ground and the sea at a glancing angle (applies at once).
        final int aniso = settings.anisotropy();
        MenuView.Item filtering = menu("Texture Filtering",
                anisoAction("16× Anisotropic", 16, aniso),
                anisoAction("8× Anisotropic", 8, aniso),
                anisoAction("4× Anisotropic", 4, aniso),
                anisoAction("Original", 1, aniso));

        final int aspect = settings.integer(Settings.ASPECT_MODE, 0);
        MenuView.Item aspectMenu = menu("Aspect Ratio",
                aspectAction("Original 4:3", 0, aspect),
                aspectAction("Fill Screen", 1, aspect));
        return menu("Display", showFPS, smoothMotion, resolution, filtering, aspectMenu);
    }

    private float maxRefreshRate() {
        Display display = activity.getDisplay();
        float best = 60f;
        if (display != null)
            for (Display.Mode mode : display.getSupportedModes()) best = Math.max(best, mode.getRefreshRate());
        return best;
    }

    private MenuView.Item smoothAction(String title, int value, int current) {
        return action(title, () -> {
            settings.set(Settings.SMOOTH_MOTION, value);
            settings.applyNow();
            activity.applyRefreshRate();
        }).checked(current == value);
    }

    private MenuView.Item scaleAction(String title, int value, int current) {
        return action(title, () -> {
            settings.set(Settings.RENDER_SCALE, value);
            settings.applyNow();
        }).checked(current == value);
    }

    private MenuView.Item anisoAction(String title, int value, int current) {
        return action(title, () -> {
            settings.set(Settings.ANISOTROPY, value);
            settings.applyNow();
        }).checked(current == value);
    }

    private MenuView.Item aspectAction(String title, int mode, int current) {
        return action(title, () -> {
            if (settings.integer(Settings.ASPECT_MODE, 0) == mode) return;
            settings.set(Settings.ASPECT_MODE, mode);
            alertNextLaunch("The new aspect ratio takes effect the next time BlueWake starts.");
        }).checked(current == mode);
    }

    // These guest changes apply at the next launch, like the code mods.
    private MenuView.Item gameplayAction(String title, String key) {
        return action(title, () -> {
            settings.set(key, !settings.bool(key, false));
            touch.clearTouchInput();
            touch.relayout();
        }).subtitle("Applies when BlueWake restarts").checked(settings.bool(key, false));
    }

    private MenuView.Item gameplayMenu() {
        return menu("Gameplay",
                gameplayAction("Jump & Sprint", Settings.MOVEMENT_EXTRAS),
                gameplayAction("Fast Transitions", Settings.FAST_TRANSITIONS),
                gameplayAction("Quick Doors", Settings.QUICK_DOORS));
    }

    // Mods. Each applies the next time BlueWake starts: code mods are compiled into the game module and switched on
    // before the game runs (BLUEWAKE_MODS, Settings.launchEnvironment), and texture replacement is loaded with the
    // renderer.

    /** "On", "Off", or the change waiting for the next launch. */
    private String modState(String key) {
        boolean now = settings.bool(key, false);
        Boolean launched = launchMods.get(key);
        if (launched != null && now == launched) return now ? "On" : "Off";
        return now ? "On at next launch" : "Off at next launch";
    }

    private MenuView.Item modToggle(String title, String detail, String key) {
        return action(title, () -> {
            boolean on = !settings.bool(key, false);
            settings.set(key, on);
            // One widescreen shape at a time: the two patch the same code.
            if (on && key.equals(Settings.MOD_WIDESCREEN)) settings.set(Settings.MOD_WIDESCREEN_1610, false);
            if (on && key.equals(Settings.MOD_WIDESCREEN_1610)) settings.set(Settings.MOD_WIDESCREEN, false);
            alertNextLaunch("Mods take effect the next time BlueWake starts. Your saves are not changed.");
        }).subtitle(detail + " · " + modState(key)).checked(settings.bool(key, false));
    }

    /** Texture count for the menu, counted once per launch (a pack holds thousands of files) and after an install. */
    private int countPackFiles() {
        if (packFiles < 0) packFiles = countTextures(settings.texturePackDirectory());
        return packFiles;
    }

    void texturePackChanged() { packFiles = -1; }

    private static int countTextures(File dir) {
        File[] files = dir.listFiles();
        if (files == null) return 0;
        int n = 0;
        for (File f : files) {
            if (f.isDirectory()) {
                n += countTextures(f);
            } else {
                String name = f.getName().toLowerCase(Locale.US);
                if (name.endsWith(".png") || name.endsWith(".dds")) n++;
            }
        }
        return n;
    }

    private MenuView.Item modsMenu() {
        int pack = countPackFiles();
        String packDetail = pack > 0 ? pack + " textures installed" : "No pack installed yet";
        List<MenuView.Item> children = new ArrayList<>();
        children.add(MenuView.Item.inline(new ArrayList<>(Arrays.asList(
                modToggle("Widescreen 16:9", "Shows more of the world on wide screens", Settings.MOD_WIDESCREEN),
                modToggle("Widescreen 16:10", "For 16:10 screens, such as a tablet", Settings.MOD_WIDESCREEN_1610),
                modToggle("HD Texture Pack", packDetail, Settings.MOD_HD_TEXTURES),
                modToggle("Better Wind Waker", "Swift Sail, instant text and more", Settings.MOD_BETTERWW)))));
        MenuView.Item bww = betterWWSettingsMenu();
        if (bww != null) children.add(bww);
        children.add(MenuView.Item.inline(new ArrayList<>(Arrays.asList(
                action("Install Texture Pack…", data::chooseTexturePack)
                        .subtitle("Pick a folder of PNG or DDS textures")))));
        return MenuView.Item.menu("Mods", children);
    }

    /**
     * Better Wind Waker's settings, one switch each, from the game module's option table. A switch the player
     * changes is stored under Settings.OPTION_PREFIX + its name and applies at the next launch (BLUEWAKE_OPTIONS);
     * the settings need Better Wind Waker on.
     */
    private MenuView.Item betterWWSettingsMenu() {
        List<MenuView.Item> items = new ArrayList<>();
        final boolean modOn = settings.bool(Settings.MOD_BETTERWW, false);
        for (int i = 0; ; i++) {
            String[] option;
            try {
                option = Shell.nativeGameOption(i);
            } catch (UnsatisfiedLinkError e) {
                break;
            }
            if (option == null) break;
            final String name = option[0];
            final String key = Settings.OPTION_PREFIX + name;
            final boolean on = settings.has(key) ? settings.bool(key, false) : "1".equals(option[2]);
            final boolean running = "1".equals(option[3]);
            MenuView.Item item = action(option[1] != null ? option[1] : name, () -> {
                settings.set(key, !on);
                // Swift Sail and Brisk Sail are two tunings of one sail.
                if (!on && name.equals("swift_sail")) settings.set(Settings.OPTION_PREFIX + "brisk_sail", false);
                if (!on && name.equals("brisk_sail")) settings.set(Settings.OPTION_PREFIX + "swift_sail", false);
            }).checked(on);
            if (modOn && on != running) item.subtitle(on ? "On at next launch" : "Off at next launch");
            items.add(item);
        }
        if (items.isEmpty()) return null;
        return MenuView.Item.menu("Better Wind Waker Settings", items)
                .subtitle(modOn ? "Apply at next launch" : "Turn on Better Wind Waker to use them");
    }

    private MenuView.Item controllerMenu() {
        MenuView.Item camera = menu("Camera Stick",
                controllerToggle("Invert Horizontal", Settings.INVERT_X),
                controllerToggle("Invert Vertical", Settings.INVERT_Y));
        List<MenuView.Item> remaps = new ArrayList<>();
        for (int i = 0; i < Settings.REMAP_NAMES.length; i++) {
            final int index = i;
            final int current = settings.remapCurrent(i);
            List<MenuView.Item> choices = new ArrayList<>();
            for (int c = 0; c < Settings.NATIVE_CHOICES.length; c++) {
                final int nativeButton = Settings.NATIVE_CHOICES[c];
                choices.add(action(Settings.NATIVE_NAMES[c], () -> {
                    settings.remapSet(index, nativeButton);
                    settings.applyNow();
                }).checked(nativeButton == current));
            }
            remaps.add(MenuView.Item.menu("GameCube " + Settings.REMAP_NAMES[i], choices)
                    .subtitle(Settings.nativeName(current)));
        }
        remaps.add(action("Reset Buttons", () -> {
            settings.remapReset();
            settings.applyNow();
        }));
        return menu("Controller", camera, MenuView.Item.menu("Button Mapping", remaps));
    }

    private MenuView.Item controllerToggle(String title, String key) {
        return action(title, () -> {
            settings.set(key, !settings.bool(key, false));
            settings.applyNow();
        }).checked(settings.bool(key, false));
    }

    private MenuView.Item touchMenu() {
        MenuView.Item show = action("Show Touch Controls", () -> {
            settings.set(Settings.SHOW_TOUCH, !settings.bool(Settings.SHOW_TOUCH, true));
            applyControllerVisibility();
        }).checked(settings.bool(Settings.SHOW_TOUCH, true));
        return menu("Touch Controls", show,
                action("Touch Control Settings…", this::showSettingsPanel),
                action("Move Controls", this::beginLayoutEditing));
    }

    private MenuView.Item dataMenu() {
        return menu("Game Data & Saves",
                MenuView.Item.inline(new ArrayList<>(Arrays.asList(
                        action("Back Up Saves…", data::backUpSaves),
                        action("Restore Saves…", data::chooseSavesToRestore),
                        action("Import Dolphin Save…", data::chooseDolphinSave),
                        action("Where Are My Files?", data::explainFiles)))),
                action("Remove Disc Image…", data::confirmDataRemoval).destructive());
    }

    private MenuView.Item helpMenu() {
        return menu("Help & Feedback",
                action("Report a Problem on GitHub…", this::reportProblem),
                action("Share Session Log…", this::shareSessionLog),
                action("BlueWake on GitHub", () -> open(Uri.parse(GITHUB_URL))));
    }

    // ---------------------------------------------------------------- help

    private String diagnosticReport() {
        StringBuilder r = new StringBuilder();
        String version = "?";
        long build = 0;
        try {
            android.content.pm.PackageInfo info =
                    activity.getPackageManager().getPackageInfo(activity.getPackageName(), 0);
            version = info.versionName;
            build = info.getLongVersionCode();
        } catch (android.content.pm.PackageManager.NameNotFoundException ignored) {
        }
        r.append("BlueWake ").append(version).append(" (").append(build).append(")\n");
        r.append("Device ").append(Build.MANUFACTURER).append(' ').append(Build.MODEL).append(", Android ")
                .append(Build.VERSION.RELEASE).append(" (API ").append(Build.VERSION.SDK_INT).append(")\n");
        File game = new File(settings.dataDir, "game");
        for (String name : new String[] {"GZLE01.iso", "main.dol", "rels"})
            r.append(name).append(": ").append(new File(game, name).exists() ? "present" : "missing").append('\n');
        for (String name : new String[] {"GZLE01.card", "sram.bin"})
            r.append(name).append(": ").append(new File(settings.dataDir, name).exists() ? "present" : "missing")
                    .append('\n');
        r.append("Controllers: ").append(BlueWakeActivity.gamepadCount()).append('\n');
        r.append(String.format(Locale.US, "Render scale: %s, anisotropy: %d, smooth motion: %d, aspect: %d, "
                        + "camera invert x=%d y=%d, buttons remapped: %d\n",
                settings.has(Settings.RENDER_SCALE) ? Integer.toString(settings.renderScale()) : "3 (default)",
                settings.anisotropy(), settings.smoothMotion(), settings.integer(Settings.ASPECT_MODE, 0),
                settings.bool(Settings.INVERT_X, false) ? 1 : 0, settings.bool(Settings.INVERT_Y, false) ? 1 : 0,
                settings.remapped() ? 1 : 0));
        PowerManager power = (PowerManager) activity.getSystemService(Context.POWER_SERVICE);
        if (power != null)
            r.append("Thermal state: ").append(power.getCurrentThermalStatus()).append(", Battery Saver: ")
                    .append(power.isPowerSaveMode() ? 1 : 0).append('\n');
        return r.toString();
    }

    private void reportProblem() {
        // A marker in the session log at the moment the player reported it.
        Shell.log("[mark] Report a Problem opened");
        // A new GitHub issue with the device report filled in. The session log is too long for a URL; the body
        // asks for it (Help & Feedback > Share Session Log).
        String body = "**What happened?**\n\n\n**What were you doing (scene, menu, controls)?**\n\n\n"
                + "**About when (the time on your phone or tablet)?**\n\n\n"
                + "Please attach the session log: BlueWake menu > Help & Feedback > Share Session Log "
                + "(or Android/data/" + activity.getPackageName() + "/files/logs over USB).\n\n"
                + "<details><summary>Device report</summary>\n\n```\n" + diagnosticReport() + "```\n</details>\n";
        open(Uri.parse(GITHUB_URL + "/issues/new").buildUpon().appendQueryParameter("title", "")
                .appendQueryParameter("body", body).build());
    }

    private File latestSessionLog() {
        File[] logs = new File(settings.dataDir, "logs").listFiles((dir, name) -> LogProvider.isSessionLog(name));
        if (logs == null || logs.length == 0) return null;
        Arrays.sort(logs, (a, b) -> a.getName().compareTo(b.getName()));
        return logs[logs.length - 1];
    }

    private void shareSessionLog() {
        Shell.log("[mark] Share Session Log opened");
        File log = latestSessionLog();
        Intent send = new Intent(Intent.ACTION_SEND);
        if (log != null) {
            Uri uri = LogProvider.uriFor(activity, log);
            send.setType("text/plain");
            send.putExtra(Intent.EXTRA_STREAM, uri);
            send.setClipData(android.content.ClipData.newRawUri(log.getName(), uri));
            send.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        } else {
            send.setType("text/plain");
            send.putExtra(Intent.EXTRA_TEXT, diagnosticReport());
        }
        try {
            activity.startActivity(Intent.createChooser(send, "Share Session Log"));
        } catch (ActivityNotFoundException e) {
            showMessage("Nothing to Share With", "This device has no app to share the log with.");
        }
    }

    // ---------------------------------------------------------------- settings panel

    void showSettingsPanel() {
        endLayoutEditing();
        updatingSliders = true;
        setFraction(opacitySlider, settings.number(Settings.OPACITY, 0.8f), 0.25f, 1.0f);
        setFraction(sizeSlider, settings.number(Settings.SIZE, 1f), 0.70f, 1.35f);
        hideSwitch.setChecked(settings.bool(Settings.HIDE_ON_CONTROLLER, true));
        editSwitch.setChecked(false);
        updatingSliders = false;
        settingsPanel.setVisibility(VISIBLE);
        settingsPanel.bringToFront();
        menu.bringToFront();
        touch.clearTouchInput();
        if (!settingsOpen) pause(Shell.PAUSE_SETTINGS, true);
        settingsOpen = true;
        arrange();
    }

    void hideSettingsPanel() {
        settingsPanel.setVisibility(GONE);
        if (settingsOpen) pause(Shell.PAUSE_SETTINGS, false);
        settingsOpen = false;
    }

    private void confirmResetLayout() {
        alert("Reset the Touch Layout?", "Every control on this kind of device, including the D-pad, returns to its "
                + "default position and size.", "Cancel", "Reset", true, () -> touch.resetLayout());
    }

    // ---------------------------------------------------------------- layout editor

    void beginLayoutEditing() {
        settingsPanel.setVisibility(GONE);
        if (settingsOpen) pause(Shell.PAUSE_SETTINGS, false);
        settingsOpen = false;
        editorBar.setVisibility(VISIBLE);
        editorBar.bringToFront();
        menuButton.bringToFront();
        menu.bringToFront();
        selectedSizeSlider.setEnabled(false);
        editorHint.setText("Drag controls • tap one to resize");
        if (!touch.editing()) pause(Shell.PAUSE_LAYOUT, true);
        touch.beginLayoutEditing();
        arrange();
    }

    void endLayoutEditing() {
        if (!touch.editing()) return;
        touch.endLayoutEditing();
        updatingSliders = true;
        editSwitch.setChecked(false);
        updatingSliders = false;
        editorBar.setVisibility(GONE);
        pause(Shell.PAUSE_LAYOUT, false);
        arrange();
    }

    @Override
    public void onEditorSelection(String identifier, String label, float scale) {
        if (identifier == null) return;
        updatingSliders = true;
        setFraction(selectedSizeSlider, scale, 0.60f, 1.75f);
        updatingSliders = false;
        selectedSizeSlider.setEnabled(true);
        editorHint.setText(label + " size");
    }

    // ---------------------------------------------------------------- controller and lifecycle

    /** A controller hides the touch controls (with Hide with a controller on) and lets go of every held touch. */
    void setControllerConnected(boolean connected) {
        final boolean arrived = connected && !controllerConnected;
        controllerConnected = connected;
        controllerHidden = connected && settings.bool(Settings.HIDE_ON_CONTROLLER, true);
        touch.setControllerHidden(controllerHidden);
        if (arrived) touch.clearTouchInput();
        arrange();
    }

    private void applyControllerVisibility() { setControllerConnected(BlueWakeActivity.gamepadCount() > 0); }

    /** Every held touch lets go when the app stops being active. */
    void clearTouchInput() { touch.clearTouchInput(); }

    void onDestroy() { handler.removeCallbacks(fpsTick); }
}
