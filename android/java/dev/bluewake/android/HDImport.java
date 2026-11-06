package dev.bluewake.android;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.CancellationSignal;
import android.os.Handler;
import android.os.Looper;
import android.os.OperationCanceledException;

import java.lang.ref.WeakReference;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

/** The same local HD import as iOS. Only the completed pack path enters settings. */
final class HDImport {
    private static final int FOLDER = 4201, IMAGE = 4202, DISC_KEY = 4203, CONTENT_KEY = 4204;
    private final Activity activity;
    private final Overlay overlay;
    private final Settings settings;
    private Uri source, discKey;
    private boolean image, titleKey, resumed, destroyed;
    private Runnable pendingUI;
    private Job job;
    private Overlay.Wait wait;

    HDImport(Activity activity, Overlay overlay, Settings settings) {
        this.activity = activity;
        this.overlay = overlay;
        this.settings = settings;
    }

    void choose() {
        if (job != null || destroyed) return;
        clearSelection();
        overlay.sheet("Import from Wind Waker HD", "Choose your decrypted dump folder, or a USA WUD/WUX image "
                + "and its key files.", Arrays.asList("Decrypted dump folder…", "WUD/WUX disc image…"),
                Arrays.asList(true, true), choice -> {
                    image = choice == 1;
                    pick(image ? IMAGE : FOLDER);
                });
    }

    private void pick(int request) {
        Intent intent = new Intent(request == FOLDER ? Intent.ACTION_OPEN_DOCUMENT_TREE : Intent.ACTION_OPEN_DOCUMENT);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        if (request != FOLDER) {
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
        }
        overlay.startPicker(intent, request);
    }

    boolean onActivityResult(int request, int result, Intent data) {
        if (request < FOLDER || request > CONTENT_KEY) return false;
        Uri uri = result == Activity.RESULT_OK && data != null ? data.getData() : null;
        if (uri == null) {
            clearSelection();
            return true;
        }
        if (request == FOLDER) {
            source = uri;
            whenResumed(() -> start(null));
        } else if (request == IMAGE) {
            source = uri;
            whenResumed(() -> overlay.alert("Disc Key", "Choose the disc key file for this Wind Waker HD image.",
                    "Cancel", "Choose File…", false, () -> pick(DISC_KEY)));
        } else if (request == DISC_KEY && source != null) {
            discKey = uri;
            whenResumed(() -> overlay.sheet("Content Key", "Choose the key file to read this disc's content.",
                    Arrays.asList("Common key file…", "Decrypted title key file…"), Arrays.asList(true, true),
                    choice -> { titleKey = choice == 1; pick(CONTENT_KEY); }));
        } else if (request == CONTENT_KEY && source != null && discKey != null) {
            whenResumed(() -> start(uri));
        } else {
            clearSelection();
        }
        return true;
    }

    private void clearSelection() {
        source = discKey = null;
        image = titleKey = false;
        pendingUI = null;
    }

    private void whenResumed(Runnable action) {
        pendingUI = action;
        drainUI();
    }

    private void drainUI() {
        if (!resumed || destroyed || activity.isFinishing() || activity.isDestroyed()) return;
        Runnable action = pendingUI;
        pendingUI = null;
        if (action != null) action.run();
        if (job != null && job.done) finish();
    }

    void onResume() { resumed = true; drainUI(); }
    void onPause() { resumed = false; }
    void onStop() { if (job != null) cancel(); }

    void onDestroy() {
        destroyed = true;
        clearSelection();
        if (job != null) job.cancel();
        if (wait != null) wait.dismiss();
        wait = null;
    }

    private void start(Uri contentKey) {
        if (job != null || source == null || destroyed) return;
        final long id;
        try {
            id = HDImportNative.create();
        } catch (RuntimeException failure) {
            clearSelection();
            overlay.showMessage("Could Not Start Import", "BlueWake could not reserve an import. Please try again.");
            return;
        }
        if (id == 0) {
            clearSelection();
            overlay.showMessage("Import Still Finishing", "The previous import is still closing its files. Try again shortly.");
            return;
        }
        Job next = new Job(activity.getApplicationContext(), this, id, source, image, discKey, contentKey, titleKey);
        job = next;
        clearSelection();
        try {
            wait = overlay.wait("Importing from Wind Waker HD", "Opening files…", this::cancel);
            new Thread(next, "WindWakerHDImport").start();
        } catch (RuntimeException | OutOfMemoryError failure) {
            HDImportNative.release(id);
            job = null;
            if (wait != null) wait.dismiss();
            wait = null;
            overlay.showMessage("Could Not Start Import", "Please return to BlueWake and try again.");
        }
    }

    private void cancel() {
        if (job == null || job.done) return;
        job.cancel();
        if (wait != null) wait.cancelling();
    }

    private void progress(Job owner, String message) {
        if (!destroyed && job == owner && !owner.done && !owner.cancel.isCanceled() && wait != null)
            wait.setMessage(message);
    }

    private void finish() {
        Job completed = job;
        if (completed == null || !completed.done) return;
        job = null;
        if (wait != null) wait.dismiss();
        wait = null;
        overlay.texturePackChanged();
        String[] result = completed.result;
        if ("success".equals(result[0])) {
            overlay.offerToTurnOn(Settings.MOD_HD_TEXTURES, "Wind Waker HD Textures Ready", result[2]
                    + " textures were made from your HD copy. Restart BlueWake to load the selected pack."
                    + (completed.cleanupFailed ? " Temporary input files could not all be removed." : ""));
        } else if ("cancelled".equals(result[0])) {
            overlay.showMessage("Import Cancelled", "Your existing texture packs are unchanged."
                    + (completed.cleanupFailed ? " Temporary input files could not all be removed." : ""));
        } else {
            overlay.showMessage("Import Failed", result[3]
                    + (completed.cleanupFailed ? "\nTemporary input files could not all be removed." : ""));
        }
    }

    private static byte[] utf8(String text) {
        return text == null ? new byte[0] : text.getBytes(StandardCharsets.UTF_8);
    }

    /** Holds no Activity: recreation requests cancellation while the worker owns all input grants/descriptors. */
    private static final class Job implements Runnable {
        final Context context;
        final WeakReference<HDImport> owner;
        final Handler main = new Handler(Looper.getMainLooper());
        final long id;
        final Uri source, discKey, contentKey;
        final boolean image, titleKey;
        final CancellationSignal cancel = new CancellationSignal();
        volatile boolean done;
        String[] result;
        boolean cleanupFailed;

        Job(Context context, HDImport owner, long id, Uri source, boolean image, Uri discKey,
            Uri contentKey, boolean titleKey) {
            this.context = context;
            this.owner = new WeakReference<>(owner);
            this.id = id;
            this.source = source;
            this.image = image;
            this.discKey = discKey;
            this.contentKey = contentKey;
            this.titleKey = titleKey;
        }

        void cancel() {
            HDImportNative.cancel(id);
            cancel.cancel();
        }

        void progress(String text) {
            main.post(() -> {
                HDImport current = owner.get();
                if (current != null) current.progress(this, text);
            });
        }

        private void importPack() {
            HDImportInputs inputs = null;
            try {
                inputs = HDImportInputs.prepare(context, source, image, discKey, contentKey, titleKey, cancel, this::progress);
                cancel.throwIfCanceled();
                Settings settings = new Settings(context);
                result = HDImportNative.run(id, utf8(inputs.sourcePath), utf8(inputs.discKeyPath),
                        utf8(inputs.contentKeyPath), titleKey,
                        utf8(new java.io.File(settings.dataDir, "Load/Textures").getAbsolutePath()),
                        (read, total, count, stage) -> progress(total > 0
                                ? read + " of " + total + " files, " + count + " textures so far" : stage));
            } catch (OperationCanceledException ex) {
                result = new String[] {"cancelled", "", "0", ""};
                cleanupFailed = ex.getSuppressed().length != 0;
            } catch (OutOfMemoryError ex) {
                result = new String[] {"failed", "", "0", "There was not enough memory to finish the import."};
                cleanupFailed = ex.getSuppressed().length != 0;
            } catch (Exception ex) {
                result = new String[] {cancel.isCanceled() ? "cancelled" : "failed", "", "0",
                        ex.getMessage() != null ? ex.getMessage() : "The selected files could not be read."};
                cleanupFailed = ex.getSuppressed().length != 0;
            } finally {
                if (inputs != null) {
                    try { inputs.close(); } catch (Exception ex) { cleanupFailed = true; }
                }
            }
        }

        @Override public void run() {
            try {
                importPack();
                if (result == null || result.length != 4)
                    result = new String[] {"failed", "", "0", "The import did not return a result."};
                if ("success".equals(result[0])) {
                    // Keep the completed selection even if the old activity was destroyed.
                    // The reservation covers this write as well as the native work.
                    if (!new Settings(context).selectTexturePack(new java.io.File(result[1])))
                        result = new String[] {"failed", "", "0", "The pack was created, but BlueWake could not save its selection."};
                }
            } catch (RuntimeException | OutOfMemoryError failure) {
                result = new String[] {"failed", "", "0", "BlueWake could not finish selecting the imported pack."};
            } finally {
                try {
                    HDImportNative.release(id);
                } catch (RuntimeException failure) {
                    result = new String[] {"failed", "", "0", "BlueWake could not close the import session."};
                }
                done = true;
                main.post(() -> { HDImport current = owner.get(); if (current != null) current.drainUI(); });
            }
        }
    }
}
