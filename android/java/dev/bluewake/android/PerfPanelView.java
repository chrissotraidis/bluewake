package dev.bluewake.android;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.os.BatteryManager;
import android.os.PowerManager;
import android.view.View;

import java.util.Locale;

/**
 * The performance bar: one row across the top of the picture, in the shape of the Steam Deck's overlay — the frame
 * rate with a graph of the frame times beside it, then the processor, the graphics card, memory and the battery, each a
 * name in its part's colour with the numbers that belong to it.
 *
 * The numbers are perf_monitor.c's, through Shell.nativePerfSnapshot; the shapes are this file's. A bar under a number
 * is that number's share of something (a load against 100%, a memory against the whole of it, a frame time against the
 * graph's floor, a battery against full). A part the phone cannot report is left out rather than drawn as a row of
 * dashes: /proc/stat and the thermal zones are denied to apps, so the processor's number is the game's own share and
 * its second column is the mean clock of the cores — or the platform's thermal word, in the warning colour, once the
 * phone is warm enough (moderate or worse; a phone reads "light" for most of a session) — and a card that publishes no
 * counters loses its part of the bar. The segments are
 * measured from the widest value each one can print, and the least useful are dropped when the picture is narrow: once
 * per layout, never per frame, so the bar does not change shape under the eye.
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
    private static final int BATT = Color.rgb(209, 209, 219);
    private static final int RATE = Color.rgb(242, 242, 242);
    private static final int DIM = Color.argb(150, 255, 255, 255);
    private static final int CAPTION = Color.argb(200, 255, 255, 255);
    private static final int LINE = Color.rgb(237, 194, 84);
    private static final int WARN = Color.rgb(255, 204, 77);
    /** A value that is a plain number (a memory's total, a status): no bar. */
    private static final float NO_BAR = -1f;

    // The widest each column can print, so a number's growth never moves the row: a share of one core passes 100% on
    // a phone, a memory's two halves are both below 99.9 GB, and a status word is at most "emergency".
    private static final String RATE_RESERVE = "888", SHARE_RESERVE = "game 888%", PLAIN_SHARE_RESERVE = "888%",
            MEMORY_RESERVE = "88.8/88.8 GB", GAME_MEMORY_RESERVE = "game 88.8 GB", CLOCK_RESERVE = "8.8 GHz",
            SECOND_RESERVE = "emergency", GPU_CLOCK_RESERVE = "8888 MHz";

    private final float[] values = new float[FIELDS];
    /** What Shell.nativePerfSnapshot writes the parts' names into: the bar is too narrow to carry them. */
    private final String[] names = new String[3];
    private final Paint plate = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint label = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint value = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint rate = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint dim = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint caption = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint plot = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path line = new Path();
    private final RectF box = new RectF();

    private final float dp, padX, padY, inner, gap, gapPart, barHeight, radius, maxGraph, minGraph;
    // What this phone can fill at all, decided from the first numbers: a card with no counters loses its part.
    private boolean decided, showGpu;
    // What fits across the picture at this width: the optional columns, and the graph's width within that budget.
    private boolean fitClock, fitGameMem, fitGpu;
    private float graphWidth = -1f;
    private String thermal;

    PerfPanelView(Context context) {
        super(context);
        dp = getResources().getDisplayMetrics().density;
        padX = 10f * dp;
        padY = 5f * dp;
        inner = 26f * dp;
        gap = 14f * dp;
        gapPart = 4f * dp;
        barHeight = 3f * dp;
        radius = 9f * dp;
        maxGraph = 96f * dp;
        minGraph = 60f * dp;
        setPaint(label, 10f, "sans-serif-medium", DIM);
        setPaint(value, 13f, "sans-serif-medium", Color.WHITE);
        setPaint(rate, 17f, "sans-serif-medium", RATE);
        setPaint(dim, 11f, "sans-serif", DIM);
        setPaint(caption, 8f, "sans-serif", DIM);
        setPaint(plot, 1.4f * dp, "sans-serif", LINE);
        plot.setStyle(Paint.Style.STROKE);
        for (Paint p : new Paint[] {label, value, rate, dim, caption}) {
            p.setFontFeatureSettings("tnum");
        }
        thermal = thermalWord(context);
    }

    private void setPaint(Paint p, float size, String family, int color) {
        p.setTextSize(size * dp);
        p.setTypeface(Typeface.create(family, Typeface.NORMAL));
        p.setColor(color);
    }

    /** The platform's own thermal answer, where the phone's zones are not readable. */
    private static String thermalWord(Context context) {
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

    /**
     * Whether the platform is warm enough that its thermal word says more than the clock: a phone reads "light" for
     * most of a session, which would cost the clock for nothing, so only moderate and worse take the column.
     */
    private boolean warm() {
        return thermal != null && !thermal.equals("nominal") && !thermal.equals("light");
    }

    /** The battery's charge, which the platform answers without a permission; -1 where it will not. */
    private int batteryPercent() {
        final BatteryManager battery = (BatteryManager) getContext().getSystemService(Context.BATTERY_SERVICE);
        if (battery == null) {
            return -1;
        }
        final int percent = battery.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY);
        return percent >= 0 && percent <= 100 ? percent : -1;
    }

    /** Read the numbers again: called on the overlay's half second tick while the bar is shown. */
    void refresh() {
        try {
            Shell.nativePerfSnapshot(values, names);
        } catch (UnsatisfiedLinkError ignored) {
            // Before libmain.so is loaded: the next tick reads them.
            return;
        }
        thermal = thermalWord(getContext());
        if (!decided && known(values[FPS]) && values[FPS] > 0f) {
            decided = true;
            showGpu = known(values[GPU_PCT]) || known(values[GPU_CLOCK]);
            layoutRow();
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

    // ---------------------------------------------------------------- the row's budget

    private float text(Paint p, String s) {
        return p.measureText(s);
    }

    /** A part's width: its name, then the widest number it can print. */
    private float part(String title, String reserve) {
        return text(label, title) + gapPart + text(value, reserve);
    }

    /** Everything the bar would draw with its current options on, in pixels. */
    private float rowWidth() {
        float w = text(label, "FPS") + gapPart + text(rate, RATE_RESERVE);
        w += gap + graphWidth;
        w += gap + part("CPU", SHARE_RESERVE);
        if (fitClock) {
            w += gapPart + text(dim, SECOND_RESERVE);
        }
        if (fitGpu) {
            w += gap + part("GPU", PLAIN_SHARE_RESERVE) + gapPart + text(dim, GPU_CLOCK_RESERVE);
        }
        w += gap + part("RAM", MEMORY_RESERVE);
        if (fitGameMem) {
            w += gapPart + text(dim, GAME_MEMORY_RESERVE);
        }
        w += gap + part("BATT", PLAIN_SHARE_RESERVE);
        return w;
    }

    /**
     * Fits the row to the picture. The segments are measured from the widest value each can print, and the least
     * useful are dropped in order until the row fits — once per layout, so the bar is fixed for the width it has.
     */
    private void layoutRow() {
        final float available = getWidth() - padX * 2f;
        if (available <= 0f) {
            return;
        }
        fitClock = true;
        fitGameMem = true;
        fitGpu = showGpu;
        graphWidth = maxGraph;
        for (int attempt = 0; attempt < 8; attempt++) {
            if (rowWidth() <= available) {
                break;
            }
            if (fitGameMem) {
                fitGameMem = false;
            } else if (fitClock) {
                fitClock = false;
            } else if (fitGpu) {
                fitGpu = false;
            } else if (graphWidth > minGraph) {
                graphWidth = Math.max(minGraph, graphWidth - 18f * dp);
            } else {
                break;   // nothing left worth dropping: the row is drawn as it is
            }
        }
        invalidate();
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        if (w != oldw || graphWidth < 0f) {
            layoutRow();
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        final int w = MeasureSpec.getMode(widthMeasureSpec) == MeasureSpec.EXACTLY
                ? MeasureSpec.getSize(widthMeasureSpec)
                : Math.round(maxGraph * 3f);
        setMeasuredDimension(w, panelHeightPx());
    }

    /** The bar's height in pixels: one row of numbers and the little bar under each of them. */
    int panelHeightPx() {
        return Math.round(padY * 2f + inner);
    }

    // ---------------------------------------------------------------- drawing

    @Override
    protected void onDraw(Canvas canvas) {
        final float w = getWidth(), h = panelHeightPx();
        if (graphWidth < 0f) {
            layoutRow();
        }
        plate.setColor(Color.argb(158, 0, 0, 0));
        box.set(0f, 0f, w, h);
        canvas.drawRoundRect(box, radius, radius, plate);

        final Paint.FontMetrics metrics = value.getFontMetrics();
        final float baseline = padY + inner * 0.5f - (metrics.ascent + metrics.descent) * 0.5f;
        float x = padX;

        // The frame rate first, as the Deck's bar has it, with the graph of the frame times beside it.
        label.setColor(DIM);
        canvas.drawText("FPS", x, baseline, label);
        x += text(label, "FPS") + gapPart;
        rate.setColor(known(values[FPS]) && values[FPS] > 0f && values[FPS] < 27f ? WARN : RATE);
        final String shown = known(values[FPS]) && values[FPS] > 0f ? fmt("%.0f", values[FPS]) : "-";
        final float rateWidth = text(rate, RATE_RESERVE);
        canvas.drawText(shown, x + rateWidth - text(rate, shown), baseline, rate);
        x += rateWidth;

        x = drawGraph(canvas, x + gap, padY, graphWidth, inner) + gap;

        // The processor: the game's own share, because the machine's is not readable by an app, and then the mean
        // clock of the cores — or the platform's thermal word, in the warning colour, once the phone is warm enough
        // for that to be the more useful of the two.
        final String share = known(values[CPU_GAME]) ? "game " + fmt("%.0f%%", values[CPU_GAME]) : null;
        x = drawPart(canvas, x, baseline, "CPU", CPU, share, SHARE_RESERVE,
                known(values[CPU_GAME]) ? values[CPU_GAME] / 100f : NO_BAR) + gap;
        if (fitClock) {
            final boolean throttled = warm();
            final String second = throttled ? thermal
                    : known(values[CPU_CLOCK]) ? fmt("%.1f GHz", values[CPU_CLOCK] / 1000f) : null;
            if (second != null) {
                dim.setColor(throttled ? WARN : DIM);
                canvas.drawText(second, x, baseline, dim);
            }
            x += text(dim, SECOND_RESERVE) + gap;
        }

        if (fitGpu) {
            x = drawPart(canvas, x, baseline, "GPU", GPU,
                    known(values[GPU_PCT]) ? fmt("%.0f%%", values[GPU_PCT]) : null, PLAIN_SHARE_RESERVE,
                    known(values[GPU_PCT]) ? values[GPU_PCT] / 100f : NO_BAR) + gapPart;
            if (known(values[GPU_CLOCK])) {
                dim.setColor(DIM);
                canvas.drawText(fmt("%.0f MHz", values[GPU_CLOCK]), x, baseline, dim);
            }
            x += text(dim, GPU_CLOCK_RESERVE) + gap;
        }

        final String memory = known(values[RAM_TOTAL]) && values[RAM_TOTAL] > 0f
                ? known(values[RAM_USED]) ? fmt("%.1f/%.1f GB", values[RAM_USED] / 1024f, values[RAM_TOTAL] / 1024f)
                : null
                : known(values[RAM_USED]) ? fmt("%.1f GB", values[RAM_USED] / 1024f) : null;
        x = drawPart(canvas, x, baseline, "RAM", RAM, memory, MEMORY_RESERVE,
                known(values[RAM_TOTAL]) && values[RAM_TOTAL] > 0f && known(values[RAM_USED])
                        ? values[RAM_USED] / values[RAM_TOTAL] : NO_BAR) + gapPart;
        if (fitGameMem) {
            if (known(values[RAM_GAME])) {
                dim.setColor(DIM);
                canvas.drawText("game " + fmt("%.1f GB", values[RAM_GAME] / 1024f), x, baseline, dim);
            }
            x += text(dim, GAME_MEMORY_RESERVE) + gap;
        }

        // The battery closes the row at the bar's right edge, as the Deck's does: the room the reserved widths leave
        // over becomes one gap before it rather than a tail after it.
        final float batteryWidth = part("BATT", PLAIN_SHARE_RESERVE);
        final int battery = batteryPercent();
        drawPart(canvas, Math.max(x, w - padX - batteryWidth), baseline, "BATT",
                battery >= 0 && battery <= 15 ? WARN : BATT, battery >= 0 ? battery + "%" : null,
                PLAIN_SHARE_RESERVE, battery >= 0 ? battery / 100f : NO_BAR);
    }

    /**
     * One part: its name in the part's colour, then its number, under which a thin bar shows the number's share of
     * what it is measured against (a negative share means a plain number, and draws none). Returns the row's next x:
     * the part is given the width of its widest number, so the columns do not move as the numbers change.
     */
    private float drawPart(Canvas canvas, float x, float baseline, String title, int color, String text,
                           String reserve, float share) {
        label.setColor(color);
        canvas.drawText(title, x, baseline, label);
        final float numbers = x + text(label, title) + gapPart;
        if (text == null || text.isEmpty()) {
            dim.setColor(DIM);
            canvas.drawText("-", numbers, baseline, dim);
        } else {
            value.setColor(Color.WHITE);
            canvas.drawText(text, numbers, baseline, value);
            if (share >= 0f) {
                bar(canvas, numbers, numbers + text(value, text), baseline, color, Math.min(1f, share));
            }
        }
        return numbers + text(value, reserve);
    }

    /** A number's share: a dim track with the filled part in the part's colour, under the number it belongs to. */
    private void bar(Canvas canvas, float left, float right, float baseline, int color, float share) {
        final float top = baseline + 3f * dp;
        plate.setColor(Color.argb(95, Color.red(color), Color.green(color), Color.blue(color)));
        box.set(left, top, right, top + barHeight);
        canvas.drawRoundRect(box, barHeight * 0.5f, barHeight * 0.5f, plate);
        final float filled = (right - left) * share;
        if (filled > 0f) {
            plate.setColor(color);
            box.set(left, top, left + Math.max(filled, barHeight), top + barHeight);
            canvas.drawRoundRect(box, barHeight * 0.5f, barHeight * 0.5f, plate);
        }
    }

    /**
     * The frame times as a graph, with its own caption and scale inside its plate: a line is a projection of numbers,
     * so it says what it plots on the left and where its top is on the right.
     */
    private float drawGraph(Canvas canvas, float x, float top, float w, float h) {
        box.set(x, top, x + w, top + h);
        plate.setColor(Color.argb(89, 0, 0, 0));
        canvas.drawRoundRect(box, 4f * dp, 4f * dp, plate);

        float graphTop = known(values[MS_MAX]) ? values[MS_MAX] : 0f;
        if (graphTop < 50f) {
            graphTop = 50f;
        }
        final float captionBase = top + 7.5f * dp;
        caption.setColor(CAPTION);
        canvas.drawText("frame time, ms", x + 5f * dp, captionBase, caption);
        final String scale = String.format(Locale.US, "0\u2013%.0f ms", graphTop);
        canvas.drawText(scale, x + w - 5f * dp - caption.measureText(scale), captionBase, caption);

        final float plotTop = top + 9.5f * dp, plotBottom = top + h - 2f * dp;
        caption.setColor(Color.argb(40, 255, 255, 255));
        canvas.drawLine(x + 5f * dp, (plotTop + plotBottom) * 0.5f, x + w - 5f * dp, (plotTop + plotBottom) * 0.5f,
                caption);

        final int count = Math.min(120, Math.max(0, (int) values[FRAME_COUNT]));
        final float left = x + 5f * dp, right = x + w - 5f * dp;
        if (count >= 2) {
            final float step = (right - left) / 119f;
            line.reset();
            for (int i = 0; i < count; i++) {
                final float ms = values[FRAMES + i];
                final float px = right - step * (count - 1 - i);
                final float py = plotBottom - Math.min(1f, Math.max(0f, ms / graphTop)) * (plotBottom - plotTop);
                if (i == 0) {
                    line.moveTo(px, py);
                } else {
                    line.lineTo(px, py);
                }
            }
            plot.setStyle(Paint.Style.STROKE);
            plot.setStrokeWidth(1.4f * dp);
            plot.setStrokeJoin(Paint.Join.ROUND);
            plot.setColor(LINE);
            canvas.drawPath(line, plot);
            plot.setStyle(Paint.Style.FILL);
        }
        return x + w;
    }
}
