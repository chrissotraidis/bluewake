package dev.bluewake.android;

import android.content.Context;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;
import java.nio.channels.OverlappingFileLockException;
import java.nio.charset.StandardCharsets;
import java.nio.file.DirectoryStream;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.StandardOpenOption;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.Arrays;
import java.util.UUID;

/** Owns only temporary importer copies, never provider documents or output packs. */
final class HDImportWorkspace implements AutoCloseable {
    private static final String ROOT = "wwhd-input-v1", MARKER = ".owner";
    private static final byte[] MAGIC = "BlueWake HD input workspace v1\n".getBytes(StandardCharsets.US_ASCII);
    private static final int MAX_ROOT_ENTRIES = 128, MAX_VISITS = 100016, MAX_DEPTH = 40;
    // One channel for this inode, retained until process death. Never remove the
    // lock file or release it when an individual input owner closes.
    private static FileChannel lockChannel;
    private static FileLock processLock;
    private static Path initializedRoot;
    final Path path;
    private boolean closed;

    private HDImportWorkspace(Path path) { this.path = path; }

    static void recoverOnStartup(Context context) {
        Context app = context.getApplicationContext();
        try {
            new Thread(() -> {
                try { initialize(app); }
                catch (IOException | RuntimeException failure) {
                    Log.w("BlueWake", "Temporary HD input recovery could not start.");
                }
            }, "HDInputRecovery").start();
        } catch (RuntimeException failure) {
            // prepare() retries initialization on its own worker.
            Log.w("BlueWake", "Temporary HD input recovery could not start.");
        }
    }

    // Local, bounded filesystem work only. No provider calls or callbacks under
    // this monitor. Once it returns, no later initializer sweeps live owners.
    static synchronized Path initialize(Context context) throws IOException {
        Path root = context.getNoBackupFilesDir().toPath().resolve(ROOT);
        if (initializedRoot != null) {
            if (!initializedRoot.equals(root) || !processLock.isValid())
                throw new IOException("The temporary input workspace is unavailable.");
            return initializedRoot;
        }
        try { Files.createDirectory(root); }
        catch (FileAlreadyExistsException exists) { /* Checked without following links below. */ }
        if (!attributes(root).isDirectory())
            throw new IOException("The temporary input workspace is not a directory.");
        Path lock = root.resolve(".lock");
        try {
            if (!attributes(lock).isRegularFile())
                throw new IOException("The temporary input lock is not a regular file.");
        } catch (NoSuchFileException absent) { /* Created without following links. */ }
        FileChannel channel = FileChannel.open(lock, StandardOpenOption.CREATE,
                StandardOpenOption.WRITE, LinkOption.NOFOLLOW_LINKS);
        FileLock held = null;
        try {
            try { held = channel.tryLock(); }
            catch (OverlappingFileLockException busy) {
                throw new IOException("Temporary HD inputs are already in use.");
            }
            if (held == null) throw new IOException("Temporary HD inputs are already in use.");
            boolean complete = recover(root);
            lockChannel = channel;
            processLock = held;
            initializedRoot = root;
            if (!complete) Log.w("BlueWake", "Some temporary HD inputs remain for a later startup cleanup.");
            return root;
        } finally {
            if (initializedRoot == null) {
                try { if (held != null) held.release(); }
                finally { channel.close(); }
            }
        }
    }

    static HDImportWorkspace create(Context context) throws IOException {
        Path root = initialize(context);
        // Detect accidental replacement of the dedicated root; do not follow it.
        if (!attributes(root).isDirectory()) throw new IOException("The temporary input workspace is unavailable.");
        Path directory = Files.createDirectory(root.resolve("input-" + UUID.randomUUID()));
        try {
            Files.write(directory.resolve(MARKER), MAGIC, StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE);
            return new HDImportWorkspace(directory);
        } catch (IOException | RuntimeException | Error failure) {
            // No payload/provider work has started yet, including on a short marker write.
            try { Files.deleteIfExists(directory.resolve(MARKER)); Files.delete(directory); }
            catch (IOException cleanup) { failure.addSuppressed(cleanup); }
            throw failure;
        }
    }

    private static BasicFileAttributes attributes(Path path) throws IOException {
        return Files.readAttributes(path, BasicFileAttributes.class, LinkOption.NOFOLLOW_LINKS);
    }

    private static boolean marked(Path directory) throws IOException {
        if (!attributes(directory).isDirectory()) return false;
        Path marker = directory.resolve(MARKER);
        BasicFileAttributes info = attributes(marker);
        if (!info.isRegularFile() || info.size() != MAGIC.length) return false;
        ByteBuffer bytes = ByteBuffer.allocate(MAGIC.length + 1);
        try (FileChannel channel = FileChannel.open(marker, StandardOpenOption.READ, LinkOption.NOFOLLOW_LINKS)) {
            while (bytes.hasRemaining() && channel.read(bytes) > 0) { }
        }
        return bytes.position() == MAGIC.length && Arrays.equals(MAGIC, Arrays.copyOf(bytes.array(), MAGIC.length));
    }

    private static final class Budget {
        int left = MAX_VISITS;
        void visit() throws IOException {
            if (--left < 0) throw new IOException("Temporary input cleanup reached its entry limit.");
        }
    }

    private static boolean recover(Path root) throws IOException {
        Budget budget = new Budget();
        boolean complete = true;
        int scanned = 0;
        try (DirectoryStream<Path> entries = Files.newDirectoryStream(root)) {
            for (Path directory : entries) {
                if (++scanned > MAX_ROOT_ENTRIES || budget.left <= 0) return false;
                String name = directory.getFileName().toString();
                if (!name.matches("input-[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}")) continue;
                try {
                    if (marked(directory)) remove(directory, budget);
                } catch (NoSuchFileException absent) { /* Unmarked or already gone: never adopt it. */ }
                catch (IOException failure) { complete = false; }
            }
        }
        return complete;
    }

    private static void remove(Path directory, Budget budget) throws IOException {
        // Remove transient keys before large trees, without opening any key or
        // resolving a symlink (especially the /proc/self/fd image alias).
        for (String key : new String[]{"disc.key", "common.key", "title.key"}) {
            budget.visit();
            Files.deleteIfExists(directory.resolve(key));
        }
        Path marker = directory.resolve(MARKER);
        Files.walkFileTree(directory, new SimpleFileVisitor<Path>() {
            @Override public FileVisitResult preVisitDirectory(Path path, BasicFileAttributes info) throws IOException {
                budget.visit();
                if (directory.relativize(path).getNameCount() > MAX_DEPTH)
                    throw new IOException("Temporary input cleanup reached its depth limit.");
                return FileVisitResult.CONTINUE;
            }
            @Override public FileVisitResult visitFile(Path file, BasicFileAttributes info) throws IOException {
                budget.visit();
                if (!file.equals(marker)) Files.delete(file);
                return FileVisitResult.CONTINUE;
            }
            @Override public FileVisitResult postVisitDirectory(Path path, IOException failure) throws IOException {
                if (failure != null) throw failure;
                if (!path.equals(directory)) Files.delete(path);
                return FileVisitResult.CONTINUE;
            }
        }); // No FOLLOW_LINKS. A child symlink is unlinked, never traversed.
        // Keep ownership evidence after any partial cleanup or process kill.
        Files.delete(marker);
        Files.delete(directory);
    }

    @Override public synchronized void close() throws IOException {
        if (closed) return;
        try {
            if (!marked(path)) throw new IOException("Temporary input ownership marker is invalid.");
        } catch (NoSuchFileException absent) {
            if (Files.exists(path, LinkOption.NOFOLLOW_LINKS)) throw absent;
            closed = true;
            return;
        }
        remove(path, new Budget());
        closed = true;
    }
}
