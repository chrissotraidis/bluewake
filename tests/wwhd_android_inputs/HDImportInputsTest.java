package dev.bluewake.android;

import android.app.Instrumentation;
import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.res.AssetFileDescriptor;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.os.OperationCanceledException;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.FileVisitResult;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

/** Standalone generated-provider instrumentation; no game, native importer or UI. */
@SuppressWarnings("try") // Several rejection/lifetime cases deliberately only close the result.
public final class HDImportInputsTest extends Instrumentation {
    private static final String AUTHORITY = "dev.bluewake.hdinputs.test.documents";
    private static final byte[] DATA = new byte[131091];
    private static final byte[] KEY = new byte[16];
    private int passed;
    private String phase;
    private final List<HDImportInputs> deathOwners = new ArrayList<>();
    private static native byte[] nativeRead(String path);

    private static byte[] mappedWux(byte[] data) {
        int sector = 32768, count = (data.length + sector - 1) / sector;
        ByteBuffer out = ByteBuffer.allocate((count + 1) * sector).order(ByteOrder.LITTLE_ENDIAN);
        out.put(new byte[]{'W', 'U', 'X', '0'}).putInt(0x1099D02E).putInt(sector).putInt(0).putLong(data.length);
        for (int i = 0; i < count; ++i) {
            out.putInt(32 + i * 4, count - i - 1);
            System.arraycopy(data, i * sector, out.array(), (count - i) * sector, Math.min(sector, data.length - i * sector));
        }
        return out.array();
    }

    private static final class Node {
        final String id;
        String name;
        byte[] bytes;
        Long size;
        String mode = "file";
        final List<String> children = new ArrayList<>();
        Node(String id, String name, byte[] bytes) {
            this.id = id; this.name = name; this.bytes = bytes;
            this.size = bytes == null ? null : (long)bytes.length;
        }
    }

    public static final class Provider extends ContentProvider {
        static final Map<String, Node> nodes = new HashMap<>();
        static final Set<String> opened = new HashSet<>();
        static final List<Thread> writers = new ArrayList<>();
        static CountDownLatch entered, release;
        static String block = "", listing = "";
        static boolean readOnly;
        static int fd;

        @Override public boolean onCreate() { return true; }
        @Override public String getType(Uri uri) { return "application/octet-stream"; }
        @Override public Uri insert(Uri uri, ContentValues values) { throw new AssertionError("provider write"); }
        @Override public int update(Uri uri, ContentValues values, String where, String[] args) { throw new AssertionError("provider write"); }
        @Override public int delete(Uri uri, String where, String[] args) { throw new AssertionError("provider write"); }
        @Override public Cursor query(Uri uri, String[] columns, String where, String[] args, String sort) {
            return query(uri, columns, where, args, sort, null);
        }
        @Override public Cursor query(Uri uri, String[] columns, String where, String[] args, String sort,
                CancellationSignal signal) {
            require(!Thread.holdsLock(HDImportWorkspace.class), "provider query under initialization monitor");
            if (block.equals("query")) waitForCancellation(signal);
            MatrixCursor cursor = new MatrixCursor(columns);
            String id = DocumentsContract.getDocumentId(uri);
            Node node = nodes.get(id);
            if (uri.getLastPathSegment().equals("children")) {
                for (String child : node.children) row(cursor, columns, nodes.get(child));
            } else row(cursor, columns, node);
            Bundle extras = new Bundle();
            if (listing.equals("loading")) extras.putBoolean(DocumentsContract.EXTRA_LOADING, true);
            if (listing.equals("error")) extras.putString(DocumentsContract.EXTRA_ERROR, "synthetic listing error");
            cursor.setExtras(extras);
            return cursor;
        }
        private static void row(MatrixCursor cursor, String[] columns, Node node) {
            MatrixCursor.RowBuilder row = cursor.newRow();
            for (String column : columns) {
                if (column.equals(Document.COLUMN_DOCUMENT_ID)) row.add(node.id);
                else if (column.equals(Document.COLUMN_DISPLAY_NAME)) row.add(node.name);
                else if (column.equals(Document.COLUMN_SIZE)) row.add(node.size);
                else if (column.equals(Document.COLUMN_MIME_TYPE))
                    row.add(node.bytes == null ? Document.MIME_TYPE_DIR : "application/octet-stream");
                else row.add(null);
            }
        }
        private static void waitForCancellation(CancellationSignal signal) {
            if (signal == null) throw new AssertionError("provider did not receive cancellation signal");
            entered.countDown();
            while (!signal.isCanceled()) {
                try { if (release.await(10, TimeUnit.MILLISECONDS)) break; }
                catch (InterruptedException error) { throw new AssertionError(error); }
            }
            signal.throwIfCanceled();
        }
        @Override public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
            return openFile(uri, mode, null);
        }
        @Override public AssetFileDescriptor openAssetFile(Uri uri, String mode, CancellationSignal signal)
                throws FileNotFoundException {
            // ContentResolver.openFileDescriptor uses the asset entry point.
            // Real DocumentsProvider forwards this signal to openDocument too.
            return new AssetFileDescriptor(openFile(uri, mode, signal), 0, AssetFileDescriptor.UNKNOWN_LENGTH);
        }
        @Override public AssetFileDescriptor openTypedAssetFile(Uri uri, String type, Bundle options,
                CancellationSignal signal) throws FileNotFoundException {
            return openAssetFile(uri, "r", signal);
        }
        @Override public ParcelFileDescriptor openFile(Uri uri, String mode, CancellationSignal signal)
                throws FileNotFoundException {
            require(!Thread.holdsLock(HDImportWorkspace.class), "provider open under initialization monitor");
            if (!mode.equals("r")) { readOnly = false; throw new AssertionError("provider opened writable"); }
            if (block.equals("open")) waitForCancellation(signal);
            Node node = nodes.get(DocumentsContract.getDocumentId(uri));
            opened.add(node.id);
            try {
                if (node.mode.equals("file") || node.mode.equals("close-error")) {
                    Path file = Files.createTempFile(getContext().getFilesDir().toPath(), "generated-", ".bin");
                    Files.write(file, node.bytes);
                    ParcelFileDescriptor descriptor = ParcelFileDescriptor.open(file.toFile(), ParcelFileDescriptor.MODE_READ_ONLY);
                    Files.delete(file); // Only descriptors own these generated bytes now.
                    fd = descriptor.getFd();
                    if (node.mode.equals("close-error")) return new ParcelFileDescriptor(descriptor) {
                        @Override public void close() throws IOException {
                            super.close();
                            throw new IOException("synthetic descriptor close failure");
                        }
                    };
                    return descriptor;
                }
                ParcelFileDescriptor[] pipe = ParcelFileDescriptor.createReliablePipe();
                Thread writer = new Thread(() -> {
                    try {
                        if (node.mode.equals("idle")) { entered.countDown(); release.await(); }
                        if (node.mode.equals("error")) {
                            pipe[1].closeWithError("synthetic provider failure");
                        } else {
                            try (ParcelFileDescriptor.AutoCloseOutputStream out = new ParcelFileDescriptor.AutoCloseOutputStream(pipe[1])) {
                                out.write(node.bytes);
                            }
                        }
                    } catch (IOException expectedWhenCancelled) {
                        try { pipe[1].close(); } catch (IOException ignored) { }
                    } catch (InterruptedException error) { throw new AssertionError(error); }
                }, "generated-provider-writer");
                writers.add(writer);
                writer.start();
                return pipe[0];
            } catch (IOException error) { throw new FileNotFoundException("generated provider failed"); }
        }
    }

    private static Uri document(String id) { return DocumentsContract.buildDocumentUri(AUTHORITY, id); }
    private static Uri tree(String id) { return DocumentsContract.buildTreeDocumentUri(AUTHORITY, id); }
    private static Node node(String id, String name, byte[] bytes) {
        Node node = new Node(id, name, bytes);
        Provider.nodes.put(id, node);
        return node;
    }
    private static void require(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
    private static byte[] read(String path) throws IOException {
        try (FileInputStream stream = new FileInputStream(path)) { return stream.readAllBytes(); }
    }
    private Map<String, String> storage() throws IOException {
        Map<String, String> result = new HashMap<>();
        for (File base : new File[]{getTargetContext().getCacheDir(), getTargetContext().getNoBackupFilesDir()}) {
            Path root = base.toPath();
            Files.walkFileTree(root, new SimpleFileVisitor<Path>() {
                private String name(Path path) { return base.getName() + "/" + root.relativize(path); }
                @Override public FileVisitResult preVisitDirectory(Path path, BasicFileAttributes info) {
                    result.put(name(path), "directory");
                    return FileVisitResult.CONTINUE;
                }
                @Override public FileVisitResult visitFile(Path path, BasicFileAttributes info) throws IOException {
                    result.put(name(path), info.isSymbolicLink() ? "link:" + Files.readSymbolicLink(path)
                            : Arrays.toString(Files.readAllBytes(path)));
                    return FileVisitResult.CONTINUE;
                }
            });
        }
        return result;
    }
    private void reset() throws Exception {
        for (Thread writer : Provider.writers) {
            writer.join(2000);
            require(!writer.isAlive(), "provider writer still running");
        }
        Provider.writers.clear(); Provider.nodes.clear(); Provider.opened.clear();
        Provider.block = ""; Provider.listing = ""; Provider.readOnly = true;
        Provider.entered = new CountDownLatch(1); Provider.release = new CountDownLatch(1);
        node("image", "toy.WUX", DATA);
        node("disc", "synthetic-disc.key", KEY);
        node("contentKey", "synthetic-content.key", "00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff\n".getBytes(StandardCharsets.US_ASCII));
        node("root", "dump", null).children.addAll(Arrays.asList("content", "unneeded"));
        node("content", "content", null).children.addAll(Arrays.asList("nested", "ignored"));
        node("nested", "Object", null).children.addAll(Arrays.asList("resource", "pack", "szs", "sarc"));
        node("resource", "雪.bfres", DATA);
        node("pack", "A.PACK", DATA); node("szs", "A.szs", DATA); node("sarc", "A.sarc", DATA);
        node("ignored", "ignored.bflim", DATA); node("unneeded", "outside.bfres", DATA);
    }
    private HDImportInputs prepare(boolean image, CancellationSignal cancel, HDImportInputs.Progress progress) throws IOException {
        return HDImportInputs.prepare(getTargetContext(), image ? document("image") : tree("root"), image,
            image ? document("disc") : null, image ? document("contentKey") : null, true, cancel, progress);
    }
    private interface Case { void run() throws Exception; }
    private void check(String label, Case test) throws Exception {
        reset();
        Map<String, String> before = storage();
        test.run();
        require(storage().equals(before), label + ": leaked staging or touched another import");
        require(Provider.readOnly, "provider was opened writable");
        ++passed;
        Bundle status = new Bundle(); status.putString("stream", "PASS: " + label + "\n"); sendStatus(0, status);
    }
    private void failure(Case action) throws Exception {
        try { action.run(); } catch (IOException expected) { return; }
        throw new AssertionError("expected rejected input");
    }
    private void rejected(boolean image) throws Exception {
        failure(() -> { try (HDImportInputs ignored = prepare(image, new CancellationSignal(), null)) { } });
    }
    private void cancelWhile(String block) throws Exception {
        CancellationSignal cancel = new CancellationSignal();
        Provider.block = block;
        if (block.equals("pipe")) Provider.nodes.get("image").mode = "idle";
        ExecutorService worker = Executors.newSingleThreadExecutor();
        Future<?> result = worker.submit(() -> {
            try (HDImportInputs ignored = prepare(true, cancel, null)) {
                throw new AssertionError("cancelled preparation succeeded");
            } catch (OperationCanceledException expected) { }
            catch (IOException error) { throw new AssertionError(error); }
        });
        try {
            if (!Provider.entered.await(2, TimeUnit.SECONDS)) {
                if (result.isDone()) result.get();
                throw new AssertionError("provider boundary not reached");
            }
            cancel.cancel();
            result.get(2, TimeUnit.SECONDS); // Pipe writer has NOT replied or closed yet.
        } finally {
            Provider.release.countDown(); worker.shutdownNow();
            require(worker.awaitTermination(2, TimeUnit.SECONDS), "adapter worker still running");
        }
    }

    private void tests() throws Exception {
        HDImportWorkspace.initialize(getTargetContext()); // Persistent root/lock are not a leak.
        for (int i = 0; i < DATA.length; ++i) DATA[i] = (byte)(i * 7);
        for (int i = 0; i < KEY.length; ++i) KEY[i] = (byte)i;
        Path unrelated = Files.createTempFile(getTargetContext().getCacheDir().toPath(), "unrelated-", ".txt");
        Files.write(unrelated, KEY);
        Path legacy = Files.createTempDirectory(getTargetContext().getCacheDir().toPath(), "wwhd-input-legacy-");
        Files.write(legacy.resolve("disc.key"), KEY);
        check("tree root filters content and preserves relative names", () -> {
            try (HDImportInputs input = prepare(false, new CancellationSignal(), null)) {
                require(Arrays.equals(read(input.sourcePath + "/Object/雪.bfres"), DATA), "folder bytes differ");
                require(Provider.opened.equals(new HashSet<>(Arrays.asList("resource", "pack", "szs", "sarc"))), "wrong source filters");
                require(input.discKeyPath.isEmpty() && input.contentKeyPath.isEmpty(), "folder unexpectedly has keys");
            }
        });
        check("selected content folder", () -> {
            try (HDImportInputs input = HDImportInputs.prepare(getTargetContext(), tree("content"), false,
                    null, null, false, new CancellationSignal(), null)) {
                require(Arrays.equals(read(input.sourcePath + "/Object/雪.bfres"), DATA), "content-root bytes differ");
            }
        });
        check("seekable image descriptor lifetime and transient keys", () -> {
            String path, keyPath;
            HDImportInputs input = prepare(true, new CancellationSignal(), null);
            try {
                path = input.sourcePath; keyPath = input.discKeyPath;
                require(Files.isSymbolicLink(Path.of(path)), "seekable image was copied");
                require(Files.readSymbolicLink(Path.of(path)).toString().startsWith("/proc/self/fd/"), "unexpected alias target");
                require(Arrays.equals(read(path), DATA), "descriptor bytes differ");
                require(Arrays.equals(nativeRead(path), DATA), "native descriptor reopen/read failed");
                require(Arrays.equals(read(keyPath), KEY), "binary key changed");
            } finally { input.close(); }
            failure(() -> read(path));
            require(!Files.exists(Path.of(keyPath)), "key copy survived close");
            input.close(); // idempotent
        });
        check("mapped WUX native reads through named descriptor alias", () -> {
            Node image = Provider.nodes.get("image"); image.bytes = mappedWux(DATA); image.size = (long)image.bytes.length;
            try (HDImportInputs input = prepare(true, new CancellationSignal(), null)) {
                require(Files.isSymbolicLink(Path.of(input.sourcePath)), "WUX copied unnecessarily");
                require(Arrays.equals(nativeRead(input.sourcePath), DATA), "native WUX mapping differs");
            }
        });
        check("reliable pipe fallback with unknown size", () -> {
            Provider.nodes.get("image").mode = "pipe"; Provider.nodes.get("image").size = null;
            try (HDImportInputs input = prepare(true, new CancellationSignal(), null)) {
                require(!Files.isSymbolicLink(Path.of(input.sourcePath)), "pipe exposed to native");
                require(Arrays.equals(read(input.sourcePath), DATA), "pipe copy differs");
                require(Arrays.equals(nativeRead(input.sourcePath), DATA), "native pipe fallback read failed");
            }
        });
        check("folder resource pipe copy", () -> {
            Provider.nodes.get("resource").mode = "pipe";
            try (HDImportInputs input = prepare(false, new CancellationSignal(), null)) {
                require(Arrays.equals(read(input.sourcePath + "/Object/雪.bfres"), DATA), "tree pipe copy differs");
            }
        });
        for (String name : new String[]{"../escape.bfres", "a/b.bfres", "a\\b.bfres", "..", "", "nul\0.bfres"})
            check("reject unsafe provider name", () -> { Provider.nodes.get("resource").name = name; rejected(false); });
        check("duplicate logical names", () -> { Provider.nodes.get("pack").name = "雪.BFRES"; rejected(false); });
        check("document cycle", () -> { Provider.nodes.get("nested").children.add("root"); rejected(false); });
        check("repeated document ID", () -> { Provider.nodes.get("nested").children.add("resource"); rejected(false); });
        check("depth bound", () -> {
            Node parent = Provider.nodes.get("nested"); parent.children.clear();
            for (int i = 0; i < 34; ++i) { String id = "deep" + i; parent.children.add(id); parent = node(id, "d", null); }
            rejected(false);
        });
        for (String extra : new String[]{"loading", "error"})
            check("reject incomplete provider listing", () -> { Provider.listing = extra; rejected(false); });
        check("oversized resource rejected before open", () -> {
            Provider.nodes.get("resource").size = 256L * 1024 * 1024 + 1;
            rejected(false); require(!Provider.opened.contains("resource"), "oversized file opened");
        });
        check("truncated regular image", () -> { Provider.nodes.get("image").size = (long)DATA.length + 1; rejected(true); });
        check("truncated pipe", () -> {
            Provider.nodes.get("image").mode = "pipe"; Provider.nodes.get("image").size = (long)DATA.length + 1; rejected(true);
        });
        check("overlong pipe", () -> {
            Provider.nodes.get("image").mode = "pipe"; Provider.nodes.get("image").size = (long)DATA.length - 1; rejected(true);
        });
        check("provider closeWithError rejected", () -> { Provider.nodes.get("image").mode = "error"; rejected(true); });
        check("key descriptor close failure rejects preparation", () -> { Provider.nodes.get("disc").mode = "close-error"; rejected(true); });
        check("image close failure still removes owned staging", () -> {
            Provider.nodes.get("image").mode = "close-error";
            HDImportInputs input = prepare(true, new CancellationSignal(), null);
            failure(input::close);
            input.close();
        });
        check("oversized key", () -> { Provider.nodes.get("disc").bytes = new byte[1025]; Provider.nodes.get("disc").size = 1025L; rejected(true); });
        check("invalid hex key", () -> { Provider.nodes.get("contentKey").bytes[0] = 'z'; rejected(true); });
        check("pre-cancel creates no files", () -> {
            CancellationSignal cancel = new CancellationSignal(); cancel.cancel();
            try (HDImportInputs ignored = prepare(true, cancel, null)) { throw new AssertionError("pre-cancel succeeded"); }
            catch (OperationCanceledException expected) { }
            require(Provider.opened.isEmpty(), "pre-cancel opened provider");
        });
        for (String block : new String[]{"query", "open", "pipe"}) check("cancel during " + block, () -> cancelWhile(block));
        check("cancel during copying", () -> {
            CancellationSignal cancel = new CancellationSignal();
            Provider.nodes.get("image").mode = "pipe";
            Provider.nodes.get("image").bytes = new byte[3 * 1024 * 1024];
            Provider.nodes.get("image").size = (long)Provider.nodes.get("image").bytes.length;
            try (HDImportInputs ignored = prepare(true, cancel, bytes -> cancel.cancel())) {
                throw new AssertionError("copy cancel succeeded");
            } catch (OperationCanceledException expected) { }
        });
        check("separate owner survives another owner's close", () -> {
            try (HDImportInputs first = prepare(true, new CancellationSignal(), null)) {
                try (HDImportInputs second = prepare(false, new CancellationSignal(), null)) { }
                require(Arrays.equals(read(first.sourcePath), DATA), "other descriptor closed");
                require(Arrays.equals(read(first.discKeyPath), KEY), "other staging removed");
            }
        });
        check("repeated concurrent initialization never sweeps a live owner", () -> {
            try (HDImportInputs input = prepare(true, new CancellationSignal(), null)) {
                ExecutorService workers = Executors.newFixedThreadPool(2);
                try {
                    Future<Path> first = workers.submit(() -> HDImportWorkspace.initialize(getTargetContext()));
                    Future<Path> second = workers.submit(() -> HDImportWorkspace.initialize(getTargetContext()));
                    require(first.get(2, TimeUnit.SECONDS).equals(second.get(2, TimeUnit.SECONDS)), "initializer roots differ");
                    require(Arrays.equals(read(input.discKeyPath), KEY), "initializer removed live keys");
                    require(Arrays.equals(read(input.sourcePath), DATA), "initializer removed live image alias");
                } finally { workers.shutdownNow(); }
            }
        });
        check("cleanup preserves marker after bounded-depth failure and can retry", () -> {
            HDImportWorkspace workspace = HDImportWorkspace.create(getTargetContext());
            Path marker = workspace.path.resolve(".owner");
            byte[] originalMarker = Files.readAllBytes(marker);
            Files.write(workspace.path.resolve("disc.key"), KEY);
            List<Path> nested = new ArrayList<>();
            Path at = workspace.path;
            for (int i = 0; i < 42; ++i) { at = Files.createDirectory(at.resolve("d")); nested.add(at); }
            failure(workspace::close);
            require(Arrays.equals(originalMarker, Files.readAllBytes(marker)), "partial cleanup lost marker");
            require(!Files.exists(workspace.path.resolve("disc.key")), "key was not removed before tree cleanup");
            for (int i = nested.size() - 1; i >= 0; --i) Files.delete(nested.get(i));
            workspace.close(); workspace.close();
        });
        check("invalid ownership marker preserves payload", () -> {
            HDImportWorkspace workspace = HDImportWorkspace.create(getTargetContext());
            Path marker = workspace.path.resolve(".owner"), payload = workspace.path.resolve("disc.key");
            byte[] originalMarker = Files.readAllBytes(marker);
            Files.write(marker, new byte[]{1}); Files.write(payload, KEY);
            failure(workspace::close);
            require(Arrays.equals(KEY, Files.readAllBytes(payload)), "invalid ownership removed payload");
            Files.write(marker, originalMarker); workspace.close();
        });
        reset(); // Join all generated pipe writers before reporting completion.
        require(Arrays.equals(Files.readAllBytes(unrelated), KEY), "unowned cache file changed");
        Files.delete(unrelated);
        require(Arrays.equals(KEY, Files.readAllBytes(legacy.resolve("disc.key"))), "legacy cache input changed");
        Files.delete(legacy.resolve("disc.key")); Files.delete(legacy);
    }

    private Path workspaceRoot() { return getTargetContext().getNoBackupFilesDir().toPath().resolve("wwhd-input-v1"); }
    private Path files() { return getTargetContext().getFilesDir().toPath(); }
    private static String protectedName(int index) { return "input-00000000-0000-0000-0000-00000000000" + index; }

    private void prepareDeath() throws Exception {
        reset();
        for (int i = 0; i < DATA.length; ++i) DATA[i] = (byte)(i * 7);
        HDImportInputs image = prepare(true, new CancellationSignal(), null);
        deathOwners.add(image);
        deathOwners.add(prepare(false, new CancellationSignal(), null));
        Provider.nodes.get("image").mode = "pipe";
        deathOwners.add(prepare(true, new CancellationSignal(), null));
        List<String> names = new ArrayList<>();
        for (HDImportInputs input : deathOwners) {
            Path directory = Path.of(input.discKeyPath.isEmpty() ? input.sourcePath : input.discKeyPath).getParent();
            names.add(directory.getFileName().toString());
        }
        Path root = workspaceRoot();
        byte[] marker = Files.readAllBytes(root.resolve(names.get(0)).resolve(".owner"));
        Path victim = Files.createDirectory(files().resolve("recovery-victim"));
        Files.write(victim.resolve("keep.bin"), DATA);
        Files.write(victim.resolve(".owner"), marker);
        // Recovery must preserve unmarked/invalid/linked roots even when they
        // have names that otherwise look like an importer workspace.
        for (int i = 1; i <= 3; ++i) {
            Path protectedDirectory = Files.createDirectory(root.resolve(protectedName(i)));
            Files.write(protectedDirectory.resolve("keep.bin"), DATA);
            if (i == 2) Files.write(protectedDirectory.resolve(".owner"), new byte[]{1});
            if (i == 3) Files.createSymbolicLink(protectedDirectory.resolve(".owner"), victim.resolve(".owner"));
        }
        Files.createSymbolicLink(root.resolve(protectedName(4)), victim);
        Path legacy = Files.createDirectory(getTargetContext().getCacheDir().toPath().resolve("wwhd-input-death-legacy"));
        Files.write(legacy.resolve("keep.bin"), DATA);
        Files.write(root.resolve(names.get(0)).resolve("partly-removed.bin"), DATA);
        Files.delete(root.resolve(names.get(0)).resolve("partly-removed.bin"));
        Files.write(files().resolve("recovery-state.txt"), names, StandardCharsets.US_ASCII);
        Bundle ready = new Bundle();
        ready.putString("stream", "RECOVERY_READY pid=" + android.os.Process.myPid() + "\n");
        sendStatus(0, ready);
        new CountDownLatch(1).await(); // Host force-stops this process. No close/finally cleanup.
    }

    private void recoverDeath() throws Exception {
        Path root = workspaceRoot(), victim = files().resolve("recovery-victim");
        byte[] victimMarker = Files.readAllBytes(victim.resolve(".owner"));
        List<String> names = Files.readAllLines(files().resolve("recovery-state.txt"), StandardCharsets.US_ASCII);
        require(names.size() == 3, "missing process-death owners");
        for (String name : names) {
            require(name.matches("input-[0-9a-f-]{36}"), "invalid fixture workspace name");
            require(Files.isDirectory(root.resolve(name), LinkOption.NOFOLLOW_LINKS), "staging disappeared before recovery");
        }
        Path image = root.resolve(names.get(0));
        require(Files.exists(image.resolve("disc.key")) && Files.exists(image.resolve("title.key")), "no transient keys survived kill");
        require(Files.exists(root.resolve(names.get(1)).resolve("input/Object/雪.bfres")), "no folder copy survived kill");
        require(Files.exists(root.resolve(names.get(2)).resolve("source.wux")), "no pipe copy survived kill");
        // Rebind the stale proc alias to an actually live descriptor in this new
        // process. Recovery must unlink it, not inspect or delete its target.
        try (ParcelFileDescriptor descriptor = ParcelFileDescriptor.open(victim.resolve("keep.bin").toFile(), ParcelFileDescriptor.MODE_READ_ONLY)) {
            Files.delete(image.resolve("disc.wud"));
            Files.createSymbolicLink(image.resolve("disc.wud"), Path.of("/proc/self/fd/" + descriptor.getFd()));
            Files.createSymbolicLink(image.resolve("outside-directory"), victim);
            ExecutorService workers = Executors.newFixedThreadPool(2);
            try {
                Future<Path> first = workers.submit(() -> HDImportWorkspace.initialize(getTargetContext()));
                Future<Path> second = workers.submit(() -> HDImportWorkspace.initialize(getTargetContext()));
                require(first.get(5, TimeUnit.SECONDS).equals(second.get(5, TimeUnit.SECONDS)), "restart roots differ");
            } finally { workers.shutdownNow(); }
            require(descriptor.getStatSize() == DATA.length, "recovery changed provider descriptor target");
        }
        for (String name : names) require(!Files.exists(root.resolve(name), LinkOption.NOFOLLOW_LINKS), "killed owner's workspace remains");
        byte[] expected = new byte[DATA.length];
        for (int i = 0; i < expected.length; ++i) expected[i] = (byte)(i * 7);
        require(Arrays.equals(Files.readAllBytes(victim.resolve("keep.bin")), expected), "outside target changed");
        require(Arrays.equals(Files.readAllBytes(victim.resolve(".owner")), victimMarker), "outside marker changed");
        for (int i = 1; i <= 4; ++i) require(Files.exists(root.resolve(protectedName(i)), LinkOption.NOFOLLOW_LINKS), "unowned root removed");
        require(!Files.exists(root.resolve(protectedName(1)).resolve(".owner"), LinkOption.NOFOLLOW_LINKS), "unmarked root was adopted");
        require(Arrays.equals(Files.readAllBytes(root.resolve(protectedName(2)).resolve(".owner")), new byte[]{1}), "invalid marker changed");
        require(Files.readSymbolicLink(root.resolve(protectedName(3)).resolve(".owner")).equals(victim.resolve(".owner")), "linked marker changed");
        require(Files.readSymbolicLink(root.resolve(protectedName(4))).equals(victim), "linked root changed");
        for (int i = 1; i <= 3; ++i) {
            Path protectedDirectory = root.resolve(protectedName(i));
            require(Arrays.equals(Files.readAllBytes(protectedDirectory.resolve("keep.bin")), expected), "unowned payload changed");
            Files.deleteIfExists(protectedDirectory.resolve(".owner"));
            Files.delete(protectedDirectory.resolve("keep.bin")); Files.delete(protectedDirectory);
        }
        Files.delete(root.resolve(protectedName(4)));
        Path legacy = getTargetContext().getCacheDir().toPath().resolve("wwhd-input-death-legacy");
        require(Arrays.equals(Files.readAllBytes(legacy.resolve("keep.bin")), expected), "legacy cache copy changed");
        Files.delete(legacy.resolve("keep.bin")); Files.delete(legacy);
        Files.delete(victim.resolve("keep.bin")); Files.delete(victim.resolve(".owner")); Files.delete(victim);
        Files.delete(files().resolve("recovery-state.txt"));
        reset();
        try (HDImportInputs input = prepare(true, new CancellationSignal(), null)) {
            require(Files.exists(Path.of(input.discKeyPath)), "new import after recovery failed");
        }
    }

    @Override public void onCreate(Bundle arguments) {
        super.onCreate(arguments);
        phase = arguments == null ? "" : arguments.getString("phase", "");
        start();
    }
    @Override public void onStart() {
        Bundle result = new Bundle();
        try {
            System.loadLibrary("wwhd_inputs_probe");
            if (phase.equals("crash-prepare")) { prepareDeath(); throw new AssertionError("kill fixture returned"); }
            if (phase.equals("crash-recover")) {
                recoverDeath();
                result.putString("stream", "PASS: real process-death input recovery, retained-lock release, preservation and proc-fd aliases\n");
                finish(-1, result);
                return;
            }
            tests();
            result.putString("stream", "PASS: " + passed + " generated Android document-adapter cases\n");
            finish(-1, result);
        } catch (Throwable error) {
            result.putString("stream", "FAIL: " + error + "\n" + android.util.Log.getStackTraceString(error));
            finish(1, result);
        }
    }
}
