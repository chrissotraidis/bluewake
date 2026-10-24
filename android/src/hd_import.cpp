// Synchronous JNI boundary for the shared importer. Java owns the worker and
// document grants; this reservation owns the token until all PNG workers join.
#include "wwhd/import_job.h"
#include "wwhd/filesystem.h"

#include <jni.h>
#include <cstdlib>
#include <limits>
#include <memory>

namespace {
namespace fs = std::filesystem;

struct Session {
    enum State { Reserved, Running, Finished } state = Reserved;
    jlong id;
    std::atomic<bool> cancelled{false};
    explicit Session(jlong value) : id(value) {}
};
std::mutex session_lock;
std::shared_ptr<Session> active;
jlong last_id = 0;

struct FinishRun {
    std::shared_ptr<Session> session;
    ~FinishRun() {
        std::lock_guard<std::mutex> hold(session_lock);
        session->state = Session::Finished;
    }
};

struct JavaException {};

// JNI's NewStringUTF/GetStringUTFChars use modified UTF-8 (including six-byte
// surrogate pairs). Filesystem paths use standard UTF-8, including emoji.
std::vector<jchar> utf16(const std::string& text, bool strict = false) {
    std::vector<jchar> out;
    out.reserve(text.size());
    for (size_t at = 0; at < text.size();) {
        const auto first = static_cast<unsigned char>(text[at]);
        const size_t length = first < 0x80 ? 1 : first >= 0xc2 && first <= 0xdf ? 2 :
                              first >= 0xe0 && first <= 0xef ? 3 : first >= 0xf0 && first <= 0xf4 ? 4 : 0;
        uint32_t cp = length == 1 ? first : first & (0x7f >> length);
        bool valid = length != 0 && length <= text.size() - at;
        for (size_t i = 1; valid && i < length; ++i) {
            const auto byte = static_cast<unsigned char>(text[at + i]);
            valid = (byte & 0xc0) == 0x80;
            cp = (cp << 6) | (byte & 0x3f);
        }
        valid = valid && !(length == 2 && cp < 0x80) && !(length == 3 && cp < 0x800) &&
                !(length == 4 && cp < 0x10000) && cp <= 0x10ffff && !(cp >= 0xd800 && cp <= 0xdfff);
        if (!valid) {
            if (strict)
                throw wwhd::FormatError("A selected path is not valid UTF-8.");
            cp = 0xfffd; // An OS error may contain invalid filename bytes.
        }
        at += valid ? length : 1;
        if (cp > 0xffff) {
            cp -= 0x10000;
            out.push_back(static_cast<jchar>(0xd800 + (cp >> 10)));
            out.push_back(static_cast<jchar>(0xdc00 + (cp & 0x3ff)));
        } else {
            out.push_back(static_cast<jchar>(cp));
        }
    }
    return out;
}

jstring java_text(JNIEnv* env, const std::string& text) {
    const auto chars = utf16(text);
    const jchar empty = 0;
    const auto value = env->NewString(chars.empty() ? &empty : chars.data(), static_cast<jsize>(chars.size()));
    if (!value)
        throw JavaException{};
    return value;
}

void java_error(JNIEnv* env, const char* type, const char* ascii_message) {
    if (env->ExceptionCheck())
        return;
    const auto cls = env->FindClass(type);
    if (cls) {
        env->ThrowNew(cls, ascii_message);
        env->DeleteLocalRef(cls);
    }
}

std::string path_bytes(JNIEnv* env, jbyteArray bytes, bool required) {
    const jsize length = bytes ? env->GetArrayLength(bytes) : 0;
    if (length > 4096 || (required && length == 0))
        throw wwhd::FormatError("A selected path is empty or too long.");
    std::string path(static_cast<size_t>(length), '\0');
    if (length)
        env->GetByteArrayRegion(bytes, 0, length, reinterpret_cast<jbyte*>(path.data()));
    if (env->ExceptionCheck())
        throw JavaException{};
    if (path.find('\0') != std::string::npos)
        throw wwhd::FormatError("A selected path contains a null byte.");
    utf16(path, true);
    if (!path.empty() && !fs::path(path).is_absolute())
        throw wwhd::FormatError("Selected files must have absolute local paths.");
    return path;
}

jobjectArray result(JNIEnv* env, const std::string& status, const std::string& output = {},
                    size_t count = 0, const std::string& message = {}) {
    const auto cls = env->FindClass("java/lang/String");
    if (!cls)
        throw JavaException{};
    const auto values = env->NewObjectArray(4, cls, nullptr);
    env->DeleteLocalRef(cls);
    if (!values)
        throw JavaException{};
    const std::string fields[] = {status, output, std::to_string(count), message};
    for (jsize i = 0; i < 4; ++i) {
        const auto value = java_text(env, fields[i]);
        env->SetObjectArrayElement(values, i, value);
        env->DeleteLocalRef(value);
        if (env->ExceptionCheck())
            throw JavaException{};
    }
    return values;
}

jobjectArray run(JNIEnv* env, jlong id, jbyteArray source, jbyteArray disc_key, jbyteArray content_key,
                 jboolean title_key, jbyteArray textures, jobject callback) {
    std::shared_ptr<Session> session;
    {
        std::lock_guard<std::mutex> hold(session_lock);
        if (active && active->id == id && active->state == Session::Reserved) {
            session = active;
            session->state = Session::Running;
        }
    }
    if (!session)
        return result(env, "failed", {}, 0, "This import session is stale or has already run.");
    FinishRun finish{session};
    bool callback_failed = false;
    try {
        if (session->cancelled.load(std::memory_order_relaxed))
            throw wwhd::Cancelled();
        wwhd::ImportOptions options;
        options.hd_folder = wwhd::hd_content_folder(path_bytes(env, source, true));
        options.disc_key_file = path_bytes(env, disc_key, false);
        const auto content = path_bytes(env, content_key, false);
        const auto base = path_bytes(env, textures, true);
        std::error_code ec;
        if (fs::is_directory(options.hd_folder, ec)) {
            if (!options.disc_key_file.empty() || !content.empty())
                throw wwhd::FormatError("Folder imports do not use key files.");
        } else if (options.disc_key_file.empty() || content.empty()) {
            throw wwhd::FormatError("Choose a disc key and either a common key or a title key for the image.");
        }
        if (title_key)
            options.title_key_file = content;
        else
            options.common_key_file = content;
        const char* gc = std::getenv("BLUEWAKE_DISC");
        if (!gc || !*gc)
            throw wwhd::FormatError("BlueWake could not find the GameCube disc it plays, which the import also reads.");
        options.gc_disc = gc;
        options.output = wwhd::new_pack_folder(base);
        options.cancel = &session->cancelled;
        options.threads = 2;
        if (callback) {
            const auto cls = env->GetObjectClass(callback);
            if (!cls)
                throw JavaException{};
            const auto update = env->GetMethodID(cls, "update", "(JJJLjava/lang/String;)V");
            env->DeleteLocalRef(cls);
            if (!update)
                throw JavaException{};
            // import_pack reports on this calling thread, never a PNG worker.
            // A callback failure requests cleanup but cannot throw past a
            // completed rename ("Done" is reported after publication).
            options.progress = [&, update](const wwhd::ImportProgress& p) noexcept {
                if (callback_failed)
                    return;
                try {
                    const auto stage = java_text(env, p.stage);
                    env->CallVoidMethod(callback, update, static_cast<jlong>(p.files_read),
                                        static_cast<jlong>(p.files_total), static_cast<jlong>(p.replacements), stage);
                    const bool failed = env->ExceptionCheck();
                    env->DeleteLocalRef(stage);
                    if (!failed)
                        return;
                } catch (...) {
                }
                env->ExceptionClear();
                callback_failed = true;
                session->cancelled.store(true, std::memory_order_relaxed);
            };
        }
        const auto imported = wwhd::import_pack(options);
        return result(env, "success", options.output, imported.replacements,
                      "Imported " + std::to_string(imported.replacements) + " textures from Wind Waker HD.");
    } catch (const wwhd::Cancelled&) {
        return result(env, callback_failed ? "failed" : "cancelled", {}, 0,
                      callback_failed ? "The import progress callback failed." : "The import was cancelled.");
    } catch (const std::exception& ex) {
        return result(env, "failed", {}, 0, std::string("The import failed: ") + ex.what());
    }
}
} // namespace

extern "C" JNIEXPORT jlong JNICALL Java_dev_bluewake_android_HDImportNative_create(JNIEnv* env, jclass) {
    try {
        std::lock_guard<std::mutex> hold(session_lock);
        if (active)
            return 0;
        if (last_id == std::numeric_limits<jlong>::max())
            throw std::runtime_error("session IDs exhausted");
        active = std::make_shared<Session>(++last_id);
        return active->id;
    } catch (...) {
        java_error(env, "java/lang/IllegalStateException", "Could not reserve an HD import session.");
        return 0;
    }
}

extern "C" JNIEXPORT void JNICALL Java_dev_bluewake_android_HDImportNative_cancel(JNIEnv* env, jclass, jlong id) {
    try {
        std::lock_guard<std::mutex> hold(session_lock);
        if (active && active->id == id)
            active->cancelled.store(true, std::memory_order_relaxed);
    } catch (...) {
        java_error(env, "java/lang/IllegalStateException", "Could not cancel the HD import session.");
    }
}

extern "C" JNIEXPORT jobjectArray JNICALL Java_dev_bluewake_android_HDImportNative_run(
    JNIEnv* env, jclass, jlong id, jbyteArray source, jbyteArray disc_key, jbyteArray content_key,
    jboolean title_key, jbyteArray textures, jobject callback) {
    try {
        return run(env, id, source, disc_key, content_key, title_key, textures, callback);
    } catch (const JavaException&) {
        return nullptr; // Pending JVM exception; native work has already unwound.
    } catch (...) {
        java_error(env, "java/lang/RuntimeException", "Could not return the HD import result.");
        return nullptr;
    }
}

extern "C" JNIEXPORT void JNICALL Java_dev_bluewake_android_HDImportNative_release(JNIEnv* env, jclass, jlong id) {
    try {
        std::lock_guard<std::mutex> hold(session_lock);
        if (!active || active->id != id)
            return;
        if (active->state == Session::Running) {
            java_error(env, "java/lang/IllegalStateException", "Wait for HDImportNative.run to return before release.");
            return;
        }
        active.reset();
    } catch (...) {
        java_error(env, "java/lang/IllegalStateException", "Could not release the HD import session.");
    }
}
