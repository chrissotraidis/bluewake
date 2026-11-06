package dev.bluewake.android;

/** The synchronous shared importer in libmain.so. No paths or keys are persisted here. */
public final class HDImportNative {
    private HDImportNative() {}

    /** Called on run's worker thread. Dispatch UI updates without blocking this callback. */
    public interface Progress {
        void update(long read, long total, long replacements, String stage);
    }

    /** Reserve the one process-wide import, including document preparation; 0 means busy. */
    public static native long create();

    /** Safe from any thread, including Progress; stale handles are ignored. */
    public static native void cancel(long session);

    /**
     * Call once per session, off the UI thread, after libmain.so is loaded.
     * Paths are absolute local paths encoded with StandardCharsets.UTF_8, not content URIs.
     * For a folder both key paths are null/empty; for an image both are required.
     * titleKey selects a decrypted title key instead of a common key. The GameCube disc
     * comes from BLUEWAKE_DISC. Progress may be null. Keep all grants through this call.
     * Returns {success/cancelled/failed, outputAbsolutePath, countDecimal, message};
     * output is empty and count is zero on failure/cancellation. A published pack wins
     * a late cancel or callback failure. A callback failure before publication cancels
     * native work and returns failed after cleanup. Stale/duplicate/concurrent runs fail.
     */
    public static native String[] run(long session, byte[] sourceUtf8, byte[] discKeyUtf8,
                                      byte[] contentKeyUtf8, boolean titleKey, byte[] texturesDirUtf8,
                                      Progress callback);

    /**
     * Release in finally after run returns, or after abandoning preparation before run.
     * A running session throws IllegalStateException and remains reserved; stale or
     * duplicate releases are harmless. Cancellation alone does not free the session.
     */
    public static native void release(long session);
}
