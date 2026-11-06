package dev.bluewake.android;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import android.provider.OpenableColumns;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.system.StructPollfd;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.text.Normalizer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

/** Worker-only SAF inputs. Keep this owner alive through the entire native call;
 * cancel the worker, then close here, never close its descriptors from the UI.
 * No persistent URI grants, provider writes, key logging or legacy-cache cleanup.
 */
public final class HDImportInputs implements AutoCloseable {
    public interface Progress { void onProgress(String message); }

    public final String sourcePath, discKeyPath, contentKeyPath;
    private static final long MAX_RESOURCE = 256L * 1024 * 1024; // wwhd::kMaxFile
    private static final long MAX_STAGED = 32L * 1024 * 1024 * 1024;
    private static final int MAX_ENTRIES = 100000, MAX_DEPTH = 32, CHUNK = 65536;
    private final ContentResolver resolver;
    private final CancellationSignal cancel;
    private final Progress progress;
    private final Set<String> documents = new HashSet<>();
    private final Path staging;
    private final HDImportWorkspace workspace;
    private ParcelFileDescriptor imageDescriptor;
    private long copied, reported, nameBytes;
    private int entries, resources;

    public static HDImportInputs prepare(Context context, Uri source, boolean image,
            Uri discKey, Uri contentKey, boolean titleKey, CancellationSignal cancel,
            Progress progress) throws IOException {
        return new HDImportInputs(context, source, image, discKey, contentKey, titleKey, cancel, progress);
    }

    private HDImportInputs(Context context, Uri source, boolean image, Uri discKey,
            Uri contentKey, boolean titleKey, CancellationSignal signal, Progress callback) throws IOException {
        resolver = context.getContentResolver();
        cancel = signal == null ? new CancellationSignal() : signal;
        progress = callback;
        cancel.throwIfCanceled();
        requireContent(source);
        if (image) {
            requireContent(discKey);
            requireContent(contentKey);
        } else if (discKey != null || contentKey != null) {
            throw new IOException("Folder imports do not use key files.");
        }
        workspace = HDImportWorkspace.create(context);
        staging = workspace.path;
        try {
            sourcePath = image ? prepareImage(source) : prepareFolder(source);
            discKeyPath = image ? prepareKey(discKey, "disc.key") : "";
            contentKeyPath = image ? prepareKey(contentKey, titleKey ? "title.key" : "common.key") : "";
            cancel.throwIfCanceled();
        } catch (IOException | RuntimeException | Error failure) {
            try { close(); } catch (IOException cleanup) { failure.addSuppressed(cleanup); }
            throw failure;
        }
    }

    private static void requireContent(Uri uri) throws IOException {
        if (uri == null || !ContentResolver.SCHEME_CONTENT.equals(uri.getScheme()))
            throw new IOException("Choose an input from the document picker.");
    }

    private Cursor query(Uri uri, String[] columns) throws IOException {
        cancel.throwIfCanceled();
        Cursor cursor = resolver.query(uri, columns, null, null, null, cancel);
        if (cursor == null) throw new IOException("The document provider returned no listing.");
        return cursor;
    }

    private void completeListing(Cursor cursor) throws IOException {
        cancel.throwIfCanceled();
        if (cursor.getExtras().getBoolean(DocumentsContract.EXTRA_LOADING, false) ||
                cursor.getExtras().containsKey(DocumentsContract.EXTRA_ERROR))
            throw new IOException("The document listing is incomplete. Make the files available and try again.");
    }

    private static long size(Cursor cursor) throws IOException {
        int column = cursor.getColumnIndex(OpenableColumns.SIZE);
        if (column < 0 || cursor.isNull(column)) return -1;
        long size = cursor.getLong(column);
        if (size < 0) throw new IOException("The provider returned an invalid file size.");
        return size;
    }

    private static void safeName(String name) throws IOException {
        if (name == null || name.isEmpty() || name.equals(".") || name.equals("..") ||
                name.indexOf('/') >= 0 || name.indexOf('\\') >= 0 ||
                name.getBytes(StandardCharsets.UTF_8).length > 255 ||
                !name.equals(new String(name.getBytes(StandardCharsets.UTF_8), StandardCharsets.UTF_8)))
            throw new IOException("The provider returned an unsafe file name.");
        for (int i = 0; i < name.length(); ++i)
            if (Character.isISOControl(name.charAt(i)))
                throw new IOException("The provider returned an unsafe file name.");
    }

    private static final class Entry {
        final String id, name;
        final boolean directory;
        final long size;
        Entry(Cursor cursor) throws IOException {
            id = cursor.getString(cursor.getColumnIndexOrThrow(Document.COLUMN_DOCUMENT_ID));
            name = cursor.getString(cursor.getColumnIndexOrThrow(Document.COLUMN_DISPLAY_NAME));
            safeName(name);
            if (id == null || id.isEmpty() || id.length() > 4096)
                throw new IOException("The provider returned an invalid document ID.");
            String type = cursor.getString(cursor.getColumnIndexOrThrow(Document.COLUMN_MIME_TYPE));
            if (type == null) throw new IOException("The provider omitted the document type.");
            directory = Document.MIME_TYPE_DIR.equals(type);
            size = directory ? -1 : HDImportInputs.size(cursor);
        }
    }
    private static final String[] TREE_COLUMNS = {Document.COLUMN_DOCUMENT_ID,
        Document.COLUMN_DISPLAY_NAME, Document.COLUMN_MIME_TYPE, Document.COLUMN_SIZE};

    private List<Entry> children(Uri tree, String parent) throws IOException {
        List<Entry> result = new ArrayList<>();
        Set<String> names = new HashSet<>();
        try (Cursor cursor = query(DocumentsContract.buildChildDocumentsUriUsingTree(tree, parent), TREE_COLUMNS)) {
            completeListing(cursor);
            while (cursor.moveToNext()) {
                cancel.throwIfCanceled();
                if (++entries > MAX_ENTRIES) throw new IOException("The selected folder has too many entries.");
                Entry entry = new Entry(cursor);
                nameBytes += entry.id.length() + entry.name.length();
                if (nameBytes > 16 * 1024 * 1024)
                    throw new IOException("The selected folder listing is too large.");
                String logicalName = Normalizer.normalize(entry.name, Normalizer.Form.NFC).toLowerCase(Locale.ROOT);
                if (!names.add(logicalName) || !documents.add(entry.id))
                    throw new IOException("The provider returned duplicate names or a document cycle.");
                result.add(entry);
            }
            completeListing(cursor);
        }
        return result;
    }

    private String prepareFolder(Uri tree) throws IOException {
        if (!DocumentsContract.isTreeUri(tree)) throw new IOException("Choose a document folder.");
        String root = DocumentsContract.getTreeDocumentId(tree);
        try (Cursor cursor = query(DocumentsContract.buildDocumentUriUsingTree(tree, root), TREE_COLUMNS)) {
            if (!cursor.moveToFirst()) throw new IOException("The selected folder is missing.");
            Entry entry = new Entry(cursor);
            if (!entry.directory || !entry.id.equals(root) || cursor.moveToNext())
                throw new IOException("The selected folder is invalid.");
            completeListing(cursor);
        }
        documents.add(root);
        List<Entry> selected = children(tree, root);
        // Same rule as hd_content_folder: prefer the immediate 'content' folder.
        for (Entry entry : selected) {
            if (entry.directory && entry.name.equals("content")) {
                selected = children(tree, entry.id);
                break;
            }
        }
        Path local = Files.createDirectory(staging.resolve("input"));
        copyTree(tree, selected, local, 0);
        if (resources == 0) throw new IOException("No Wind Waker HD resources were found in the selected folder.");
        return local.toString();
    }

    private void copyTree(Uri tree, List<Entry> files, Path local, int depth) throws IOException {
        if (depth > MAX_DEPTH) throw new IOException("The selected folder is nested too deeply.");
        for (Entry entry : files) {
            cancel.throwIfCanceled();
            Path destination = local.resolve(entry.name);
            if (destination.toString().getBytes(StandardCharsets.UTF_8).length > 4096)
                throw new IOException("The selected folder contains an excessively long path.");
            if (entry.directory) {
                copyTree(tree, children(tree, entry.id), destination, depth + 1);
            } else {
                String name = entry.name.toLowerCase(Locale.ROOT);
                if (!(name.endsWith(".szs") || name.endsWith(".sarc") ||
                        name.endsWith(".pack") || name.endsWith(".bfres"))) continue;
                Files.createDirectories(destination.getParent());
                copyDocument(DocumentsContract.buildDocumentUriUsingTree(tree, entry.id),
                    destination, entry.size, MAX_RESOURCE, true);
                ++resources;
            }
        }
    }

    private String prepareImage(Uri uri) throws IOException {
        long declared;
        String extension;
        try (Cursor cursor = query(uri, new String[]{OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE})) {
            if (!cursor.moveToFirst()) throw new IOException("The selected image is missing.");
            String name = cursor.getString(cursor.getColumnIndexOrThrow(OpenableColumns.DISPLAY_NAME));
            safeName(name);
            name = name.toLowerCase(Locale.ROOT);
            extension = name.endsWith(".wud") ? ".wud" : name.endsWith(".wux") ? ".wux" : "";
            if (extension.isEmpty()) throw new IOException("Choose a WUD or WUX image.");
            declared = size(cursor);
            if (cursor.moveToNext()) throw new IOException("The provider returned multiple images.");
            completeListing(cursor);
        }
        imageDescriptor = resolver.openFileDescriptor(uri, "r", cancel);
        if (imageDescriptor == null) throw new IOException("The selected image could not be opened.");
        cancel.throwIfCanceled();
        long length = seekableSize(imageDescriptor);
        checkSize(declared, length, MAX_STAGED);
        if (length >= 0) {
            if (length == 0) throw new IOException("The selected image is empty.");
            // Keep the suffix expected by import_pack. Never canonicalize this
            // alias or transfer the descriptor; native sniffs WUD/WUX itself.
            Path alias = staging.resolve("disc.wud");
            try {
                Files.createSymbolicLink(alias, Paths.get("/proc/self/fd/" + imageDescriptor.getFd()));
                try (ParcelFileDescriptor reopened = ParcelFileDescriptor.open(alias.toFile(), ParcelFileDescriptor.MODE_READ_ONLY)) {
                    checkSize(length, seekableSize(reopened), MAX_STAGED);
                }
                return alias.toString();
            } catch (IOException | UnsupportedOperationException unavailable) {
                Files.deleteIfExists(alias);
                // Some providers/devices cannot reopen through procfs. Copy
                // from the already-owned read-only descriptor instead.
            }
        }
        Path local = staging.resolve("source" + extension);
        try (ParcelFileDescriptor input = imageDescriptor) {
            imageDescriptor = null;
            copy(input, local, declared, MAX_STAGED, true);
        }
        if (Files.size(local) == 0) throw new IOException("The selected image is empty.");
        return local.toString();
    }

    private String prepareKey(Uri uri, String name) throws IOException {
        long declared;
        try (Cursor cursor = query(uri, new String[]{OpenableColumns.SIZE})) {
            if (!cursor.moveToFirst()) throw new IOException("The selected key file is missing.");
            declared = size(cursor);
            if (cursor.moveToNext()) throw new IOException("The provider returned multiple key files.");
            completeListing(cursor);
        }
        Path local = staging.resolve(name);
        copyDocument(uri, local, declared, 1024, false);
        byte[] key = Files.readAllBytes(local);
        try {
            if (key.length != 16) {
                int digits = 0;
                for (byte value : key) {
                    int c = value & 255;
                    if (c == 32 || (c >= 9 && c <= 13)) continue;
                    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
                        throw new IOException("Key files must contain 16 binary bytes or 32 hex digits.");
                    ++digits;
                }
                if (digits != 32) throw new IOException("Key files must contain 16 binary bytes or 32 hex digits.");
            }
        } finally { Arrays.fill(key, (byte)0); }
        return local.toString();
    }

    // -1 means a pipe/socket, which must be copied before native random access.
    private static long seekableSize(ParcelFileDescriptor input) throws IOException {
        try {
            long size = Os.lseek(input.getFileDescriptor(), 0, OsConstants.SEEK_END);
            if (!OsConstants.S_ISREG(Os.fstat(input.getFileDescriptor()).st_mode) || size < 0 ||
                    Os.lseek(input.getFileDescriptor(), 0, OsConstants.SEEK_SET) != 0)
                throw new IOException("The provider did not supply a regular seekable file.");
            return size;
        } catch (ErrnoException error) {
            if (error.errno == OsConstants.ESPIPE) return -1;
            throw new IOException("The selected file could not be inspected.");
        }
    }

    private static void checkSize(long declared, long actual, long limit) throws IOException {
        if (declared > limit || actual > limit) throw new IOException("The selected input exceeds the import size limit.");
        if (declared >= 0 && actual >= 0 && declared != actual)
            throw new IOException("The selected file is incomplete or changed size.");
    }

    private void copyDocument(Uri uri, Path local, long declared, long limit, boolean notify) throws IOException {
        checkSize(declared, -1, limit);
        cancel.throwIfCanceled();
        try (ParcelFileDescriptor input = resolver.openFileDescriptor(uri, "r", cancel)) {
            if (input == null) throw new IOException("The selected file could not be opened.");
            copy(input, local, declared, limit, notify);
        }
    }

    private void copy(ParcelFileDescriptor input, Path local, long declared, long limit, boolean notify) throws IOException {
        cancel.throwIfCanceled();
        long length = seekableSize(input);
        checkSize(declared, length, limit);
        long expected = length >= 0 ? length : declared;
        byte[] buffer = new byte[CHUNK];
        StructPollfd poll = new StructPollfd();
        poll.fd = input.getFileDescriptor();
        poll.events = (short)OsConstants.POLLIN;
        long count = 0;
        try (OutputStream output = Files.newOutputStream(local, StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE)) {
            if (length < 0) {
                int flags = Os.fcntlInt(poll.fd, OsConstants.F_GETFL, 0);
                Os.fcntlInt(poll.fd, OsConstants.F_SETFL, flags | OsConstants.O_NONBLOCK);
            }
            for (;;) {
                cancel.throwIfCanceled();
                int n;
                try {
                    if (Os.poll(new StructPollfd[]{poll}, 100) == 0) continue;
                    cancel.throwIfCanceled();
                    n = Os.read(poll.fd, buffer, 0, buffer.length);
                } catch (ErrnoException error) {
                    if (error.errno == OsConstants.EINTR || error.errno == OsConstants.EAGAIN) continue;
                    throw error;
                }
                if (n == 0) break;
                if (n > limit - count || n > MAX_STAGED - copied || (expected >= 0 && n > expected - count))
                    throw new IOException("The selected file exceeds its size or the import size limit.");
                output.write(buffer, 0, n);
                count += n;
                copied += n;
                if (notify && progress != null && copied - reported >= 1024 * 1024) {
                    reported = copied;
                    reportProgress();
                }
            }
            cancel.throwIfCanceled();
            input.checkError(); // A reliable pipe's EOF may actually be a provider failure.
            if (expected >= 0 && count != expected) throw new IOException("The selected file is incomplete.");
            output.flush();
        } catch (ErrnoException error) {
            throw new IOException("The selected file could not be copied.");
        } finally { Arrays.fill(buffer, (byte)0); }
        // Both the output close above and provider close by the caller can fail.
        if (notify && progress != null) { reported = copied; reportProgress(); }
        cancel.throwIfCanceled();
    }

    private void reportProgress() {
        progress.onProgress("Preparing selected HD files: " + (copied / (1024 * 1024)) + " MiB copied");
    }

    @Override public void close() throws IOException {
        IOException failure = null;
        if (imageDescriptor != null) {
            try { imageDescriptor.close(); } catch (IOException error) { failure = error; }
            finally { imageDescriptor = null; }
        }
        try {
            workspace.close();
        } catch (IOException error) {
            if (failure == null) failure = error; else failure.addSuppressed(error);
        }
        if (failure != null) throw failure;
    }
}
