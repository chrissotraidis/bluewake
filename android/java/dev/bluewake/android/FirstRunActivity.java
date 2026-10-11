package dev.bluewake.android;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * The launcher activity: runs before SDL and the game, so a missing game
 * folder can be put right without the game's own lifecycle getting in the way.
 * If main.dol and the 415 rels are present it forwards straight to the game
 * (BlueWakeActivity). Otherwise it shows the first-run screen, the Android
 * counterpart of the iPhone and iPad app's (apple/ios/src/first_run.m): the
 * player picks their own GZLE01 disc image, it is copied into the game folder,
 * and main.dol + rels/ are extracted from it on the device (disc_import.c,
 * through Shell.nativeDiscPrepare). The translated game code is already in the
 * app, so once the files exist the game starts and this screen never returns.
 */
public final class FirstRunActivity extends Activity {
    static final int REQUEST_DISC = 0x4B57;  // "BW"

    private File dataDir;
    private File gameDir;
    private File isoPath;
    private TextView status;
    private Button choose;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        File external = getExternalFilesDir(null);
        dataDir = external != null ? external : getFilesDir();
        gameDir = new File(dataDir, "game");
        isoPath = new File(gameDir, "GZLE01.iso");

        if (ready()) {
            forward();
            return;
        }

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setGravity(Gravity.CENTER);
        layout.setPadding(48, 48, 48, 48);
        layout.setBackgroundColor(Color.BLACK);

        TextView title = new TextView(this);
        title.setText("BlueWake");
        title.setTextColor(Color.WHITE);
        title.setTextSize(28);
        title.setGravity(Gravity.CENTER);
        layout.addView(title, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView subtitle = new TextView(this);
        subtitle.setText("To play, BlueWake needs your own disc image of\nThe Legend of Zelda: The Wind Waker (GameCube USA).\nChoose it once and the game files are made on this device.");
        subtitle.setTextColor(Color.LTGRAY);
        subtitle.setTextSize(15);
        subtitle.setGravity(Gravity.CENTER);
        LinearLayout.LayoutParams sub = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        sub.setMargins(0, 24, 0, 0);
        layout.addView(subtitle, sub);

        status = new TextView(this);
        status.setTextColor(Color.WHITE);
        status.setTextSize(14);
        status.setGravity(Gravity.CENTER);
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        sp.setMargins(0, 24, 0, 0);
        layout.addView(status, sp);

        choose = new Button(this);
        choose.setText("Choose Disc Image");
        LinearLayout.LayoutParams bp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        bp.setMargins(0, 24, 0, 0);
        bp.gravity = Gravity.CENTER_HORIZONTAL;
        layout.addView(choose, bp);
        choose.setOnClickListener(v -> pickDisc());

        setContentView(layout);
    }

    private boolean ready() {
        File main = new File(gameDir, "main.dol");
        File rels = new File(gameDir, "rels");
        String[] files = rels.list();
        return main.isFile() && rels.isDirectory() && files != null && files.length == 415;
    }

    private void forward() {
        Intent intent = new Intent(this, BlueWakeActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(intent);
        finish();
    }

    private void pickDisc() {
        Intent pick = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        pick.addCategory(Intent.CATEGORY_OPENABLE);
        pick.setType("*/*");
        try {
            startActivityForResult(pick, REQUEST_DISC);
        } catch (Exception e) {
            setStatus("No file picker on this device.");
        }
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != REQUEST_DISC || result != Activity.RESULT_OK || data == null)
            return;
        Uri uri = data.getData();
        if (uri == null)
            return;
        importDisc(uri);
    }

    private void setStatus(String text) {
        if (status != null)
            status.setText(text);
    }

    private void importDisc(final Uri uri) {
        if (choose != null)
            choose.setEnabled(false);
        setStatus("Copying the disc image…");
        new Thread(() -> {
            String error = null;
            try {
                if (!gameDir.isDirectory() && !gameDir.mkdirs())
                    throw new IOException("Could not create the game folder.");
                try (InputStream in = getContentResolver().openInputStream(uri);
                     OutputStream out = new FileOutputStream(isoPath)) {
                    if (in == null)
                        throw new IOException("The disc image could not be opened.");
                    copy(in, out);
                }
                // Load libmain.so (SDL is inside it) so the extractor is there;
                // JNI_OnLoad only registers methods, it does not start SDL.
                System.loadLibrary("main");
                error = Shell.nativeDiscPrepare(isoPath.getAbsolutePath(), gameDir.getAbsolutePath());
            } catch (IOException e) {
                error = e.getMessage() != null ? e.getMessage() : "The disc image could not be copied.";
            } catch (UnsatisfiedLinkError e) {
                error = "The native library could not be loaded.";
            }
            final String result = error;
            runOnUiThread(() -> {
                if (result == null) {
                    setStatus("Done.");
                    forward();
                } else {
                    setStatus(result);
                    if (choose != null)
                        choose.setEnabled(true);
                }
            });
        }).start();
    }

    private static void copy(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[65536];
        for (int n; (n = in.read(buffer)) > 0; )
            out.write(buffer, 0, n);
    }
}
