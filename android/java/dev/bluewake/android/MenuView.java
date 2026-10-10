package dev.bluewake.android;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.text.TextUtils;
import android.text.TextPaint;
import android.view.MotionEvent;
import android.view.View;

import java.util.ArrayList;
import java.util.List;

/**
 * The ⋯ menu: the iPhone and iPad app's UIMenu (apple/ios/src/BWGameOverlay.mm buildMenu) drawn as a panel under
 * the ⋯ button. Rows carry a title, an optional subtitle, a check mark for the current choice, a chevron for a
 * submenu; inline groups sit between separators, as UIMenuOptionsDisplayInline does. Opening a submenu shows its
 * rows with a back row at the top. Choosing an action closes the menu, as on the iPhone.
 */
public class MenuView extends View {
    /** One menu entry: an action, a submenu (children) or an inline group (inline children, no title). */
    static final class Item {
        final String title;
        String subtitle;
        boolean checked, destructive, inline, enabled = true;
        Runnable action;
        List<Item> children;
        Item(String title) { this.title = title; }

        static Item action(String title, Runnable action) {
            Item item = new Item(title);
            item.action = action;
            return item;
        }

        static Item menu(String title, List<Item> children) {
            Item item = new Item(title);
            item.children = children;
            return item;
        }

        static Item inline(List<Item> children) {
            Item item = new Item("");
            item.children = children;
            item.inline = true;
            return item;
        }

        Item subtitle(String s) {
            subtitle = s;
            return this;
        }

        Item checked(boolean on) {
            checked = on;
            return this;
        }

        Item destructive() {
            destructive = true;
            return this;
        }
    }

    interface Listener {
        void onMenuClosed();
    }

    private static final class Row {
        Item item;          // null for the back row
        boolean separatorAbove;
        final RectF rect = new RectF();
    }

    private final float dp;
    private final Paint panelPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint linePaint = new Paint();
    private final Paint pressPaint = new Paint();
    private final TextPaint titlePaint = new TextPaint(Paint.ANTI_ALIAS_FLAG);
    private final TextPaint subtitlePaint = new TextPaint(Paint.ANTI_ALIAS_FLAG);
    private final TextPaint markPaint = new TextPaint(Paint.ANTI_ALIAS_FLAG);
    private final List<Item> stack = new ArrayList<>();
    private final List<Row> rows = new ArrayList<>();
    private final RectF panel = new RectF();
    private float anchorRight, anchorTop, scroll, maxScroll;
    private float downY, downScroll;
    private Row pressedRow;
    private boolean dragging;
    private Listener listener;

    public MenuView(Context context) {
        super(context);
        dp = context.getResources().getDisplayMetrics().density;
        panelPaint.setColor(Color.rgb(30, 30, 32));
        linePaint.setColor(Color.argb(46, 255, 255, 255));
        pressPaint.setColor(Color.argb(40, 255, 255, 255));
        titlePaint.setColor(Color.WHITE);
        titlePaint.setTextSize(16f * dp);
        subtitlePaint.setColor(Color.argb(150, 235, 235, 245));
        subtitlePaint.setTextSize(12.5f * dp);
        markPaint.setColor(Color.WHITE);
        markPaint.setTextSize(16f * dp);
        markPaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
        setVisibility(GONE);
    }

    void setListener(Listener listener) { this.listener = listener; }

    boolean isOpen() { return getVisibility() == VISIBLE; }

    /** Opens the menu with root's children, its top right corner at (right, top). */
    void open(Item root, float right, float top) {
        stack.clear();
        stack.add(root);
        anchorRight = right;
        anchorTop = top;
        setVisibility(VISIBLE);
        rebuild();
    }

    /** Back out of a submenu, or close at the top level. */
    void back() {
        if (stack.size() > 1) {
            stack.remove(stack.size() - 1);
            rebuild();
        } else {
            close();
        }
    }

    void close() {
        if (!isOpen()) return;
        setVisibility(GONE);
        stack.clear();
        if (listener != null) listener.onMenuClosed();
    }

    private void rebuild() {
        rows.clear();
        Item current = stack.get(stack.size() - 1);
        if (stack.size() > 1) rows.add(new Row());  // back row
        boolean afterInline = false;
        for (Item item : current.children) {
            if (item.inline) {
                boolean first = true;
                for (Item child : item.children) {
                    Row row = new Row();
                    row.item = child;
                    row.separatorAbove = first && !rows.isEmpty();
                    first = false;
                    rows.add(row);
                }
                afterInline = true;
            } else {
                Row row = new Row();
                row.item = item;
                row.separatorAbove = afterInline;
                afterInline = false;
                rows.add(row);
            }
        }
        scroll = 0f;
        requestLayout();
        layoutRows();
        invalidate();
    }

    private float rowHeight(Row row) {
        if (row.item != null && row.item.subtitle != null && !row.item.subtitle.isEmpty()) return 62f * dp;
        return 48f * dp;
    }

    private void layoutRows() {
        float width = Math.min(320f * dp, getWidth() - 24f * dp);
        float total = 0f;
        for (Row row : rows) total += rowHeight(row) + (row.separatorAbove ? 7f * dp : 0f);
        float maxHeight = getHeight() - anchorTop - 12f * dp;
        float height = Math.min(total, Math.max(120f * dp, maxHeight));
        float right = Math.min(anchorRight, getWidth() - 12f * dp);
        panel.set(right - width, anchorTop, right, anchorTop + height);
        maxScroll = Math.max(0f, total - height);
        float y = panel.top - scroll;
        for (Row row : rows) {
            if (row.separatorAbove) y += 7f * dp;
            float h = rowHeight(row);
            row.rect.set(panel.left, y, panel.right, y + h);
            y += h;
        }
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        if (isOpen()) layoutRows();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (rows.isEmpty()) return;
        float radius = 14f * dp;
        canvas.drawRoundRect(panel, radius, radius, panelPaint);
        canvas.save();
        canvas.clipRect(panel);
        Item current = stack.get(stack.size() - 1);
        for (int i = 0; i < rows.size(); i++) {
            Row row = rows.get(i);
            if (row.rect.bottom < panel.top || row.rect.top > panel.bottom) continue;
            if (row.separatorAbove) {
                Paint thick = new Paint(linePaint);
                thick.setStrokeWidth(6f * dp);
                thick.setColor(Color.argb(90, 0, 0, 0));
                canvas.drawRect(panel.left, row.rect.top - 7f * dp, panel.right, row.rect.top, thick);
            } else if (i > 0) {
                canvas.drawRect(panel.left + 16f * dp, row.rect.top, panel.right, row.rect.top + Math.max(1f, dp * 0.5f),
                        linePaint);
            }
            if (row == pressedRow) canvas.drawRect(row.rect, pressPaint);
            drawRow(canvas, row, current);
        }
        canvas.restore();
    }

    private void drawRow(Canvas canvas, Row row, Item current) {
        final float left = row.rect.left + 16f * dp, right = row.rect.right - 16f * dp;
        if (row.item == null) {
            // The back row: the submenu's title, with a chevron back.
            titlePaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
            titlePaint.setColor(Color.WHITE);
            canvas.drawText("‹", left, baseline(row.rect.centerY(), titlePaint), titlePaint);
            String title = TextUtils.ellipsize(current.title, titlePaint, right - left - 20f * dp,
                    TextUtils.TruncateAt.END).toString();
            canvas.drawText(title, left + 20f * dp, baseline(row.rect.centerY(), titlePaint), titlePaint);
            titlePaint.setTypeface(Typeface.DEFAULT);
            return;
        }
        Item item = row.item;
        int alpha = item.enabled ? 255 : 110;
        float textLeft = left + 24f * dp;  // room for the check mark, as a UIMenu leaves it
        if (item.checked) {
            markPaint.setAlpha(alpha);
            canvas.drawText("✓", left, baseline(row.rect.centerY(), markPaint), markPaint);
        }
        float textRight = right - (item.children != null ? 18f * dp : 0f);
        int titleColor = item.destructive ? Color.rgb(255, 69, 58) : Color.WHITE;
        titlePaint.setColor(titleColor);
        titlePaint.setAlpha(alpha);
        String title = TextUtils.ellipsize(item.title, titlePaint, textRight - textLeft, TextUtils.TruncateAt.END)
                .toString();
        boolean hasSubtitle = item.subtitle != null && !item.subtitle.isEmpty();
        float titleY = hasSubtitle ? row.rect.top + 26f * dp : baseline(row.rect.centerY(), titlePaint);
        canvas.drawText(title, textLeft, titleY, titlePaint);
        if (hasSubtitle) {
            subtitlePaint.setAlpha(item.enabled ? 150 : 80);
            String sub = TextUtils.ellipsize(item.subtitle, subtitlePaint, textRight - textLeft,
                    TextUtils.TruncateAt.END).toString();
            canvas.drawText(sub, textLeft, row.rect.top + 46f * dp, subtitlePaint);
        }
        if (item.children != null) {
            subtitlePaint.setAlpha(170);
            float size = subtitlePaint.getTextSize();
            subtitlePaint.setTextSize(20f * dp);
            canvas.drawText("›", right - 8f * dp, baseline(row.rect.centerY(), subtitlePaint), subtitlePaint);
            subtitlePaint.setTextSize(size);
        }
    }

    private static float baseline(float centerY, Paint paint) {
        Paint.FontMetrics m = paint.getFontMetrics();
        return centerY - (m.ascent + m.descent) * 0.5f;
    }

    private Row rowAt(float x, float y) {
        if (!panel.contains(x, y)) return null;
        for (Row row : rows) if (row.rect.contains(x, y)) return row;
        return null;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        final float x = event.getX(), y = event.getY();
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                if (!panel.contains(x, y)) {
                    // A tap outside closes the menu, as a UIMenu does.
                    close();
                    return true;
                }
                downY = y;
                downScroll = scroll;
                dragging = false;
                pressedRow = rowAt(x, y);
                invalidate();
                return true;
            case MotionEvent.ACTION_MOVE:
                if (Math.abs(y - downY) > 8f * dp) dragging = true;
                if (dragging && maxScroll > 0f) {
                    scroll = Math.max(0f, Math.min(maxScroll, downScroll + downY - y));
                    pressedRow = null;
                    layoutRows();
                    invalidate();
                }
                return true;
            case MotionEvent.ACTION_UP: {
                Row row = dragging ? null : rowAt(x, y);
                pressedRow = null;
                invalidate();
                if (row != null) choose(row);
                return true;
            }
            case MotionEvent.ACTION_CANCEL:
                pressedRow = null;
                invalidate();
                return true;
            default:
                return true;
        }
    }

    private void choose(Row row) {
        if (row.item == null) {
            back();
            return;
        }
        Item item = row.item;
        if (!item.enabled) return;
        if (item.children != null) {
            stack.add(item);
            rebuild();
            return;
        }
        close();
        if (item.action != null) item.action.run();
    }
}
