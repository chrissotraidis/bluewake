package dev.bluewake.android;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.UUID;

/**
 * Game Data & Saves and the texture pack install, as the iPhone and iPad app does them
 * (apple/ios/src/BWGameOverlay.mm): back up the memory card, restore one, import a quest log from a Dolphin save,
 * explain where the files are, remove the disc image, and copy a texture pack in. The system's document picker
 * stands in for UIDocumentPickerViewController. The card is never changed in place: a replacement is checked, a copy
 * of the current card is kept in Backups, and the new card is staged beside it and renamed over it; then the app
 * closes, because the running game keeps the old card in memory.
 */
final class GameData {
    static final int REQUEST_BACKUP = 4101, REQUEST_RESTORE = 4102, REQUEST_DOLPHIN = 4103, REQUEST_TEXTURES = 4104;
    private static final int MAX_FILE = 64 * 1024 * 1024;

    private final Activity activity;
    private final Overlay overlay;
    private final Settings settings;

    GameData(Activity activity, Overlay overlay, Settings settings) {
        this.activity = activity;
        this.overlay = overlay;
        this.settings = settings;
    }

    File cardFile() {
        String env = System.getenv("BLUEWAKE_CARD_PATH");
        return env != null && !env.isEmpty() ? new File(env) : new File(settings.dataDir, "GZLE01.card");
    }

    private File gameDir() { return new File(settings.dataDir, "game"); }

    private static String stamp(String format) {
        return new SimpleDateFormat(format, Locale.US).format(new Date());
    }

    // ---------------------------------------------------------------- back up

    private String pendingBackupName;

    void backUpSaves() {
        if (!cardFile().isFile()) {
            overlay.showMessage("No Saves Yet", "BlueWake has no memory card yet. Save in the game first.");
            return;
        }
        pendingBackupName = "BlueWake-saves-" + stamp("yyyy-MM-dd") + ".card";
        Intent pick = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        pick.addCategory(Intent.CATEGORY_OPENABLE);
        pick.setType("application/octet-stream");
        pick.putExtra(Intent.EXTRA_TITLE, pendingBackupName);
        overlay.startPicker(pick, REQUEST_BACKUP);
    }

    private void finishBackup(Uri uri) {
        try (InputStream in = new FileInputStream(cardFile());
             OutputStream out = activity.getContentResolver().openOutputStream(uri, "wt")) {
            if (out == null) throw new IOException("The file could not be opened.");
            copy(in, out);
        } catch (IOException e) {
            overlay.showMessage("Could Not Back Up", e.getMessage() != null ? e.getMessage() : "");
            return;
        }
        overlay.showMessage("Saves Backed Up", pendingBackupName + " holds your saves as of your last in-game save. "
                + "Use Restore Saves to bring them back.");
    }

    // ---------------------------------------------------------------- restore

    void chooseSavesToRestore() { overlay.startPicker(openDocument(), REQUEST_RESTORE); }

    private static Intent openDocument() {
        Intent pick = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        pick.addCategory(Intent.CATEGORY_OPENABLE);
        pick.setType("*/*");
        return pick;
    }

    private void confirmRestoreFrom(Uri uri) {
        File copy = copyToCache(uri, "restore.card");
        if (copy == null) return;
        if (!Shell.nativeCardValidate(copy.getAbsolutePath())) {
            overlay.showMessage("Not a Valid BlueWake Save File",
                    "This card is damaged or incomplete. It has been kept unchanged. Choose another backup.");
            return;
        }
        String stamp = stamp("yyyyMMdd-HHmmss");
        String name = displayName(uri);
        overlay.confirm("Replace Your Saves?", "Your current saves will be replaced by " + name + ". A copy of them "
                        + "is kept in BlueWake's Backups folder as GZLE01-" + stamp + ".card.\n\nDo this at the title "
                        + "screen or right after saving. BlueWake then closes so the restored saves load when you "
                        + "open it again.", "Cancel", "Replace Saves", true,
                () -> {
                    final boolean replaced = replaceCardWith(copy, stamp);
                    //noinspection ResultOfMethodCallIgnored
                    copy.delete();
                    if (!replaced) return;
                    Shell.log("[shell] saves restored from " + name + "; previous card kept in Backups");
                    closeForSaves("Saves Restored", "BlueWake will close now. Open it again to play with the restored "
                            + "saves.");
                });
    }

    /** Keeps a copy of the current card in Backups, then puts file in its place; says why and returns false if not. */
    private boolean replaceCardWith(File file, String stamp) {
        if (!Shell.nativeCardValidate(file.getAbsolutePath())) {
            overlay.showMessage("Saves Not Changed", "The replacement card is damaged or incomplete. It has been kept "
                    + "unchanged.");
            return false;
        }
        Shell.nativeSuspendCardWrites(true);
        File card = cardFile();
        if (card.isFile()) {
            File backups = new File(settings.dataDir, "Backups");
            //noinspection ResultOfMethodCallIgnored
            backups.mkdirs();
            File backup = new File(backups, "GZLE01-" + stamp + ".card");
            try {
                copy(card, backup);
            } catch (IOException e) {
                overlay.showMessage("Saves Not Changed", "Your current saves could not be backed up, so nothing was "
                        + "replaced. " + (e.getMessage() != null ? e.getMessage() : ""));
                Shell.nativeSuspendCardWrites(false);
                return false;
            }
        }
        // Staged beside the card and renamed over it, so the card is never half written.
        File staged = new File(card.getPath() + ".restore-" + UUID.randomUUID());
        String error = null;
        try {
            copy(file, staged);
        } catch (IOException e) {
            error = e.getMessage();
        }
        if (error != null || !Shell.nativeFlushPath(staged.getAbsolutePath())
                || !Shell.nativeCardValidate(staged.getAbsolutePath()) || !staged.renameTo(card)) {
            Shell.log("[card-restore] replacement retained at " + staged.getAbsolutePath());
            overlay.showMessage("Saves Not Changed", "The save file could not be put in place. "
                    + (error != null ? error : ""));
            Shell.nativeSuspendCardWrites(false);
            return false;
        }
        return true;
    }

    /** The running game keeps the old card in memory and writes all of it on its next save, so the app closes. */
    private void closeForSaves(String title, String message) {
        overlay.alert(title, message, null, "Close BlueWake", false, overlay::closeApp);
    }

    // ---------------------------------------------------------------- Dolphin saves

    void chooseDolphinSave() { overlay.startPicker(openDocument(), REQUEST_DOLPHIN); }

    /** One quest log, "Link · 6¾ hearts · 180 rupees". */
    private static final class QuestLog {
        boolean empty, checksumOk;
        String summary;

        static QuestLog parse(String row) {
            String[] f = row.split("\t", 5);
            QuestLog log = new QuestLog();
            log.empty = "1".equals(f[0]);
            log.checksumOk = "1".equals(f[1]);
            int maxLife = Integer.parseInt(f[2]), rupees = Integer.parseInt(f[3]);
            String name = f.length > 4 && !f[4].isEmpty() ? f[4] : "No name";
            String[] quarters = {"", "¼", "½", "¾"};
            String hearts = (maxLife / 4) + quarters[maxLife % 4] + " heart" + (maxLife == 4 ? "" : "s");
            log.summary = name + " · " + hearts + " · " + rupees + " rupee" + (rupees == 1 ? "" : "s");
            return log;
        }
    }

    private void readDolphinSave(Uri uri) {
        byte[] file = readAll(uri);
        if (file == null) {
            overlay.showMessage("Could Not Import", "The file could not be read.");
            return;
        }
        byte[] card = readFile(cardFile());
        if (card == null) {
            overlay.showMessage("No Memory Card Yet", "BlueWake makes its memory card when the game starts. Open the "
                    + "game once, then try again.");
            return;
        }
        String[] error = new String[1];
        String[] theirs = Shell.nativeQuestLogs(file, true, error);
        String[] here = theirs != null ? Shell.nativeQuestLogs(card, false, error) : null;
        if (theirs == null || here == null) {
            overlay.showMessage("Could Not Import", error[0] != null ? error[0] : "");
            return;
        }
        String name = displayName(uri);
        List<QuestLog> logs = new ArrayList<>();
        for (String row : theirs) logs.add(QuestLog.parse(row));
        List<String> titles = new ArrayList<>();
        List<Boolean> enabled = new ArrayList<>();
        List<Integer> slots = new ArrayList<>();
        int usable = 0;
        for (int i = 0; i < logs.size(); i++) {
            QuestLog log = logs.get(i);
            if (log.empty) continue;
            titles.add(log.summary);
            enabled.add(log.checksumOk);
            slots.add(i + 1);
            if (log.checksumOk) usable++;
        }
        if (usable == 0) {
            overlay.showMessage("Nothing to Import", "This Dolphin save has no quest logs BlueWake can read.");
            return;
        }
        final boolean hasSaves = here.length > 0;
        overlay.sheet("Import Which Quest Log?", name, titles, enabled, choice -> {
            int source = slots.get(choice);
            String summary = titles.get(choice);
            if (!hasSaves) {
                // Nothing on BlueWake's card yet, so the whole Dolphin file goes in.
                confirmImport(file, name, 0, null, 0, null);
            } else {
                chooseDestination(file, name, source, summary, here);
            }
        });
    }

    private void chooseDestination(byte[] file, String name, int source, String summary, String[] here) {
        List<String> titles = new ArrayList<>();
        List<Boolean> enabled = new ArrayList<>();
        List<String> there = new ArrayList<>();
        for (int i = 0; i < here.length; i++) {
            QuestLog log = QuestLog.parse(here[i]);
            String current = log.empty ? null : log.summary;
            there.add(current);
            titles.add("Quest Log " + (i + 1) + ": " + (current != null ? current : "Empty"));
            enabled.add(true);
        }
        overlay.sheet("Put It in Which Quest Log?", summary + " goes into the quest log you pick and replaces what "
                + "is there now.", titles, enabled,
                choice -> confirmImport(file, name, source, summary, choice + 1, there.get(choice)));
    }

    /** source and destination are 0 when BlueWake has no saves yet and the whole Dolphin file is added. */
    private void confirmImport(byte[] file, String name, int source, String summary, int destination,
                               String replacing) {
        String stamp = stamp("yyyyMMdd-HHmmss");
        String what;
        if (destination == 0)
            what = "BlueWake has no saves yet, so all quest logs in " + name + " are copied in.";
        else if (replacing != null)
            what = "Quest Log " + destination + " (" + replacing + ") will be replaced by " + summary + " from "
                    + name + ".";
        else
            what = summary + " from " + name + " goes into Quest Log " + destination + ".";
        String title = replacing != null ? "Replace Quest Log " + destination + "?" : "Import Save?";
        String go = replacing != null ? "Replace Quest Log " + destination : "Import";
        overlay.confirm(title, what + " A copy of your current saves is kept in BlueWake's Backups folder as GZLE01-"
                        + stamp + ".card.\n\nDo this at the title screen or right after saving. BlueWake then closes "
                        + "so the imported save loads when you open it again.", "Cancel", go, true,
                () -> importSave(file, name, source, destination, stamp));
    }

    private void importSave(byte[] file, String name, int source, int destination, String stamp) {
        // The card is read again here in case the game saved while the menus were up.
        byte[] card = readFile(cardFile());
        String[] error = new String[1];
        byte[] result = card != null ? Shell.nativeCardImport(card, file, source, destination, error) : null;
        if (result == null) {
            overlay.showMessage("Could Not Import", error[0] != null ? error[0] : "The memory card could not be read.");
            return;
        }
        // The new card waits in the app's cache; replaceCardWith copies it into place.
        File staged = new File(activity.getCacheDir(), "import.card");
        try (OutputStream out = new FileOutputStream(staged)) {
            out.write(result);
        } catch (IOException e) {
            //noinspection ResultOfMethodCallIgnored
            staged.delete();
            overlay.showMessage("Saves Not Changed", e.getMessage() != null ? e.getMessage() : "");
            return;
        }
        final boolean replaced = replaceCardWith(staged, stamp);
        //noinspection ResultOfMethodCallIgnored
        staged.delete();
        if (!replaced) return;
        Shell.log("[shell] Dolphin save imported from " + name + " (quest log " + source + " into " + destination
                + "); previous card kept in Backups");
        closeForSaves("Save Imported", "BlueWake will close now. Open it again and pick the quest log to play.");
    }

    // ---------------------------------------------------------------- files

    void explainFiles() {
        overlay.showMessage("Your Files", "BlueWake keeps its files in Android/data/" + activity.getPackageName()
                + "/files on this device (reach it with a USB cable from a computer, or adb).\n\n"
                + "Your saves are GZLE01.card. Back Up Saves makes a copy you can keep anywhere, and Restore Saves "
                + "brings one back; earlier cards are kept in Backups. Import Dolphin Save brings in a quest log from "
                + "a Dolphin .gci or memory card file. The game folder holds your disc image (GZLE01.iso) and "
                + "main.dol and rels, made from it. Texture packs live in Load, and the logs folder has the session "
                + "logs.");
    }

    void confirmDataRemoval() {
        overlay.confirm("Remove Disc Image?", "This deletes your disc image (GZLE01.iso) and the game files made from "
                + "it (main.dol and rels). Your saves, mods and settings are kept.\n\nDo this at the title screen or "
                + "right after saving. BlueWake then closes; push the game files to the phone again "
                + "(scripts/android/install.py) to play.", "Cancel", "Remove Disc Image", true, () -> {
            File game = gameDir();
            //noinspection ResultOfMethodCallIgnored
            new File(game, "GZLE01.iso").delete();
            //noinspection ResultOfMethodCallIgnored
            new File(game, "main.dol").delete();
            deleteTree(new File(game, "rels"));
            Shell.log("[shell] game data removed; saves kept");
            // The game reads from these files as it plays, so it cannot go on.
            overlay.alert("Disc Image Removed", "Your saves, mods and settings are still here. BlueWake will close "
                    + "now.", null, "Close BlueWake", false, overlay::closeApp);
        });
    }

    // ---------------------------------------------------------------- results

    boolean onActivityResult(int request, int result, Intent data) {
        if (request != REQUEST_BACKUP && request != REQUEST_RESTORE && request != REQUEST_DOLPHIN
                && request != REQUEST_TEXTURES)
            return false;
        Uri uri = result == Activity.RESULT_OK && data != null ? data.getData() : null;
        if (uri == null) return true;
        if (request == REQUEST_BACKUP) finishBackup(uri);
        else if (request == REQUEST_RESTORE) confirmRestoreFrom(uri);
        else if (request == REQUEST_DOLPHIN) readDolphinSave(uri);
        else installTexturePackFrom(uri);
        return true;
    }

    // ---------------------------------------------------------------- texture packs

    void chooseTexturePack() { overlay.startPicker(new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE), REQUEST_TEXTURES); }

    /** Copies every PNG and DDS texture under the picked folder into Load/Textures/GZLE01, keeping its subfolders
     *  (the renderer searches them). */
    private void installTexturePackFrom(Uri tree) {
        final File dest = settings.texturePackDirectory();
        final Overlay.Wait wait = overlay.wait("Installing Texture Pack", "Copying textures…");
        new Thread(() -> {
            final int[] counts = new int[2];  // installed, failed
            try {
                copyTextures(tree, DocumentsContract.getTreeDocumentId(tree), dest, counts, wait);
            } catch (RuntimeException e) {
                counts[1]++;
            }
            wait.dismiss();
            overlay.post(() -> finishTextureInstall(counts[0], counts[1]));
        }, "TexturePackInstall").start();
    }

    private void copyTextures(Uri tree, String folder, File dest, int[] counts, Overlay.Wait wait) {
        Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, folder);
        String[] columns = {DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                DocumentsContract.Document.COLUMN_DISPLAY_NAME, DocumentsContract.Document.COLUMN_MIME_TYPE};
        try (Cursor c = activity.getContentResolver().query(children, columns, null, null, null)) {
            if (c == null) return;
            while (c.moveToNext()) {
                String id = c.getString(0), name = c.getString(1), mime = c.getString(2);
                if (id == null || name == null || name.isEmpty() || name.contains("/") || name.equals(".")
                        || name.equals(".."))
                    continue;
                if (DocumentsContract.Document.MIME_TYPE_DIR.equals(mime)) {
                    copyTextures(tree, id, new File(dest, name), counts, wait);
                    continue;
                }
                String lower = name.toLowerCase(Locale.US);
                if (!lower.endsWith(".png") && !lower.endsWith(".dds")) continue;
                //noinspection ResultOfMethodCallIgnored
                dest.mkdirs();
                Uri document = DocumentsContract.buildDocumentUriUsingTree(tree, id);
                try (InputStream in = activity.getContentResolver().openInputStream(document);
                     OutputStream out = new FileOutputStream(new File(dest, name))) {
                    if (in == null) throw new IOException("unreadable");
                    copy(in, out);
                    counts[0]++;
                } catch (IOException | RuntimeException e) {
                    counts[1]++;
                }
                if (counts[0] > 0 && counts[0] % 250 == 0) wait.setMessage(counts[0] + " textures copied…");
            }
        }
    }

    private void finishTextureInstall(int installed, int failed) {
        Shell.log("[mods] texture pack install: " + installed + " copied, " + failed + " failed");
        overlay.texturePackChanged();
        if (installed == 0) {
            overlay.showMessage("No Textures Found", "That folder has no PNG or DDS textures. Pick the pack's folder "
                    + "of tex1_… images for The Wind Waker (USA), often named GZLE01. If the pack is a .zip, unzip "
                    + "it first (your Files app can).");
            return;
        }
        overlay.offerToTurnOn(Settings.MOD_HD_TEXTURES, "Texture Pack Installed", installed + " textures were "
                + "installed." + (failed > 0 ? " " + failed + " could not be copied." : "") + " HD Texture Pack uses "
                + "them after BlueWake restarts.");
    }

    // ---------------------------------------------------------------- helpers

    String displayName(Uri uri) {
        String name = null;
        try (android.database.Cursor c = activity.getContentResolver().query(uri,
                new String[] {android.provider.OpenableColumns.DISPLAY_NAME}, null, null, null)) {
            if (c != null && c.moveToFirst()) name = c.getString(0);
        } catch (RuntimeException ignored) {
        }
        return name != null ? name : uri.getLastPathSegment();
    }

    private File copyToCache(Uri uri, String name) {
        File file = new File(activity.getCacheDir(), name);
        try (InputStream in = activity.getContentResolver().openInputStream(uri);
             OutputStream out = new FileOutputStream(file)) {
            if (in == null) throw new IOException("The file could not be opened.");
            copy(in, out);
            return file;
        } catch (IOException e) {
            overlay.showMessage("Could Not Read the File", e.getMessage() != null ? e.getMessage() : "");
            return null;
        }
    }

    private byte[] readAll(Uri uri) {
        try (InputStream in = activity.getContentResolver().openInputStream(uri)) {
            return in != null ? readStream(in) : null;
        } catch (IOException e) {
            return null;
        }
    }

    static byte[] readFile(File file) {
        if (!file.isFile()) return null;
        try (InputStream in = new FileInputStream(file)) {
            return readStream(in);
        } catch (IOException e) {
            return null;
        }
    }

    private static byte[] readStream(InputStream in) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buffer = new byte[65536];
        for (int n; (n = in.read(buffer)) > 0; ) {
            out.write(buffer, 0, n);
            if (out.size() > MAX_FILE) throw new IOException("The file is too large.");
        }
        return out.toByteArray();
    }

    static void copy(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[65536];
        for (int n; (n = in.read(buffer)) > 0; ) out.write(buffer, 0, n);
    }

    static void copy(File from, File to) throws IOException {
        try (InputStream in = new FileInputStream(from); OutputStream out = new FileOutputStream(to)) {
            copy(in, out);
        }
    }

    private static void deleteTree(File file) {
        File[] children = file.listFiles();
        if (children != null) for (File child : children) deleteTree(child);
        //noinspection ResultOfMethodCallIgnored
        file.delete();
    }
}
