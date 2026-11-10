package dev.bluewake.android;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.opengl.GLES20;
import android.os.Build;
import android.os.PowerManager;
import android.view.View;

import java.util.Locale;

/**
 * The performance panel: the frame rate and the frame times with a graph of them, and the machine's processor,
 * memory and graphics card — the Android counterpart of the desktop's F9 panel (runtime/host/src/perf_overlay.h,
 * drawn there by desktop_menu.cpp).
 *
 * The numbers are perf_monitor.c's, through Shell.nativePerfSnapshot; the shapes are this file's, in the desktop
 * panel's colours and order, in the Android idiom: a rounded translucent plate in the picture's top right corner,
 * over the game and under the menu. A bar under a number is that number's share of something (a load against 100%, a
 * memory against the whole of it, a clock against the card's top, a temperature against 100 C, a frame time against
 * the graph's 50 ms floor).
 *
 * A phone withholds more than a desktop: /proc/stat is denied to apps, so there is no machine-wide load and the row
 * shows the game's own share; the thermal zones are denied too, so the platform's thermal status stands beside the
 * numbers where the temperature would be; and the card publishes no counters, so its clock and the processors' mean
 * clock are what the platform will give. Whatever is left unknown reads "-". The parts' names come from the build and
 * from the driver's own GL renderer, which is also where the card's name comes from when it is not in the panel.
 */
@SuppressLint("ViewConstructor")
final class PerfPanelView extends View {

    /** What Shell.nativePerfSnapshot fills: the numbers, then the frame times kept for the graph. */
    static final int FIELDS = 18 + 120;
    private static final int FPS = 0, MS_AVG = 1, MS_MAX = 2, CPU_TOTAL = 3, CPU_GAME = 4, CPU_TEMP = 5,
            RAM_USED = 6, RAM_TOTAL = 7, RAM_GAME = 8, GPU_PCT = 9, GPU_TEMP = 10, GPU_POWER = 11,
            GPU_CLOCK = 12, VRAM_USED = 13, VRAM_TOTAL = 14, FRAME_COUNT = 15, CPU_CLOCK = 16,
            CPU_CLOCK_MAX = 17, FRAMES = 18;

    private static final int GPU = Color.rgb(140, 211, 79);
    private static final int CPU = Color.rgb(47, 150, 204);
    private static final int RAM = Color.rgb(204, 115, 179);
    private static final int RATE = Color.rgb(242, 242, 242);
    private static final int NAME = Color.rgb(209, 209, 219);
    private static final int DIM = Color.argb(130, 255, 255, 255);
    private static final int LINE = Color.rgb(237, 194, 84);
    /** A value that is a plain number (a memory's total, a status): no bar. */
    private static final float NO_BAR = -1f;

    private final float[] values = new float[FIELDS];
    private final String[] names = new String[3];
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint label = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint value = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint footer = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint caption = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path graph = new Path();
    private final RectF box = new RectF();
    private final float dp, row, gap, graphHeight, captionHeight, footerHeight, pad, labelWidth, col1, col2, col3,
            round;
    // Which rows this phone can fill at all. Decided from the first numbers, not per frame, so the panel does not
    // change shape under the eye; a phone that reports no graphics card counters keeps that row out rather than
    // showing four dashes forever.
    private boolean decided, showGpu, showVram;
    // What the platform says about itself: neither name is in sysfs an app may read.
    private String socName, gpuName, thermal;
    private boolean probed;

    PerfPanelView(Context context) {
        super(context);
        dp = getResources().getDisplayMetrics().density;
        row = 19f * dp;
        gap = 4f * dp;
        graphHeight = 46f * dp;
        footerHeight = 15f * dp;
        pad = 10f * dp;
        labelWidth = 62f * dp;
        col1 = 62f * dp;
        col2 = 88f * dp;
        col3 = 94f * dp;
        round = 9f * dp;
        captionHeight = 15f * dp;   // the graph's own line: what it plots, and its top
        setPaint(label, 12f, "sans-serif-medium", DIM);
        setPaint(value, 13f, "sans-serif-medium", Color.WHITE);
        setPaint(footer, 11f, "sans-serif", NAME);
        setPaint(caption, 11f, "sans-serif", DIM);
        value.setFontFeatureSettings("tnum");
        label.setFontFeatureSettings("tnum");
        footer.setFontFeatureSettings("tnum");
    }

    private void setPaint(Paint p, float size, String family, int color) {
        p.setTextSize(size * dp);
        p.setTypeface(Typeface.create(family, Typeface.NORMAL));
        p.setColor(color);
    }

    /**
     * The two names and the thermal status, once: the build names the SoC, the driver names the card, and the power
     * service gives the thermal status — none of them needs a permission, where sysfs is denied.
     */
    private void probe(Context context) {
        probed = true;
        if (Build.VERSION.SDK_INT >= 31) {
            socName = Build.SOC_MODEL;
        }
        if (socName == null || socName.isEmpty()) {
            socName = Build.HARDWARE;
        }
        gpuName = glRenderer();
        thermal = thermalWord(context);
    }

    /** The card's name from the driver that draws it: an off-screen EGL surface, GL_RENDERER, then teardown. */
    private String glRenderer() {
        final EGLDisplay display = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        if (display == EGL14.EGL_NO_DISPLAY) {
            return null;
        }
        final int[] version = new int[2];
        if (!EGL14.eglInitialize(display, version, 0, version, 1)) {
            return null;
        }
        final EGLConfig[] configs = new EGLConfig[1];
        final int[] configAttribs = {EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT, EGL14.EGL_NONE};
        if (!EGL14.eglChooseConfig(display, configAttribs, 0, configs, 0, 1, new int[1], 0) || configs[0] == null) {
            EGL14.eglTerminate(display);
            return null;
        }
        final int[] contextAttribs = {EGL14.EGL_CONTEXT_CLIENT_VERSION, 2, EGL14.EGL_NONE};
        final EGLContext eglContext = EGL14.eglCreateContext(display, configs[0], EGL14.EGL_NO_CONTEXT, contextAttribs, 0);
        if (eglContext == EGL14.EGL_NO_CONTEXT) {
            EGL14.eglTerminate(display);
            return null;
        }
        final int[] surfaceAttribs = {EGL14.EGL_WIDTH, 1, EGL14.EGL_HEIGHT, 1, EGL14.EGL_NONE};
        final EGLSurface surface = EGL14.eglCreatePbufferSurface(display, configs[0], surfaceAttribs, 0);
        String renderer = null;
        if (surface != EGL14.EGL_NO_SURFACE && EGL14.eglMakeCurrent(display, surface, surface, eglContext)) {
            renderer = GLES20.glGetString(GLES20.GL_RENDERER);
            EGL14.eglMakeCurrent(display, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT);
        }
        if (surface != EGL14.EGL_NO_SURFACE) {
            EGL14.eglDestroySurface(display, surface);
        }
        EGL14.eglDestroyContext(display, eglContext);
        EGL14.eglTerminate(display);
        return renderer;
    }

    /** The platform's own thermal answer, where the phone's zones are not readable. */
    private static String thermalWord(Context context) {
        if (Build.VERSION.SDK_INT < 29) {
            return null;
        }
        final PowerManager power = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        if (power == null) {
            return null;
        }
        switch (power.getCurrentThermalStatus()) {
            case PowerManager.THERMAL_STATUS_LIGHT:
                return "light";
            case PowerManager.THERMAL_STATUS_MODERATE:
                return "moderate";
            case PowerManager.THERMAL_STATUS_SEVERE:
                return "severe";
            case PowerManager.THERMAL_STATUS_CRITICAL:
                return "critical";
            case PowerManager.THERMAL_STATUS_EMERGENCY:
                return "emergency";
            case PowerManager.THERMAL_STATUS_SHUTDOWN:
                return "shutdown";
            case PowerManager.THERMAL_STATUS_NONE:
            default:
                return "nominal";
        }
    }

    /** Read the numbers again: called on the overlay's half second tick while the panel is shown. */
    void refresh() {
        try {
            Shell.nativePerfSnapshot(values, names);
        } catch (UnsatisfiedLinkError ignored) {
            // Before libmain.so is loaded: the next tick reads them.
            return;
        }
        if (!probed) {
            probe(getContext());
        }
        if (!decided && known(values[FPS]) && values[FPS] > 0f) {
            decided = true;
            showGpu = known(values[GPU_PCT]) || known(values[GPU_TEMP]) || known(values[GPU_CLOCK]);
            showVram = known(values[VRAM_USED]) || known(values[VRAM_TOTAL]);
            requestLayout();
        }
        invalidate();
    }

    private static boolean known(float v) {
        return !Float.isNaN(v) && v >= 0f;
    }

    private static String fmt(String format, float v) {
        return String.format(Locale.US, format, v);
    }

    private static String fmt(String format, float a, float b) {
        return String.format(Locale.US, format, a, b);
    }

    /** The card's name: the sampler's if the loader gave it, else the renderer's own. */
    private String card() {
        if (names[1] != null && !names[1].isEmpty()) {
            return names[1];
        }
        return gpuName != null && !gpuName.isEmpty() ? gpuName : null;
    }

    /** The processor's name: the sampler's (a Linux sysfs SoC) or the build's. */
    private String processor() {
        if (names[0] != null && !names[0].isEmpty()) {
            return names[0];
        }
        return socName != null && !socName.isEmpty() ? socName : null;
    }

    private float panelWidth() {
        return pad * 2f + labelWidth + col1 + col2 + col3;
    }

    private int rowCount() {
        return 3 + (showGpu ? 1 : 0) + (showVram ? 1 : 0);   // CPU, RAM, frame, and the card's rows when it has any
    }

    private String[] footerLines() {
        final String[] all = {card(), processor(), thermal != null ? "thermal: " + thermal : null};
        int n = 0;
        for (String line : all) {
            if (line != null && !line.isEmpty()) {
                ++n;
            }
        }
        final String[] kept = new String[n];
        n = 0;
        for (String line : all) {
            if (line != null && !line.isEmpty()) {
                kept[n++] = line;
            }
        }
        return kept;
    }

    private float panelHeight() {
        return pad * 2f + row * rowCount() + gap * 2f + captionHeight + graphHeight
                + footerHeight * footerLines().length;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        setMeasuredDimension(panelWidthPx(), panelHeightPx());
    }

    /** The plate's size in pixels, for the overlay's layout (it is WRAP_CONTENT otherwise). */
    int panelWidthPx() {
        return Math.round(panelWidth());
    }

    int panelHeightPx() {
        return Math.round(panelHeight());
    }

    @Override
    protected void onDraw(Canvas canvas) {
        final float w = panelWidth();
        paint.setColor(Color.argb(158, 0, 0, 0));
        box.set(0f, 0f, w, panelHeight());
        canvas.drawRoundRect(box, round, round, paint);

        float y = pad;
        if (showGpu)
            y = part(canvas, y, "GPU", GPU,
                    known(values[GPU_PCT]) ? fmt("%.0f%%", values[GPU_PCT]) : null,
                    known(values[GPU_PCT]) ? values[GPU_PCT] / 100f : NO_BAR,
                    known(values[GPU_TEMP]) ? fmt("%.0f C", values[GPU_TEMP]) : null,
                    known(values[GPU_TEMP]) ? values[GPU_TEMP] / 100f : NO_BAR,
                    known(values[GPU_CLOCK]) ? fmt("%.0f MHz", values[GPU_CLOCK]) : null,
                    known(values[GPU_CLOCK]) ? values[GPU_CLOCK] / 3000f : NO_BAR);
        if (showVram)
            y = part(canvas, y, "VRAM", GPU,
                    known(values[VRAM_USED]) ? fmt("%.1f GB", values[VRAM_USED] / 1024f) : null,
                    known(values[VRAM_TOTAL]) && values[VRAM_TOTAL] > 0f ? values[VRAM_USED] / values[VRAM_TOTAL] : NO_BAR,
                    known(values[VRAM_TOTAL]) ? "of " + fmt("%.1f GB", values[VRAM_TOTAL] / 1024f) : null, NO_BAR,
                    null, NO_BAR);
        // The processor: the machine-wide share and the temperature where the platform gives them; where it does not
        // (a phone), the mean clock of its cores with the fastest of them as the bar's top.
        final boolean clocked = !known(values[CPU_TEMP]) && known(values[CPU_CLOCK]);
        y = part(canvas, y, "CPU", CPU,
                known(values[CPU_TOTAL]) ? fmt("%.0f%%", values[CPU_TOTAL]) : null,
                known(values[CPU_TOTAL]) ? values[CPU_TOTAL] / 100f : NO_BAR,
                known(values[CPU_TEMP]) ? fmt("%.0f C", values[CPU_TEMP])
                        : clocked ? fmt("%.2f/%.1f GHz", values[CPU_CLOCK] / 1000f, values[CPU_CLOCK_MAX] / 1000f) : null,
                known(values[CPU_TEMP]) ? values[CPU_TEMP] / 100f
                        : clocked && values[CPU_CLOCK_MAX] > 0f ? values[CPU_CLOCK] / values[CPU_CLOCK_MAX] : NO_BAR,
                known(values[CPU_GAME]) ? "game " + fmt("%.0f%%", values[CPU_GAME]) : null,
                known(values[CPU_GAME]) ? values[CPU_GAME] / 100f : NO_BAR);
        y = part(canvas, y, "RAM", RAM,
                known(values[RAM_USED]) ? fmt("%.1f GB", values[RAM_USED] / 1024f) : null,
                known(values[RAM_TOTAL]) && values[RAM_TOTAL] > 0f ? values[RAM_USED] / values[RAM_TOTAL] : NO_BAR,
                known(values[RAM_TOTAL]) ? "of " + fmt("%.1f GB", values[RAM_TOTAL] / 1024f) : null, NO_BAR,
                known(values[RAM_GAME]) ? "game " + fmt("%.1f GB", values[RAM_GAME] / 1024f) : null,
                known(values[RAM_TOTAL]) && values[RAM_TOTAL] > 0f ? values[RAM_GAME] / values[RAM_TOTAL] : NO_BAR);
        y = part(canvas, y, known(values[FPS]) && values[FPS] > 0f ? fmt("%.0f FPS", values[FPS]) : "-", RATE,
                known(values[MS_AVG]) ? fmt("%.1f ms", values[MS_AVG]) : null,
                known(values[MS_AVG]) ? values[MS_AVG] / 50f : NO_BAR,
                known(values[MS_MAX]) ? fmt("%.1f ms", values[MS_MAX]) : null,
                known(values[MS_MAX]) ? values[MS_MAX] / 50f : NO_BAR,
                null, NO_BAR);
        y += gap;
        drawGraphCaption(canvas, y, w);
        y += captionHeight;
        drawGraph(canvas, y, w);
        y += graphHeight + gap;
        // The names last and brighter than a disabled line, as on the desktop: they are the last thing read, and the
        // game behind them is busy.
        for (String line : footerLines()) {
            canvas.drawText(line, pad, y + footerHeight * 0.8f, footer);
            y += footerHeight;
        }
    }

    /**
     * One part: its name in the part's colour, then up to three numbers in the panel's columns, each right aligned,
     * each with a thin bar under it when it is a share (a negative share means a plain number).
     */
    private float part(Canvas canvas, float y, String title, int color, String first, float firstShare,
                       String second, float secondShare, String third, float thirdShare) {
        final float baseline = y + row * 0.72f;
        label.setColor(color);
        canvas.drawText(title, pad, baseline, label);
        final float x1 = pad + labelWidth, x2 = x1 + col1, x3 = x2 + col2;
        column(canvas, x1, col1, baseline, first, firstShare, color);
        column(canvas, x2, col2, baseline, second, secondShare, color);
        column(canvas, x3, col3, baseline, third, thirdShare, color);
        return y + row;
    }

    private void column(Canvas canvas, float x, float width, float baseline, String text, float share, int color) {
        final float right = x + width;
        if (text == null || text.isEmpty()) {
            value.setColor(DIM);
            canvas.drawText("-", right - value.measureText("-"), baseline, value);
            return;
        }
        value.setColor(Color.WHITE);
        canvas.drawText(text, right - value.measureText(text), baseline, value);
        if (share < 0f) {
            return;
        }
        final float bar = 2.2f * dp;
        final float top = baseline + 2.6f * dp;
        paint.setColor(Color.argb(52, Color.red(color), Color.green(color), Color.blue(color)));
        box.set(x, top, right, top + bar);
        canvas.drawRoundRect(box, bar * 0.5f, bar * 0.5f, paint);
        final float filled = (right - x) * Math.min(1f, share);
        if (filled > 0f) {
            paint.setColor(color);
            box.set(x, top, x + Math.max(filled, bar), top + bar);
            canvas.drawRoundRect(box, bar * 0.5f, bar * 0.5f, paint);
        }
    }

    /** What the graph plots, and the frame time at its top: a line is only a number with a scale beside it. */
    private void drawGraphCaption(Canvas canvas, float y, float w) {
        float top = known(values[MS_MAX]) ? values[MS_MAX] : 0f;
        if (top < 50f) {
            top = 50f;
        }
        final float baseline = y + captionHeight * 0.85f;
        canvas.drawText("frame time, ms", pad, baseline, caption);
        final String scale = String.format(Locale.US, "0\u2013%.0f ms", top);
        canvas.drawText(scale, w - pad - caption.measureText(scale), baseline, caption);
    }

    /**
     * The frame times as a graph: the worst frame sets its top and 50 ms (20 FPS) is the least height it shows, so a
     * good run still reads as a line — the same rule as the desktop panel's.
     */
    private void drawGraph(Canvas canvas, float y, float w) {
        final float left = pad, right = w - pad;
        box.set(left, y, right, y + graphHeight);
        paint.setColor(Color.argb(89, 0, 0, 0));
        canvas.drawRoundRect(box, 4f * dp, 4f * dp, paint);
        paint.setColor(Color.argb(40, 255, 255, 255));
        canvas.drawLine(left, y + graphHeight * 0.5f, right, y + graphHeight * 0.5f, paint);

        final int count = Math.min(120, Math.max(0, (int) values[FRAME_COUNT]));
        if (count < 2) {
            return;
        }
        float top = known(values[MS_MAX]) ? values[MS_MAX] : 0f;
        if (top < 50f) {
            top = 50f;
        }
        final float step = (right - left) / 119f;
        graph.reset();
        for (int i = 0; i < count; i++) {
            final float ms = values[FRAMES + i];
            final float px = right - step * (count - 1 - i);
            final float py = y + graphHeight - 1.5f * dp
                    - Math.min(1f, Math.max(0f, ms / top)) * (graphHeight - 3f * dp);
            if (i == 0) {
                graph.moveTo(px, py);
            } else {
                graph.lineTo(px, py);
            }
        }
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(1.4f * dp);
        paint.setStrokeJoin(Paint.Join.ROUND);
        paint.setColor(LINE);
        canvas.drawPath(graph, paint);
        paint.setStyle(Paint.Style.FILL);
    }
}
