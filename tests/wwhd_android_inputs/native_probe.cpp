// Read only generated provider bytes, through the same filesystem checks and
// DiscImage implementation used by import_pack. No game, keys, or UI.
#include "wwhd/disc_image.h"
#include <filesystem>
#include <jni.h>

extern "C" JNIEXPORT jbyteArray JNICALL
Java_dev_bluewake_android_HDImportInputsTest_nativeRead(JNIEnv* env, jclass, jstring input) {
    const char* raw = env->GetStringUTFChars(input, nullptr);
    if (!raw) return nullptr;
    const std::string path(raw);
    env->ReleaseStringUTFChars(input, raw);
    try {
        const std::filesystem::path file(path);
        if (!std::filesystem::is_regular_file(file) || std::filesystem::file_size(file) == 0 ||
            (file.extension() != ".wud" && file.extension() != ".wux"))
            throw std::runtime_error("native image path checks failed");
        wwhd::DiscImage image(path);
        if (image.size() > 4 * 1024 * 1024) throw std::runtime_error("fixture too large");
        // End first, then start: prove seek/random access, not only streaming.
        const size_t split = static_cast<size_t>(image.size() / 2);
        const auto end = image.read(split, image.size() - split);
        const auto start = image.read(0, split);
        jbyteArray result = env->NewByteArray(static_cast<jsize>(image.size()));
        if (!result) return nullptr;
        env->SetByteArrayRegion(result, 0, static_cast<jsize>(start.size()), reinterpret_cast<const jbyte*>(start.data()));
        env->SetByteArrayRegion(result, static_cast<jsize>(split), static_cast<jsize>(end.size()), reinterpret_cast<const jbyte*>(end.data()));
        return result;
    } catch (const std::exception& error) {
        env->ThrowNew(env->FindClass("java/io/IOException"), error.what());
        return nullptr;
    }
}
