package dev.bluewake.android;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.view.MotionEvent;
import android.view.View;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * The GameCube touch controls, as the iPhone and iPad app draws and places them (apple/ios/src/BWGameOverlay.mm:
 * the buttons, the sticks, the D-pad, SunPad's layout defaults, the phone's two columns beside a 4:3 picture, the
 * sparse per-control layout kept separately for phones and tablets, and the layout editor). The pad state goes to
 * the iPhone app's own glue (touch_controls.cpp) through Shell.nativeTouchPublish, which also holds a quick tap
 * for the game to see it.
 *
 * One addition the iPhone app doesn't have: the movement stick floats. A touch anywhere on the left half that is
 * not on another control puts the stick under the finger; it goes back to its place when the finger lifts.
 */
public class TouchControlsView extends View {
    /** What the overlay needs to know from the controls. */
    interface Listener {
        /** A control was selected in the layout editor (null: none); label is its name for the editor bar. */
        void onEditorSelection(String identifier, String label, float scale);
    }

    private static final int BUTTON = 0, STICK = 1, DPAD_GROUP = 2;

    private static final class Control {
        final String id, label, name;
        final int kind, mask;
        int fill, titleColor, thumb;
        final RectF frame = new RectF();
        float homeX, homeY;      // a floating stick's place
        boolean pressed, hidden;
        float stickX, stickY;    // -1..1, +y up
        Control(String id, String label, String name, int kind, int mask) {
            this.id = id;
            this.label = label;
            this.name = name;
            this.kind = kind;
            this.mask = mask;
        }
        boolean contains(float x, float y, float slop) {
            if (kind == STICK) {
                float r = Math.min(frame.width(), frame.height()) * 0.5f + slop;
                float dx = x - frame.centerX(), dy = y - frame.centerY();
                return dx * dx + dy * dy <= r * r;
            }
            return x >= frame.left - slop && x <= frame.right + slop && y >= frame.top - slop
                    && y <= frame.bottom + slop;
        }
    }

    private final Settings settings;
    private final float dp;
    private final List<Control> controls = new ArrayList<>();
    private final Control move, cStick, dpadGroup;
    private final Paint fillPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint strokePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF scratch = new RectF();

    private Listener listener;
    private boolean phone = true;            // the layout's form factor
    private final RectF safe = new RectF();  // the view minus display cutouts
    private final RectF picture = new RectF(); // where the game's picture is, for the columns beside it
    private int launchAspectMode;            // the aspect the game started with
    private boolean configured, controllerHidden, editing;
    private Control selected;

    // Which control each pointer holds (Android's pointer ids are 0..31).
    private final Control[] pointerControl = new Control[32];
    private final float[] dragOffsetX = new float[32], dragOffsetY = new float[32];
    private boolean dragMoved;

    public TouchControlsView(Context context, Settings settings) {
        super(context);
        this.settings = settings;
        dp = context.getResources().getDisplayMetrics().density;
        strokePaint.setStyle(Paint.Style.STROKE);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
        move = add("move", "", "Move stick", STICK, 0);
        move.fill = rgba(0.13f, 0.13f, 0.13f, 0.86f);
        move.thumb = rgba(0.58f, 0.58f, 0.58f, 0.94f);
        cStick = add("c", "", "Camera stick", STICK, 0);
        cStick.fill = rgba(0.91f, 0.66f, 0.08f, 0.90f);
        cStick.thumb = rgba(1.00f, 0.84f, 0.25f, 0.98f);
        button("A", "A", "A", Shell.A);
        button("B", "B", "B", Shell.B);
        button("X", "X", "X", Shell.X);
        button("Y", "Y", "Y", Shell.Y);
        button("Z", "Z", "Z", Shell.Z);
        button("Start", "START", "START", Shell.START);
        button("L", "L", "L", Shell.L);
        button("R", "R", "R", Shell.R);
        button("Jump", "Jump", "Jump", Shell.JUMP);
        button("Sprint", "Run", "Sprint (hold)", Shell.SPRINT);
        button("D_U", "▲", "D-pad up", Shell.DPAD_UP);
        button("D_D", "▼", "D-pad down", Shell.DPAD_DOWN);
        button("D_L", "◀", "D-pad left", Shell.DPAD_LEFT);
        button("D_R", "▶", "D-pad right", Shell.DPAD_RIGHT);
        // The D-pad moves and resizes as one object in the editor while its four
        // directions keep their own hit regions in play.
        dpadGroup = add("DPad", "", "D-pad", DPAD_GROUP, 0);
    }

    void setListener(Listener listener) { this.listener = listener; }

    private Control add(String id, String label, String name, int kind, int mask) {
        Control c = new Control(id, label, name, kind, mask);
        controls.add(c);
        return c;
    }

    private void button(String id, String label, String name, int mask) {
        Control c = add(id, label, name, BUTTON, mask);
        c.fill = rgba(0.22f, 0.22f, 0.22f, 0.88f);
        c.titleColor = Color.WHITE;
        switch (mask) {
            case Shell.A: c.fill = rgba(0.08f, 0.56f, 0.29f, 0.92f); break;
            case Shell.B: c.fill = rgba(0.78f, 0.10f, 0.13f, 0.92f); break;
            case Shell.X:
            case Shell.Y:
                c.fill = rgba(0.72f, 0.72f, 0.72f, 0.92f);
                c.titleColor = rgba(0.12f, 0.12f, 0.12f, 1.0f);
                break;
            case Shell.Z: c.fill = rgba(0.38f, 0.18f, 0.58f, 0.94f); break;
            case Shell.START: c.fill = rgba(0.28f, 0.28f, 0.28f, 0.92f); break;
            default: break;
        }
    }

    private Control byId(String id) {
        for (Control c : controls) if (c.id.equals(id)) return c;
        return null;
    }

    private static int rgba(float r, float g, float b, float a) {
        return Color.argb(Math.round(a * 255), Math.round(r * 255), Math.round(g * 255), Math.round(b * 255));
    }

    private static boolean isDPadButton(Control c) {
        return c.kind == BUTTON && (c.mask == Shell.DPAD_UP || c.mask == Shell.DPAD_DOWN
                || c.mask == Shell.DPAD_LEFT || c.mask == Shell.DPAD_RIGHT);
    }

    // ---------------------------------------------------------------- state from the overlay

    /**
     * The form factor, the safe area and the game picture's place; when one changed, the controls are laid out
     * again (a held stick is left alone otherwise).
     */
    void configure(boolean phone, RectF safeArea, RectF picture, int launchAspectMode) {
        if (configured && this.phone == phone && safe.equals(safeArea) && this.picture.equals(picture)
                && this.launchAspectMode == launchAspectMode)
            return;
        configured = true;
        this.phone = phone;
        this.safe.set(safeArea);
        this.picture.set(picture);
        this.launchAspectMode = launchAspectMode;
        relayout();
    }

    void setControllerHidden(boolean hidden) {
        if (hidden && !controllerHidden) clearTouchInput();
        controllerHidden = hidden;
        updateAppearance();
    }

    boolean editing() { return editing; }

    /** Whether the controls are on screen (shown, no controller hiding them, or the editor is open). */
    boolean visibleInPlay() {
        return editing || (!controllerHidden && settings.bool(Settings.SHOW_TOUCH, true));
    }

    // ---------------------------------------------------------------- layout

    private RectF at(float cx, float cy, float w, float h) {
        return new RectF(cx - w * 0.5f, cy - h * 0.5f, cx + w * 0.5f, cy + h * 0.5f);
    }

    private RectF normalized(float x, float y, float w, float h) {
        return at(safe.left + x * safe.width(), safe.top + y * safe.height(), w, h);
    }

    void relayout() {
        if (safe.width() <= 0 || safe.height() <= 0) return;
        // SunPad's defaults: a fixed set on tablets at least 1000 dp wide, a
        // reference 800x380 area scaled down elsewhere, normalized phone positions.
        final float safeW = safe.width() / dp, safeH = safe.height() / dp;
        final boolean pad = !phone && safeW >= 1000f;
        final float base = pad ? 1f : Math.min(1f, Math.min(safeW / 800f, safeH / 380f));
        final float size = settings.number(Settings.SIZE, 1f);
        final float scale = base * size;
        final float margin = (pad ? 34f : Math.max(8f, 18f * base)) * dp;
        final float stick = (pad ? 172f : 126f * base) * size * dp;
        final float small = (pad ? 62f : 46f * base) * size * dp;
        final float medium = (pad ? 76f : 58f * base) * size * dp;
        final float large = (pad ? 104f : 78f * base) * size * dp;
        final float camera = (pad ? 112f : 86f * base) * size * dp;
        java.util.Map<String, RectF> columns = columnFrames(size);

        place(move, columns, phone ? normalized(0.1234722222f, 0.7803490991f, stick, stick)
                : pad ? normalized(0.1310395315f, 0.7905894519f, stick, stick)
                : new RectF(safe.left + margin, safe.bottom - stick - margin, safe.left + margin + stick,
                        safe.bottom - margin));
        // SunPad's tablet defaults were tuned on a larger iPad; on an 11-inch
        // screen the C-stick, L and Start overlapped their neighbours, so those
        // sit slightly further out (as on the iPhone app).
        place(cStick, columns, phone ? normalized(0.9233055556f, 0.8130067568f, camera, camera)
                : pad ? normalized(0.9062957540f, 0.8800000000f, camera, camera)
                : new RectF(safe.right - margin - camera, safe.bottom - margin - camera, safe.right - margin,
                        safe.bottom - margin));

        Control a = byId("A");
        place(a, columns, pad ? normalized(0.8916544656f, 0.7409513961f, large, large)
                : new RectF(safe.right - margin - large, safe.bottom - margin - camera - large - 18f * scale * dp,
                        safe.right - margin, safe.bottom - margin - camera - 18f * scale * dp));
        place(byId("B"), columns, phone ? normalized(0.8398611111f, 0.6898648649f, medium, medium)
                : pad ? normalized(0.8360175695f, 0.8092037229f, medium, medium)
                : rect(a.frame.left - medium - 12f * scale * dp, a.frame.centerY() + 8f * dp, medium, medium));
        place(byId("X"), columns, phone ? normalized(0.9034166667f, 0.4258445946f, small, small)
                : pad ? normalized(0.9593704246f, 0.7156153051f, small, small)
                : rect(a.frame.centerX() - small * 0.5f, a.frame.top - small - 10f * scale * dp, small, small));
        place(byId("Y"), columns, phone ? normalized(0.8452500000f, 0.5268581081f, small, small)
                : pad ? normalized(0.9542459736f, 0.7869700103f, small, small)
                : rect(a.frame.left - small - 8f * scale * dp, a.frame.top - small + 8f * dp, small, small));

        final float shoulder = (pad ? 132f : 94f * base) * size * dp;
        final float shoulderY = safe.top + (pad ? 92f : 68f * base) * dp;
        place(byId("L"), columns, phone ? normalized(0.0905833333f, 0.2539977477f, shoulder, small)
                : pad ? normalized(0.1281112738f, 0.6400000000f, shoulder, small)
                : rect(safe.left + margin, shoulderY, shoulder, small));
        place(byId("R"), columns, phone ? normalized(0.8687500000f, 0.2729166667f, shoulder, small)
                : pad ? normalized(0.8960468521f, 0.6400000000f, shoulder, small)
                : rect(safe.right - margin - shoulder, shoulderY, shoulder, small));
        place(byId("Z"), columns, phone ? normalized(0.9712500000f, 0.4350788288f, small, small)
                : pad ? normalized(0.8275988287f, 0.7213029990f, small, small)
                : rect(safe.right - margin - shoulder - small - 12f * scale * dp, shoulderY, small, small));
        final float startWidth = (pad ? 116f : 92f * base) * size * dp;
        place(byId("Start"), columns, phone ? normalized(0.0902222222f, 0.1128941441f, startWidth, small)
                : pad ? normalized(0.8967789165f, 0.5600000000f, startWidth, small)
                : rect(safe.centerX() - startWidth * 0.5f, safe.top + margin, startWidth, small));

        final float d = (pad ? 48f : 36f * base) * size * dp;
        place(byId("Jump"), columns, normalized(0.73f, 0.78f, medium, small));
        place(byId("Sprint"), columns, normalized(0.25f, 0.60f, medium, small));
        place(dpadGroup, columns, phone ? normalized(0.0812777778f, 0.4677364865f, 3f * d, 3f * d)
                : pad ? normalized(0.2686676428f, 0.7947259566f, 3f * d, 3f * d)
                : rect(move.frame.right + 18f * scale * dp, move.frame.centerY() - 1.5f * d, 3f * d, 3f * d));
        layoutDPadButtons();
        move.homeX = move.frame.centerX();
        move.homeY = move.frame.centerY();
        updateAppearance();
    }

    private static RectF rect(float x, float y, float w, float h) { return new RectF(x, y, x + w, y + h); }

    /**
     * Beside a 4:3 picture there is a black bar on each side; the iPhone app's phone defaults were placed for a
     * full-width game and cover Wind Waker's minimap and item HUD, so while the original picture is on and the
     * bars are wide enough the defaults are two columns in the bars: movement, D-pad, L and Start on the left;
     * Z, R, the face buttons and the camera stick on the right. Null when they don't apply (Fill Screen, narrow
     * bars); a saved position still wins over either default.
     */
    private java.util.Map<String, RectF> columnFrames(float size) {
        if (launchAspectMode != 0 || picture.width() <= 0) return null;
        final float gap = 4f * dp;
        final float leftMin = safe.left + gap, leftMax = picture.left - gap;
        final float rightMin = picture.right + gap, rightMax = safe.right - gap;
        final float w = Math.min(leftMax - leftMin, rightMax - rightMin);
        if (w < 84f * dp || safe.height() < 300f * dp) return null;
        final float col = Math.min(w, 120f * dp) * size;  // wider bars keep sizes sane
        final float lx = (leftMin + leftMax) * 0.5f, rx = (rightMin + rightMax) * 0.5f;
        final float top = safe.top, bottom = safe.bottom, h = safe.height();
        final float stick = col, camera = col * 0.80f, d = col * 0.30f;
        final float a = col * 0.60f, b = col * 0.42f, face = col * 0.40f, z = col * 0.36f;
        final float pillH = col * 0.38f;
        java.util.Map<String, RectF> m = new java.util.HashMap<>();
        // left column, top to bottom
        m.put("Start", at(lx, top + 24f * dp, col * 0.72f, col * 0.32f));
        m.put("L", at(lx, top + 24f * dp + col * 0.46f, col * 0.86f, pillH));
        m.put("DPad", at(lx, top + h * 0.47f, 3f * d, 3f * d));
        m.put("move", at(lx, bottom - stick * 0.5f - 6f * dp, stick, stick));
        // right column below the menu button: Z and R, then Y and X, then B and A
        m.put("Z", at(rx - col * 0.5f + z * 0.5f, top + 80f * dp, z, z));
        m.put("R", at(rx + col * 0.5f - col * 0.29f, top + 80f * dp, col * 0.58f, pillH));
        m.put("Y", at(rx - col * 0.25f, top + h * 0.34f, face, face));
        m.put("X", at(rx + col * 0.27f, top + h * 0.34f + col * 0.10f, face, face));
        m.put("A", at(rx + col * 0.16f, top + h * 0.53f, a, a));
        m.put("B", at(rx - col * 0.30f, top + h * 0.53f + col * 0.30f, b, b));
        m.put("c", at(rx, bottom - camera * 0.5f - 6f * dp, camera, camera));
        // Jump and Run (with Jump & Sprint on) go in the columns too; the iPhone app leaves them over the picture,
        // where they cover the minimap and the rupees.
        m.put("Sprint", at(lx, top + h * 0.295f, col * 0.62f, col * 0.34f));
        m.put("Jump", at(rx + col * 0.10f, top + h * 0.715f, col * 0.62f, col * 0.34f));
        return m;
    }

    /**
     * Sparse persistence: a control with no saved position keeps its form factor's default, clamped into the safe
     * area.
     */
    private void place(Control control, java.util.Map<String, RectF> columns, RectF frame) {
        if (columns != null && columns.containsKey(control.id)) frame = columns.get(control.id);
        JSONObject scales = settings.json(Settings.layoutKey(phone, "ControlSizeScales"));
        float individual = (float) clamp(scales.optDouble(control.id, 1.0), 0.6, 1.75);
        float w = frame.width() * individual, h = frame.height() * individual;
        float cx = frame.centerX(), cy = frame.centerY();
        JSONObject origins = settings.json(Settings.layoutKey(phone, "ControlOrigins"));
        float[] saved = parsePoint(origins.optString(control.id, null));
        if (saved != null) {
            cx = safe.left + (float) clamp(saved[0], 0, 1) * safe.width();
            cy = safe.top + (float) clamp(saved[1], 0, 1) * safe.height();
        }
        float hw = Math.min(w * 0.5f, safe.width() * 0.5f), hh = Math.min(h * 0.5f, safe.height() * 0.5f);
        cx = (float) clamp(cx, safe.left + hw, safe.right - hw);
        cy = (float) clamp(cy, safe.top + hh, safe.bottom - hh);
        control.frame.set(cx - w * 0.5f, cy - h * 0.5f, cx + w * 0.5f, cy + h * 0.5f);
    }

    /** NSStringFromCGPoint's "{x, y}", the iPhone app's stored form. */
    private static float[] parsePoint(String s) {
        if (s == null) return null;
        String t = s.replace("{", "").replace("}", "");
        String[] p = t.split(",");
        if (p.length != 2) return null;
        try {
            return new float[] {Float.parseFloat(p[0].trim()), Float.parseFloat(p[1].trim())};
        } catch (NumberFormatException e) {
            return null;
        }
    }

    private static double clamp(double v, double lo, double hi) { return Math.max(lo, Math.min(hi, v)); }

    private void layoutDPadButtons() {
        final float cell = dpadGroup.frame.width() / 3f;
        final float cx = dpadGroup.frame.centerX(), cy = dpadGroup.frame.centerY();
        Object[][] cells = {{"D_U", 0f, -1f}, {"D_D", 0f, 1f}, {"D_L", -1f, 0f}, {"D_R", 1f, 0f}};
        for (Object[] info : cells) {
            Control b = byId((String) info[0]);
            float x = cx + (float) info[1] * cell, y = cy + (float) info[2] * cell;
            b.frame.set(x - cell * 0.5f, y - cell * 0.5f, x + cell * 0.5f, y + cell * 0.5f);
        }
    }

    private void updateAppearance() {
        final boolean hidden = (controllerHidden || !settings.bool(Settings.SHOW_TOUCH, true)) && !editing;
        for (Control c : controls) {
            boolean extraHidden = c.kind == BUTTON && (c.mask == Shell.JUMP || c.mask == Shell.SPRINT)
                    && !settings.bool(Settings.MOVEMENT_EXTRAS, false) && !editing;
            c.hidden = c.kind == DPAD_GROUP ? !editing : hidden || extraHidden;
        }
        invalidate();
    }

    // ---------------------------------------------------------------- drawing

    @Override
    protected void onDraw(Canvas canvas) {
        final float alpha = editing ? 1f : settings.number(Settings.OPACITY, 0.8f);
        for (Control c : controls) {
            if (c.hidden || c.kind == DPAD_GROUP) continue;
            drawControl(canvas, c, alpha);
        }
        if (editing) {
            // The D-pad as one object: its outline, over its four directions.
            Control g = dpadGroup;
            strokePaint.setStrokeWidth((g == selected ? 4f : 3f) * dp);
            strokePaint.setColor(g == selected ? rgba(0.20f, 0.78f, 1.0f, 1.0f) : rgba(1.0f, 0.78f, 0.20f, 0.95f));
            canvas.drawRoundRect(g.frame, 14f * dp, 14f * dp, strokePaint);
        }
    }

    private static int withAlpha(int color, float alpha) {
        return Color.argb(Math.round(Color.alpha(color) * alpha), Color.red(color), Color.green(color),
                Color.blue(color));
    }

    private void drawControl(Canvas canvas, Control c, float alpha) {
        RectF f = c.frame;
        int border = rgba(1f, 1f, 1f, 0.68f);
        float width = 2f;
        if (editing && !isDPadButton(c)) {
            border = c == selected ? rgba(0.20f, 0.78f, 1.0f, 1.0f) : rgba(1.0f, 0.78f, 0.20f, 0.95f);
            width = c == selected ? 4f : 3f;
        }
        canvas.save();
        if (c.pressed && c.kind == BUTTON) canvas.scale(0.92f, 0.92f, f.centerX(), f.centerY());
        float radius = Math.min(f.width(), f.height()) * 0.5f;
        fillPaint.setColor(withAlpha(c.fill, alpha));
        canvas.drawRoundRect(f, radius, radius, fillPaint);
        strokePaint.setStrokeWidth(width * dp);
        strokePaint.setColor(withAlpha(border, alpha));
        scratch.set(f);
        scratch.inset(width * dp * 0.5f, width * dp * 0.5f);
        canvas.drawRoundRect(scratch, radius, radius, strokePaint);
        if (c.kind == STICK) {
            float side = Math.min(f.width(), f.height());
            float thumb = side * 0.42f;
            float travel = side * 0.5f - thumb * 0.5f - 3f * dp;
            fillPaint.setColor(withAlpha(c.thumb, alpha));
            canvas.drawCircle(f.centerX() + c.stickX * travel, f.centerY() - c.stickY * travel, thumb * 0.5f, fillPaint);
        } else if (c.kind == BUTTON) {
            textPaint.setColor(withAlpha(c.titleColor, alpha));
            float textSize = 18f * dp;
            // Long labels in narrow buttons shrink to fit, as UIKit's buttons truncate.
            textPaint.setTextSize(textSize);
            float maxWidth = f.width() - 8f * dp;
            float measured = textPaint.measureText(c.label);
            if (measured > maxWidth && maxWidth > 0) textPaint.setTextSize(textSize * maxWidth / measured);
            Paint.FontMetrics m = textPaint.getFontMetrics();
            canvas.drawText(c.label, f.centerX(), f.centerY() - (m.ascent + m.descent) * 0.5f, textPaint);
        }
        canvas.restore();
    }

    // ---------------------------------------------------------------- input

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        final int action = event.getActionMasked();
        final int index = event.getActionIndex();
        if (editing) return editTouch(event, action, index);
        if (!visibleInPlay()) return false;
        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN:
                if (!pointerDown(event.getPointerId(index), event.getX(index), event.getY(index))
                        && action == MotionEvent.ACTION_DOWN)
                    return false;  // nothing here: the touch is the game's
                break;
            case MotionEvent.ACTION_MOVE:
                for (int i = 0; i < event.getPointerCount(); i++)
                    pointerMove(event.getPointerId(i), event.getX(i), event.getY(i));
                break;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
                pointerUp(event.getPointerId(index));
                break;
            case MotionEvent.ACTION_CANCEL:
                clearTouchInput();
                break;
            default:
                break;
        }
        publish();
        return true;
    }

    private boolean pointerDown(int id, float x, float y) {
        if (id < 0 || id >= pointerControl.length) return false;
        final float slop = 4f * dp;
        // Buttons first (the D-pad's four directions among them), then the camera stick.
        for (Control c : controls) {
            if (c.hidden || c.kind != BUTTON || !c.contains(x, y, slop)) continue;
            pointerControl[id] = c;
            c.pressed = true;
            return true;
        }
        if (!cStick.hidden && cStick.contains(x, y, slop)) {
            pointerControl[id] = cStick;
            stick(cStick, x, y);
            return true;
        }
        if (!move.hidden && move.contains(x, y, slop) && !isStickHeld(move)) {
            pointerControl[id] = move;
            stick(move, x, y);
            return true;
        }
        // The floating movement stick: anywhere on the left half not taken.
        if (!move.hidden && x < getWidth() * 0.5f && !isStickHeld(move)) {
            float r = move.frame.width() * 0.5f;
            float cx = (float) clamp(x, safe.left + r, safe.right - r);
            float cy = (float) clamp(y, safe.top + r, safe.bottom - r);
            move.frame.offsetTo(cx - r, cy - move.frame.height() * 0.5f);
            pointerControl[id] = move;
            stick(move, x, y);
            return true;
        }
        return false;
    }

    private boolean isStickHeld(Control stick) {
        for (Control c : pointerControl) if (c == stick) return true;
        return false;
    }

    private void pointerMove(int id, float x, float y) {
        if (id < 0 || id >= pointerControl.length) return;
        Control c = pointerControl[id];
        if (c != null && c.kind == STICK) stick(c, x, y);
    }

    private void pointerUp(int id) {
        if (id < 0 || id >= pointerControl.length) return;
        Control c = pointerControl[id];
        pointerControl[id] = null;
        if (c == null) return;
        if (c.kind == STICK) {
            c.stickX = c.stickY = 0f;
            if (c == move) move.frame.offsetTo(move.homeX - move.frame.width() * 0.5f,
                    move.homeY - move.frame.height() * 0.5f);
        } else {
            c.pressed = false;
        }
        invalidate();
    }

    /** Deflection is the touch's offset over the radius, clamped to the unit circle, +y up. */
    private void stick(Control c, float x, float y) {
        float radius = Math.max(1f, Math.min(c.frame.width(), c.frame.height()) * 0.5f);
        float dx = (x - c.frame.centerX()) / radius, dy = (y - c.frame.centerY()) / radius;
        float length = (float) Math.hypot(dx, dy);
        if (length > 1f) {
            dx /= length;
            dy /= length;
        }
        c.stickX = dx;
        c.stickY = -dy;
        invalidate();
    }

    private void publish() {
        int buttons = 0;
        for (Control c : controls) if (c.kind == BUTTON && c.pressed) buttons |= c.mask;
        Shell.nativeTouchPublish(buttons, Math.round(move.stickX * 127f), Math.round(move.stickY * 127f),
                Math.round(cStick.stickX * 127f), Math.round(cStick.stickY * 127f));
        invalidate();
    }

    /** Every held control lets go (a menu opens, the app goes to the background, a controller connects). */
    void clearTouchInput() {
        for (int i = 0; i < pointerControl.length; i++) pointerControl[i] = null;
        for (Control c : controls) {
            c.pressed = false;
            c.stickX = c.stickY = 0f;
        }
        move.frame.offsetTo(move.homeX - move.frame.width() * 0.5f, move.homeY - move.frame.height() * 0.5f);
        try {
            Shell.nativeTouchClear();
        } catch (UnsatisfiedLinkError ignored) {
        }
        invalidate();
    }

    // ---------------------------------------------------------------- layout editor

    void beginLayoutEditing() {
        clearTouchInput();
        editing = true;
        selected = null;
        relayout();
    }

    void endLayoutEditing() {
        if (!editing) return;
        clearTouchInput();
        editing = false;
        selected = null;
        relayout();
    }

    /** The selected control's own size, 0.6 to 1.75, from the editor bar's slider. */
    void setSelectedScale(float value) {
        if (!editing || selected == null) return;
        JSONObject scales = settings.json(Settings.layoutKey(phone, "ControlSizeScales"));
        try {
            scales.put(selected.id, clamp(value, 0.6, 1.75));
        } catch (JSONException e) {
            return;
        }
        settings.putJson(Settings.layoutKey(phone, "ControlSizeScales"), scales);
        relayout();
    }

    void resetLayout() {
        settings.remove(Settings.layoutKey(phone, "ControlOrigins"));
        settings.remove(Settings.layoutKey(phone, "ControlSizeScales"));
        relayout();
    }

    private Control editTarget(float x, float y) {
        // The D-pad's group, not its four directions; the topmost control first.
        if (dpadGroup.contains(x, y, 0)) return dpadGroup;
        for (int i = controls.size() - 1; i >= 0; i--) {
            Control c = controls.get(i);
            if (c.kind == DPAD_GROUP || isDPadButton(c) || c.hidden) continue;
            if (c.contains(x, y, 2f * dp)) return c;
        }
        return null;
    }

    private boolean editTouch(MotionEvent event, int action, int index) {
        final int id = event.getPointerId(index);
        if (id < 0 || id >= pointerControl.length) return true;
        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN: {
                Control c = editTarget(event.getX(index), event.getY(index));
                if (c == null) return action != MotionEvent.ACTION_DOWN;
                pointerControl[id] = c;
                dragOffsetX[id] = c.frame.centerX() - event.getX(index);
                dragOffsetY[id] = c.frame.centerY() - event.getY(index);
                dragMoved = false;
                select(c);
                break;
            }
            case MotionEvent.ACTION_MOVE:
                for (int i = 0; i < event.getPointerCount(); i++) {
                    int pid = event.getPointerId(i);
                    if (pid < 0 || pid >= pointerControl.length || pointerControl[pid] == null) continue;
                    Control c = pointerControl[pid];
                    float hw = Math.min(c.frame.width() * 0.5f, safe.width() * 0.5f);
                    float hh = Math.min(c.frame.height() * 0.5f, safe.height() * 0.5f);
                    float cx = (float) clamp(event.getX(i) + dragOffsetX[pid], safe.left + hw, safe.right - hw);
                    float cy = (float) clamp(event.getY(i) + dragOffsetY[pid], safe.top + hh, safe.bottom - hh);
                    c.frame.offsetTo(cx - c.frame.width() * 0.5f, cy - c.frame.height() * 0.5f);
                    if (c == dpadGroup) layoutDPadButtons();
                    if (c == move) {
                        move.homeX = cx;
                        move.homeY = cy;
                    }
                    dragMoved = true;
                }
                invalidate();
                break;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_CANCEL: {
                Control c = pointerControl[id];
                pointerControl[id] = null;
                if (c != null && dragMoved) saveOrigin(c);
                break;
            }
            default:
                break;
        }
        return true;
    }

    private void saveOrigin(Control c) {
        if (safe.width() <= 0 || safe.height() <= 0) return;
        JSONObject origins = settings.json(Settings.layoutKey(phone, "ControlOrigins"));
        float nx = (c.frame.centerX() - safe.left) / safe.width();
        float ny = (c.frame.centerY() - safe.top) / safe.height();
        try {
            origins.put(c.id, String.format(Locale.US, "{%s, %s}", nx, ny));
        } catch (JSONException e) {
            return;
        }
        settings.putJson(Settings.layoutKey(phone, "ControlOrigins"), origins);
    }

    private void select(Control c) {
        selected = c;
        JSONObject scales = settings.json(Settings.layoutKey(phone, "ControlSizeScales"));
        float scale = (float) scales.optDouble(c.id, 1.0);
        if (listener != null) listener.onEditorSelection(c.id, c.name, scale);
        invalidate();
    }
}
