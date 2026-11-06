// Standalone host JVM / Android app_process fixture runner; never loads the game.
package dev.bluewake.android;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.stream.Stream;

final class WWHDAndroidNativeTest {
    private static Path root;
    private static int assertions;

    private static void check(boolean value, String label) {
        ++assertions;
        if (!value) throw new AssertionError(label);
    }

    private static byte[] path(Path value) { return value.toString().getBytes(StandardCharsets.UTF_8); }
    private static Path input(String name) { return root.resolve("inputs").resolve(name); }
    private static Path output(String name) { return root.resolve("outputs").resolve(name); }

    private static String[] run(long id, String source, String key, boolean title, String target,
                                HDImportNative.Progress progress) {
        return HDImportNative.run(id, path(input(source)), key == null ? null : path(input("disc.key")),
                                  key == null ? null : path(input(key)), title, path(output(target)), progress);
    }

    private static void status(String[] result, String wanted) {
        check(result != null && result.length == 4, "four result fields");
        check(result[0].equals(wanted), "expected " + wanted + ", got " + Arrays.toString(result));
        check(!result[3].isEmpty(), "result message");
        if (!wanted.equals("success")) {
            check(result[1].isEmpty() && result[2].equals("0"), "failure has no installed pack");
        }
    }

    private static void empty(String target) throws Exception {
        if (!Files.exists(output(target))) return;
        try (Stream<Path> entries = Files.list(output(target))) {
            Path[] remaining = entries.toArray(Path[]::new);
            // Output ownership keeps one empty lock after failure/cancellation.
            check(remaining.length == 0 || (remaining.length == 1 &&
                  remaining[0].getFileName().toString().equals(".bluewake-wwhd-import.lock") &&
                  Files.isRegularFile(remaining[0], LinkOption.NOFOLLOW_LINKS) &&
                  Files.size(remaining[0]) == 0), "no staged or published pack for " + target);
        }
    }

    private static void reservation() {
        long id = HDImportNative.create();
        check(id > 0 && HDImportNative.create() == 0, "one reservation");
        HDImportNative.cancel(id);
        status(HDImportNative.run(id, null, null, null, false, null, null), "cancelled");
        check(HDImportNative.create() == 0, "completed run remains reserved until release");
        status(run(id, "folder", null, false, "pre-cancel", null), "failed");
        HDImportNative.release(id);
        HDImportNative.release(id);
        long next = HDImportNative.create();
        check(next > id, "handle is not reused");
        HDImportNative.cancel(id);
        HDImportNative.release(id);
        status(run(id, "folder", null, false, "stale", null), "failed");
        check(HDImportNative.create() == 0, "stale calls keep new reservation");
        HDImportNative.release(next); // Cancelled document preparation, before run.
    }

    private static void invalidPaths() throws Exception {
        byte[][] invalid = {null, new byte[0], "relative".getBytes(StandardCharsets.UTF_8),
            new byte[] {'/', 0}, new byte[] {'/', (byte)0xc0, (byte)0xaf},
            new byte[] {'/', (byte)0xed, (byte)0xa0, (byte)0x80},
            new byte[] {'/', (byte)0xf4, (byte)0x90, (byte)0x80, (byte)0x80},
            new byte[] {'/', (byte)0xf0, (byte)0x9f}, new byte[4097]};
        for (byte[] value : invalid) {
            long id = HDImportNative.create();
            try {
                status(HDImportNative.run(id, value, null, null, false, path(output("invalid")), null), "failed");
            } finally { HDImportNative.release(id); }
        }
        for (int field = 0; field < 3; ++field) {
            long id = HDImportNative.create();
            byte[] bad = new byte[] {'/', (byte)0xff};
            try {
                status(HDImportNative.run(id, path(input("toy.wux")), field == 0 ? bad : path(input("disc.key")),
                    field == 1 ? bad : path(input("title.key")), true,
                    field == 2 ? bad : path(output("invalid")), null), "failed");
            } finally { HDImportNative.release(id); }
        }
        long id = HDImportNative.create();
        try { status(run(id, "toy.wux", null, false, "invalid", null), "failed"); }
        finally { HDImportNative.release(id); }
        id = HDImportNative.create();
        try { status(run(id, "folder", "common.key", false, "invalid", null), "failed"); }
        finally { HDImportNative.release(id); }
        empty("invalid");
    }

    private static void failureMessages() throws Exception {
        long id = HDImportNative.create();
        try { status(run(id, "toy.wux", "wrong.key", true, "wrong-key", null), "failed"); }
        finally { HDImportNative.release(id); }
        empty("wrong-key");
        Path blocked = output("blocked-é-船-🌊");
        Files.write(blocked, new byte[] {1});
        id = HDImportNative.create();
        try {
            String[] result = run(id, "folder", null, false, blocked.getFileName().toString(), null);
            status(result, "failed");
            check(result[3].contains(blocked.toString()), "UTF-8 error converts to Java Unicode exactly");
            check(Arrays.equals(Files.readAllBytes(blocked), new byte[] {1}), "existing output file preserved");
        } finally { HDImportNative.release(id); }
        Files.delete(blocked);
    }

    private static void concurrent() throws Exception {
        final long id = HDImportNative.create();
        CountDownLatch entered = new CountDownLatch(1), resume = new CountDownLatch(1);
        AtomicReference<String[]> result = new AtomicReference<>();
        AtomicReference<Throwable> error = new AtomicReference<>();
        Thread worker = new Thread(() -> {
            try {
                result.set(run(id, "folder", null, false, "concurrent", (read, total, replacements, stage) -> {
                    entered.countDown();
                    try {
                        if (!resume.await(30, TimeUnit.SECONDS)) throw new AssertionError("test barrier timeout");
                    } catch (InterruptedException ex) { throw new AssertionError(ex); }
                }));
            } catch (Throwable ex) { error.set(ex); }
        });
        worker.start();
        try {
            check(entered.await(30, TimeUnit.SECONDS), "worker reached progress barrier");
            check(HDImportNative.create() == 0, "running reservation excludes second import");
            status(run(id, "folder", null, false, "concurrent-duplicate", null), "failed");
            boolean rejected = false;
            try { HDImportNative.release(id); } catch (IllegalStateException expected) { rejected = true; }
            check(rejected && HDImportNative.create() == 0, "premature release retains live token");
            HDImportNative.cancel(id);
        } finally {
            resume.countDown();
            worker.join(30000);
            check(!worker.isAlive(), "worker joined");
            HDImportNative.release(id);
        }
        check(error.get() == null, "no JNI exception on worker");
        status(result.get(), "cancelled");
        empty("concurrent");
        empty("concurrent-duplicate");
    }

    private static void cancellation() throws Exception {
        for (String stage : new String[] {"Reading the GameCube disc", "Finishing the pack"}) {
            final long id = HDImportNative.create();
            String name = stage.startsWith("Reading") ? "cancel-gc" : "cancel-finishing";
            final boolean[] hit = {false};
            try {
                status(run(id, "folder", null, false, name, (read, total, replacements, at) -> {
                    if (at.equals(stage)) { hit[0] = true; HDImportNative.cancel(id); }
                }), "cancelled");
                check(hit[0], "cancel callback reached " + stage);
            } finally { HDImportNative.release(id); }
            empty(name);
        }
        long id = HDImportNative.create();
        try {
            String[] result = run(id, "folder", null, false, "callback-failed", (read, total, replacements, stage) -> {
                throw new IllegalArgumentException("generated callback failure");
            });
            status(result, "failed");
            check(result[3].equals("The import progress callback failed."), "callback failure is contained");
        } finally { HDImportNative.release(id); }
        empty("callback-failed");
    }

    private static void success(String source, String key, boolean title, String target, int lateAction) throws Exception {
        final long id = HDImportNative.create();
        final long[] last = {-1, -1, -1};
        final boolean[] done = {false};
        String[] result;
        try {
            // An old reservation must not cancel this run.
            HDImportNative.cancel(id - 1);
            result = run(id, source, key, title, target, (read, total, replacements, stage) -> {
                if (read < last[0] || total < last[1] || replacements < last[2] || read > total)
                    throw new AssertionError("progress counters");
                last[0] = read; last[1] = total; last[2] = replacements;
                if (stage.equals("Done")) {
                    done[0] = true;
                    if (lateAction == 1) HDImportNative.cancel(id);
                    if (lateAction == 2) throw new IllegalArgumentException("late callback failure");
                }
            });
            status(result, "success");
            check(done[0] && last[2] == 1, "terminal progress");
            check(result[2].equals("1"), "one replacement");
            check(Paths.get(result[1]).getParent().equals(output(target)), "Unicode output path round trips exactly");
            check(Files.isRegularFile(Paths.get(result[1]).resolve("import-report.json")), "published output exists");
            check(HDImportNative.create() == 0, "success still reserved");
            status(run(id, source, key, title, target, null), "failed");
        } finally { HDImportNative.release(id); }
    }

    public static void main(String[] args) throws Exception {
        System.load(args[0]);
        root = Paths.get(args[1]);
        if (args.length > 2 && args[2].equals("missing-gc")) {
            long id = HDImportNative.create();
            try {
                String[] result = run(id, "folder", null, false, "missing-gc", null);
                status(result, "failed");
                check(result[3].contains("GameCube disc"), "missing GameCube input diagnostic");
            } finally { HDImportNative.release(id); }
            empty("missing-gc");
        } else {
            reservation();
            invalidPaths();
            failureMessages();
            concurrent();
            cancellation();
            success("folder", null, false, "folder", 0);
            success("folder", null, false, "folder", 0); // Preserve WWHD and publish WWHD-2.
            for (String ext : new String[] {"wud", "wux"})
                for (boolean title : new boolean[] {false, true})
                    success("toy." + ext, title ? "title.key" : "common.key", title,
                            ext + (title ? "-title" : "-common"), 0);
            success("folder", null, false, "late-cancel", 1);
            success("folder", null, false, "late-callback", 2);
        }
        long available = HDImportNative.create();
        check(available > 0, "session available after all cases");
        HDImportNative.release(available);
        System.out.println("JNI PASS: " + assertions + " assertions; " + root.getFileName());
    }
}
