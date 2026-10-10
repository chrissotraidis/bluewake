package dev.bluewake.android;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;

import java.io.File;
import java.io.FileNotFoundException;

/**
 * Hands one session log to the app the player shares it with (Help & Feedback > Share Session Log), read-only and
 * only through the permission the share grants: the provider is not exported, and it serves nothing but
 * logs/session-*.log from BlueWake's own folder.
 */
public final class LogProvider extends ContentProvider {
    static boolean isSessionLog(String name) { return name != null && name.matches("session-[0-9-]+\\.log"); }

    static Uri uriFor(Context context, File log) {
        return new Uri.Builder().scheme("content").authority(context.getPackageName() + ".logs")
                .appendPath(log.getName()).build();
    }

    private File fileFor(Uri uri) {
        Context context = getContext();
        String name = uri.getLastPathSegment();
        if (context == null || !isSessionLog(name) || uri.getPathSegments().size() != 1) return null;
        File external = context.getExternalFilesDir(null);
        File data = external != null ? external : context.getFilesDir();
        File log = new File(new File(data, "logs"), name);
        return log.isFile() ? log : null;
    }

    @Override
    public boolean onCreate() { return true; }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] args, String sort) {
        File log = fileFor(uri);
        if (log == null) return null;
        String[] columns = projection != null ? projection
                : new String[] {OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE};
        MatrixCursor cursor = new MatrixCursor(columns, 1);
        Object[] row = new Object[columns.length];
        for (int i = 0; i < columns.length; i++) {
            if (OpenableColumns.DISPLAY_NAME.equals(columns[i])) row[i] = log.getName();
            else if (OpenableColumns.SIZE.equals(columns[i])) row[i] = log.length();
        }
        cursor.addRow(row);
        return cursor;
    }

    @Override
    public String getType(Uri uri) { return "text/plain"; }

    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        if (!"r".equals(mode)) throw new SecurityException("session logs are read-only");
        File log = fileFor(uri);
        if (log == null) throw new FileNotFoundException(uri.toString());
        return ParcelFileDescriptor.open(log, ParcelFileDescriptor.MODE_READ_ONLY);
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) { throw new UnsupportedOperationException(); }

    @Override
    public int delete(Uri uri, String selection, String[] args) { throw new UnsupportedOperationException(); }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] args) {
        throw new UnsupportedOperationException();
    }
}
