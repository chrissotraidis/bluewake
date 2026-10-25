// The built-in Wind Waker HD importer's main loop (pack.h): the port of
// build() and source_textures() in scripts/import_wwhd_textures.py, with the
// same matching, priorities and conflicts, so the pack holds the same images.
#include "pack.h"
#include "file_io.h"
#include "filesystem.h"
#include "output_staging.h"

#include "hd.h"
#include "imaging.h"
#include "sha256.h"
#include "ui.h"
#include "wiiu_disc.h"

#include <memory>

#include <zlib.h>

#define XXH_INLINE_ALL
#include "xxhash.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <exception>
#include <fstream>
#include <map>
#include <set>

namespace wwhd {
namespace fs = std::filesystem;

namespace {

bool ends_with(const std::string& s, const char* suffix) {
    const size_t n = std::strlen(suffix);
    return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
}

bool contains(const std::string& s, const char* part) { return s.find(part) != std::string::npos; }

std::string parent_of(const std::string& path) {
    const size_t slash = path.rfind('/');
    return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

using Targets = std::map<std::pair<std::string, std::string>, std::vector<const GCTexture*>>;

const std::vector<const GCTexture*>& lookup(const Targets& targets, const std::string& resource, const std::string& name) {
    static const std::vector<const GCTexture*> kNone;
    const auto it = targets.find({resource, name});
    return it == targets.end() ? kNone : it->second;
}

HDTexture rgba_texture(const std::string& source, const std::string& name, const Image& image) {
    HDTexture t;
    t.source = source;
    t.name = name;
    t.width = image.width;
    t.height = image.height;
    t.format = 0x1A;
    t.components = {0, 1, 2, 3};
    t.levels.push_back(image.rgba);
    return t;
}

Image first_image(const HDTexture& t) {
    HDTexture one = t;
    one.levels.resize(1);
    return hd_images(one).front();
}

using Yield = std::function<void(HDTexture&&, std::vector<const GCTexture*>)>;
using Layout = std::map<std::string, Bytes>;

// source_textures(): the HD textures in one archive member, with the GameCube
// textures each may replace.
void source_textures(const Bytes& data, const std::string& path, const Targets& targets, const Layout* layout,
                     const Yield& yield) {
    const std::string name = base_name(path);
    if (ends_with(path, ".bffnt") && contains(path, "/permanent_2d_UsEnglish.pack/")) {
        struct Binding {
            FontSprite sprite;
            const std::vector<const GCTexture*>* candidates;
        };
        std::vector<Binding> bindings;
        for (const FontSprite& s : font_sprites(stem(path))) {
            const auto& c = lookup(targets, s.resource, s.name);
            if (!c.empty())
                bindings.push_back({s, &c});
        }
        if (bindings.empty())
            return;
        BitmapFont font(data);
        for (const Binding& b : bindings) {
            Image image;
            if (b.sprite.text.empty()) {
                const auto m = font.mapping.find(uint32_t(b.sprite.glyph));
                if (m == font.mapping.end())
                    throw FormatError("font lacks requested glyph");
                image = font.glyph(m->second);
            } else {
                image = font.render(b.sprite.text);
            }
            for (const GCTexture* gc : *b.candidates) {
                const Image adapted = fit_sprite(image, *gc);
                yield(rgba_texture(path + "#GameCube-sprite:" + b.sprite.name, b.sprite.name, adapted), {gc});
            }
        }
        return;
    }
    if (contains(path, "/Opening_00.szs/timg/")) {
        bool any = false;
        for (const OpeningPanel& p : kOpeningPanels)
            any = any || name == p.picture;
        if (any) {
            const Image image = first_image(bflim(data, path));
            for (const OpeningPanel& p : kOpeningPanels) {
                if (name != p.picture)
                    continue;
                for (const char* res : {"object|opening", "object|open1", "object|open2", "object|open3"})
                    for (const GCTexture* gc : lookup(targets, res, p.name)) {
                        const Image adapted = opening_sprite(image, p.box, *gc);
                        yield(rgba_texture(path + "#GameCube-panel:" + p.name, p.name, adapted), {gc});
                    }
            }
            return;
        }
    }
    if (ends_with(path, ".bflim")) {
        std::vector<const LayoutSprite*> binding;
        for (const LayoutSprite& s : kLayoutSprites)
            if (contains(path, ("/" + std::string(s.archive) + ".szs/timg/" + s.filename).c_str()))
                binding.push_back(&s);
        // Python takes the first (archive, filename) that matches; each has one entry.
        if (!binding.empty()) {
            const Image image = first_image(bflim(data, path));
            const LayoutSprite* first = binding.front();
            for (const LayoutSprite& s : kLayoutSprites) {
                if (std::strcmp(s.archive, first->archive) != 0 || std::strcmp(s.filename, first->filename) != 0)
                    continue;
                for (const GCTexture* gc : lookup(targets, s.resource, s.name)) {
                    const Image adapted = fit_sprite(image, *gc);
                    yield(rgba_texture(path + "#GameCube-sprite:" + s.name, s.name, adapted), {gc});
                }
            }
            return;
        }
    }
    if (data.size() >= 4 && std::memcmp(data.data(), "FRES", 4) == 0) {
        const std::string identity = resource(path);
        std::set<std::string> names;
        for (const auto& [key, list] : targets)
            if (key.first == identity)
                names.insert(key.second);
        bfres(data, path, &names, [&](HDTexture&& hd) {
            const auto& c = lookup(targets, identity, lower(hd.name));
            yield(std::move(hd), c);
        });
        return;
    }
    if (ends_with(path, ".bflim")) {
        const std::vector<std::string> names = inventory_names(path);
        if (!names.empty()) {
            HDTexture hd = bflim(data, path);
            hd.name = names[0];
            std::vector<const GCTexture*> c;
            for (const std::string& n : names)
                for (const GCTexture* gc : lookup(targets, "object|itemicon", n))
                    c.push_back(gc);
            yield(std::move(hd), c);
            return;
        }
    }
    if (contains(path, "/SwimTime_00.szs/timg/")) {
        if (const char* swim = swim_icon(name)) {
            HDTexture hd = bflim(data, path);
            hd.name = swim;
            yield(std::move(hd), lookup(targets, "object|swimres", swim));
        }
        return;
    }
    if (layout != nullptr && contains(path, "/Heart_00.szs/timg/")) {
        if (const char* heart = heart_name(name)) {
            const auto base_data = layout->find("HeartBase_00^t.bflim");
            if (base_data == layout->end())
                throw FormatError("missing heart outline");
            const Image base = first_image(bflim(base_data->second, path + "#outline"));
            const Image fill = first_image(bflim(data, path));
            yield(rgba_texture(path + "#GameCube-layout", heart, heart_image(base, fill)),
                  lookup(targets, "object|menures", heart));
        }
        return;
    }
    if (layout != nullptr && contains(path, "/Title_00.szs/timg/")) {
        const char* logo = name == "TitleLogoZelda_00^l.bflim"       ? "logo_zelda_main"
                           : name == "TitleLogoWindwaker_00^l.bflim" ? "logo_sub_e"
                                                                     : nullptr;
        if (logo == nullptr)
            return;
        const Image image = first_image(bflim(data, path));
        std::optional<Image> badge;
        if (std::strcmp(logo, "logo_sub_e") == 0) {
            const auto b = layout->find("TitleLogoHD_00^l.bflim");
            if (b == layout->end())
                throw FormatError("missing HD badge");
            badge = first_image(bflim(b->second, path + "#HD-badge"));
        }
        std::vector<const GCTexture*> candidates;
        for (const char* res : {"object|tlogo", "object|tlogoe"})
            for (const GCTexture* gc : lookup(targets, res, logo))
                candidates.push_back(gc);
        std::set<std::pair<int, int>> sizes;
        for (const GCTexture* gc : candidates)
            sizes.insert({gc->width, gc->height});
        for (const auto& [w, h] : sizes) {
            const Image adapted = title_image(w * 4, h * 4, image, badge ? &*badge : nullptr);
            std::vector<const GCTexture*> same;
            for (const GCTexture* gc : candidates)
                if (gc->width == w && gc->height == h)
                    same.push_back(gc);
            yield(rgba_texture(path + "#GameCube-layout", logo, adapted), same);
        }
    }
}

bool compatible(const GCTexture& gc, const HDTexture& hd) {
    const std::string name = lower(gc.name);
    if (std::min(gc.width, gc.height) < 16 || name.rfind("zbtoon", 0) == 0 || name.rfind("toon", 0) == 0)
        return false;
    const std::string res = resource(gc.source);
    if (res == "object|always" &&
        (name.rfind("camera_", 0) == 0 || name.rfind("map_frame", 0) == 0 || name.rfind("map_check", 0) == 0))
        return false;
    if (contains(lower(hd.source), ".bfres#") && (res == "object|tlogo" || res == "object|tlogoe") &&
        (name == "logo_zelda_main" || name == "logo_zelda_jpa" || name == "logo_sub"))
        return false;
    return hd.width >= gc.width && hd.height >= gc.height &&
           int64_t(hd.width) * gc.height == int64_t(hd.height) * gc.width;
}

struct WriteError : std::runtime_error { using std::runtime_error::runtime_error; };

void write_file(const fs::path& path, const Bytes& data) {
    check_cancel();
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
        throw WriteError("cannot write " + path.string());
    for (size_t done = 0; done < data.size();) {
        check_cancel();
        const size_t count = std::min(kIoChunk, data.size() - done);
        if (!out.write(reinterpret_cast<const char*>(data.data() + done), std::streamsize(count)))
            throw WriteError("cannot write " + path.string());
        done += count;
    }
    WWHD_CHECKPOINT("write-before-close");
    // Buffered writes can fail only on close (for example, a full volume).
    out.close();
    if (!out)
        throw WriteError("cannot finish writing " + path.string());
}

// Encodes and writes PNGs on a few threads. Queued images are capped by size,
// so a phone does not hold the whole pack in memory. drain() waits for every
// queued write (before a file may be removed, and at the end).
class PngWriter {
  public:
    explicit PngWriter(unsigned threads) : cancel_(import_cancel) {
        try {
            for (unsigned i = 0; i < threads; i++)
                workers_.emplace_back([this] { work(); });
        } catch (...) {
            stop();
            join();
            throw;
        }
    }
    ~PngWriter() {
        stop();
        join();
    }
    void add(const fs::path& path, const Image& image) {
        check_cancel();
        if (workers_.empty()) {
            write_file(path, encode_png(image));
            return;
        }
        std::unique_lock<std::mutex> hold(lock_);
        // Include the next image in the budget. One oversized image may run
        // alone so a large texture cannot deadlock the producer.
        while (!stop_ && queued_bytes_ != 0 &&
               (image.rgba.size() > kMaxQueuedBytes || queued_bytes_ > kMaxQueuedBytes - image.rgba.size())) {
            WWHD_CHECKPOINT("png-room-wait");
            check();
#ifdef WWHD_IMPORT_TESTING
            import_test_checkpoint("png-waiting");
#endif
            room_.wait_for(hold, std::chrono::milliseconds(25));
        }
        check();
        queued_bytes_ += image.rgba.size();
        queue_.push_back({path, image});
        hold.unlock();
        wake_.notify_one();
        WWHD_CHECKPOINT("png-queued");
    }
    void drain() {
        std::unique_lock<std::mutex> hold(lock_);
        while ((!queue_.empty() || busy_ != 0) && !stop_) {
            WWHD_CHECKPOINT("png-drain-wait");
            check();
            // An external atomic token cannot notify the condition variable.
#ifdef WWHD_IMPORT_TESTING
            import_test_checkpoint("png-waiting");
#endif
            room_.wait_for(hold, std::chrono::milliseconds(25));
        }
        check();
    }

  private:
    static constexpr size_t kMaxQueuedBytes = 32u << 20;
    void clear_queue() {
        for (const auto& job : queue_)
            queued_bytes_ -= job.second.rgba.size();
        queue_.clear();
    }
    void stop() {
        {
            std::lock_guard<std::mutex> hold(lock_);
            stop_ = true;
            clear_queue();
        }
#ifdef WWHD_IMPORT_TESTING
        import_test_checkpoint("png-stopping");
#endif
        wake_.notify_all();
        room_.notify_all();
    }
    void join() {
        for (std::thread& t : workers_)
            t.join();
    }
    void check() {
        check_cancel();
        if (cancelled_)
            throw Cancelled();
        if (!error_.empty())
            throw WriteError(error_);
    }
    void work() {
        CancellationScope scope(cancel_);
        for (;;) {
            std::pair<fs::path, Image> job;
            {
                std::unique_lock<std::mutex> hold(lock_);
                wake_.wait(hold, [&] { return !queue_.empty() || stop_; });
                if (stop_)
                    return;
                job = std::move(queue_.front());
                queue_.pop_front();
                busy_++;
            }
            std::string failure;
            bool cancelled = false;
            try {
                WWHD_CHECKPOINT("png-before-encode");
                write_file(job.first, encode_png(job.second));
            } catch (const Cancelled&) {
                cancelled = true;
            } catch (const std::exception& ex) {
                failure = ex.what();
            }
            {
                std::lock_guard<std::mutex> hold(lock_);
                busy_--;
                queued_bytes_ -= job.second.rgba.size();
                if (cancelled || !failure.empty()) {
                    cancelled_ = cancelled_ || cancelled;
                    stop_ = true;
                    clear_queue();
                }
                if (!failure.empty() && error_.empty())
                    error_ = failure;
            }
            room_.notify_all();
            wake_.notify_all();
        }
    }
    std::mutex lock_;
    std::condition_variable wake_, room_;
    std::deque<std::pair<fs::path, Image>> queue_;
    size_t queued_bytes_ = 0;
    size_t busy_ = 0;
    bool stop_ = false;
    bool cancelled_ = false;
    const std::atomic<bool>* cancel_;
    std::string error_;
    std::vector<std::thread> workers_;
};

std::string digest_of(const std::vector<Image>& images, bool exact) {
    if (exact) {
        Sha256 sha;
        for (const Image& im : images)
            sha.update(im.rgba.data(), im.rgba.size());
        return sha.hex();
    }
    std::unique_ptr<XXH3_state_t, decltype(&XXH3_freeState)> state(XXH3_createState(), XXH3_freeState);
    if (!state)
        throw std::bad_alloc();
    XXH3_128bits_reset(state.get());
    for (const Image& im : images)
        for (size_t at = 0; at < im.rgba.size(); at += kIoChunk) {
            check_cancel();
            XXH3_128bits_update(state.get(), im.rgba.data() + at, std::min(kIoChunk, im.rgba.size() - at));
        }
    const XXH128_hash_t h = XXH3_128bits_digest(state.get());
    char hex[33];
    std::snprintf(hex, sizeof hex, "%016llx%016llx", (unsigned long long)h.high64, (unsigned long long)h.low64);
    return hex;
}

std::string json_string(const std::string& s) {
    std::string out = "\"";
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += char(c);
        } else if (c < 0x20) {
            char esc[8];
            std::snprintf(esc, sizeof esc, "\\u%04x", c);
            out += esc;
        } else {
            out += char(c);
        }
    }
    return out + "\"";
}

struct Match {
    std::string file, hd;
    std::vector<std::string> originals, alternatives;
    int priority = 0;
    int original_w = 0, original_h = 0, hd_w = 0, hd_h = 0;
    size_t mipmaps = 0;
    std::string image_sha256;
    std::vector<std::string> generated;
    std::vector<std::string> generated_sha256;
};

} // namespace

Bytes encode_png(const Image& image) {
    const auto put32 = [](Bytes& b, uint32_t v) {
        for (int i = 3; i >= 0; i--)
            b.push_back(uint8_t(v >> (8 * i)));
    };
    const auto chunk = [&](Bytes& out, const char* type, const Bytes& body) {
        put32(out, uint32_t(body.size()));
        const size_t start = out.size();
        out.insert(out.end(), type, type + 4);
        out.insert(out.end(), body.begin(), body.end());
        put32(out, uint32_t(crc32(0, out.data() + start, uInt(out.size() - start))));
    };
    // Feed the same unfiltered scanlines to zlib a row at a time. Each PNG
    // worker used to hold a second full image plus a compressBound-sized
    // buffer, even when the compressed result was small.
    struct Deflater {
        z_stream stream{};
        Deflater() {
            if (deflateInit(&stream, 6) != Z_OK)
                throw FormatError("PNG compression failed");
        }
        ~Deflater() { deflateEnd(&stream); }
    } compressor;
    Bytes packed;
    std::array<uint8_t, 64 * 1024> buffer;
    const auto deflate_bytes = [&](const uint8_t* data, size_t size, int flush) {
        z_stream& z = compressor.stream;
        z.next_in = const_cast<Bytef*>(data);
        z.avail_in = uInt(size);
        int status;
        do {
            check_cancel();
            z.next_out = buffer.data();
            z.avail_out = uInt(buffer.size());
            status = deflate(&z, flush);
            if (status != Z_OK && status != Z_STREAM_END)
                throw FormatError("PNG compression failed");
            packed.insert(packed.end(), buffer.data(), buffer.data() + buffer.size() - z.avail_out);
        } while (z.avail_in || (flush == Z_FINISH && status != Z_STREAM_END));
    };
    const uint8_t filter = 0;
    for (int y = 0; y < image.height; y++) {
        WWHD_CHECKPOINT("png-row");
        deflate_bytes(&filter, 1, Z_NO_FLUSH);
        deflate_bytes(image.at(0, y), size_t(image.width) * 4, Z_NO_FLUSH);
    }
    deflate_bytes(nullptr, 0, Z_FINISH);
    Bytes out = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    Bytes ihdr;
    put32(ihdr, uint32_t(image.width));
    put32(ihdr, uint32_t(image.height));
    ihdr.insert(ihdr.end(), {8, 6, 0, 0, 0}); // 8-bit RGBA
    chunk(out, "IHDR", ihdr);
    chunk(out, "IDAT", packed);
    chunk(out, "IEND", {});
    return out;
}

ImportResult import_pack(const ImportOptions& o) {
    CancellationScope scope(o.cancel);
    WWHD_CHECKPOINT("import-start");
    ImportResult result;
    ImportProgress progress;
    const auto report = [&](const std::string& stage) {
        progress.stage = stage;
        if (o.progress)
            o.progress(progress);
    };
    const fs::path output = fs::absolute(fs::path(o.output));
    std::error_code ec;
    OutputStaging temporary(output); // Lock/recovery stays on the import worker.

    report("Opening Wind Waker HD");
    check_cancel();
    std::unique_ptr<WiiUDisc> disc;
    std::map<std::string, const WiiUDiscEntry*> disc_entries;
    std::vector<std::string> files;
    const fs::path input(o.hd_folder);
    if (fs::is_regular_file(input, ec)) {
        const std::string ext = lower(input.extension().string());
        if (ext != ".wux" && ext != ".wud" && ext != ".iso")
            throw FormatError("HD input must be a WUD/WUX disc or a decrypted content folder");
        WiiUDiscKeys keys;
        // Each key field is a key file or a key the player pasted; a keys file
        // holds every title's key, so the image picks its own from the list.
        const auto keys_from = [](const std::string& source, const char* role) {
            std::vector<DiscKey> keys = key_candidates(source);
            if (keys.empty())
                throw FormatError(std::string("the ") + role + " is neither a key file nor a 32-digit hex key");
            return keys;
        };
        const std::string disc_source =
            o.disc_key_file.empty() ? (input.parent_path() / "game.key").string() : o.disc_key_file;
        keys.disc = keys_from(disc_source, "disc key");
        if (!o.title_key_file.empty()) {
            keys.title = keys_from(o.title_key_file, "title key");
        } else {
            const fs::path common = o.common_key_file.empty() ? input.parent_path() / "common.key" : fs::path(o.common_key_file);
            if (!o.common_key_file.empty() || fs::is_regular_file(common, ec))
                keys.common = keys_from(common.string(), "common key");
        }
        disc = std::make_unique<WiiUDisc>(o.hd_folder, keys);
        for (const WiiUDiscEntry& entry : disc->entries()) {
            check_cancel();
            const std::string name = lower(entry.path);
            if (entry.path.rfind("content/Cafe/", 0) != 0 &&
                (ends_with(name, ".szs") || ends_with(name, ".sarc") || ends_with(name, ".pack") || ends_with(name, ".bfres"))) {
                files.push_back(entry.path);
                disc_entries.emplace(entry.path, &entry);
            }
        }
    } else {
        files = hd_files(o.hd_folder);
    }
    progress.files_total = files.size();
    if (files.empty())
        throw FormatError("no Wind Waker HD resources in the selected input");
    check_cancel();
    report("Reading the GameCube disc");
    check_cancel();
    const std::vector<GCTexture> originals = index_gc(o.gc_disc, result.errors);
    Targets targets;
    for (const GCTexture& t : originals) {
        check_cancel();
        targets[{resource(t.source), lower(t.name)}].push_back(&t);
    }

    check_cancel();
    const fs::path staging = temporary.create();

    std::map<std::string, Match> matches;
    std::set<std::string> conflicts;
    try {
        // Declared in the try block, so an error or a cancel stops its threads
        // before the staging folder is removed.
        const unsigned cores = std::thread::hardware_concurrency();
        const unsigned threads = o.threads ? o.threads : std::max(1u, std::min(4u, cores > 1 ? cores - 1 : 1u));
        PngWriter writer(o.exact_report ? 0 : threads);
        for (const std::string& source : files) {
            check_cancel();
            progress.files_read++;
            if (progress.files_read % 20 == 0) {
                progress.replacements = matches.size() - conflicts.size();
                report("Reading Wind Waker HD");
            }
            const Bytes archive = disc ? disc->read(*disc_entries.at(source)) : read_file(o.hd_folder + "/" + source);
            try {
                // Keep only the two inputs used by another layout member.
                // Visit the members again in archive order, releasing each
                // before the next instead of retaining the whole English pack.
                const bool keep = ends_with(source, "/Title_00.szs") || ends_with(source, "_2d_UsEnglish.pack");
                std::map<std::string, Layout> layouts;
                if (keep) {
                    leaves(archive, source, [&](const std::string& path, const Bytes& data) {
                        if (ends_with(path, ".bflim")) {
                            // An empty layout still marks the same folders as
                            // before, including a malformed one missing a part.
                            Layout& layout = layouts[parent_of(path)];
                            const std::string name = base_name(path);
                            if (name == "HeartBase_00^t.bflim" || name == "TitleLogoHD_00^l.bflim")
                                layout[name] = data;
                        }
                    });
                }
                const auto handle = [&](const std::string& path, const Bytes& data) {
                    check_cancel();
                    const auto l = layouts.find(parent_of(path));
                    const Layout* layout = keep && l != layouts.end() ? &l->second : nullptr;
                    source_textures(data, path, targets, layout, [&](HDTexture&& hd, std::vector<const GCTexture*> candidates) {
                        check_cancel();
                        std::vector<const GCTexture*> gc;
                        for (const GCTexture* t : candidates)
                            if (compatible(*t, hd))
                                gc.push_back(t);
                        if (gc.empty())
                            return;
                        try {
                            std::vector<Image> images = hd_images(hd);
                            // A complete generated mip chain prevents shimmering
                            // when the source archive stops above 1x1.
                            complete_mips(images);
                            const bool mask = contains(hd.source, ".bflim") || contains(hd.source, ".bffnt");
                            // Only I4/I8 need different pixels, and both use
                            // the same intensity image. Other formats borrow
                            // the decoded chain instead of copying it per format.
                            std::vector<Image> intensity;
                            std::map<bool, std::string> digests;
                            for (const GCTexture* t : gc)
                                if (const bool is_intensity = t->format == 0 || t->format == 1;
                                    !digests.count(is_intensity)) {
                                    if (is_intensity)
                                        intensity = for_gc_format(images, t->format, mask);
                                    digests[is_intensity] = digest_of(is_intensity ? intensity : images, o.exact_report);
                                }
                            const int priority = candidate_priority(hd.name, resource(hd.source));
                            for (const GCTexture* original : gc) {
                                check_cancel();
                                const bool is_intensity = original->format == 0 || original->format == 1;
                                const auto& variant = is_intensity ? intensity : images;
                                const std::string& digest = digests[is_intensity];
                                const std::string key = original->filename();
                                std::vector<std::string> originals_for_key = {original->source}, alternatives;
                                const auto previous = matches.find(key);
                                if (previous != matches.end()) {
                                    Match& p = previous->second;
                                    if (priority < p.priority) {
                                        p.alternatives.push_back(hd.source);
                                        p.originals.push_back(original->source);
                                        continue;
                                    }
                                    if (priority == p.priority) {
                                        if (p.image_sha256 != digest) {
                                            conflicts.insert(key);
                                            p.alternatives.push_back(hd.source);
                                        } else {
                                            p.originals.push_back(original->source);
                                        }
                                        continue;
                                    }
                                    // A later audited match can resolve a prior
                                    // disagreement between unpreferred variants.
                                    originals_for_key.insert(originals_for_key.end(), p.originals.begin(), p.originals.end());
                                    alternatives = p.alternatives;
                                    alternatives.push_back(p.hd);
                                    writer.drain();
                                    WWHD_CHECKPOINT("remove-replaced");
                                    for (const std::string& name : p.generated) {
                                        fs::remove(staging / name, ec);
                                        if (ec)
                                            throw WriteError("cannot remove generated texture " + name + ": " + ec.message());
                                    }
                                    conflicts.erase(key);
                                }
                                Match m;
                                for (size_t i = 0; i < variant.size(); i++) {
                                    check_cancel();
                                    const std::string name =
                                        i == 0 ? key : key.substr(0, key.size() - 4) + "_mip" + std::to_string(i) + ".png";
                                    if (o.exact_report) {
                                        const Bytes png = encode_png(variant[i]);
                                        write_file(staging / name, png);
                                        Sha256 file_sha;
                                        file_sha.update(png.data(), png.size());
                                        m.generated_sha256.push_back(file_sha.hex());
                                    } else {
                                        writer.add(staging / name, variant[i]);
                                    }
                                    m.generated.push_back(name);
                                }
                                m.file = key;
                                m.hd = hd.source;
                                m.originals = originals_for_key;
                                m.priority = priority;
                                m.alternatives = alternatives;
                                m.original_w = original->width;
                                m.original_h = original->height;
                                m.hd_w = hd.width;
                                m.hd_h = hd.height;
                                m.mipmaps = images.size();
                                m.image_sha256 = digest;
                                matches[key] = std::move(m);
                            }
                        } catch (const FormatError& ex) {
                            result.errors.push_back({hd.source, ex.what()});
                        }
                    });
                };
                leaves(archive, source, handle);
            } catch (const FormatError& ex) {
                result.errors.push_back({source, ex.what()});
            }
        }
        writer.drain();
        check_cancel();
        report("Finishing the pack");
        check_cancel();
        for (const std::string& key : conflicts) {
            check_cancel();
            WWHD_CHECKPOINT("remove-conflict");
            for (const std::string& name : matches[key].generated) {
                fs::remove(staging / name, ec);
                if (ec)
                    throw WriteError("cannot remove generated texture " + name + ": " + ec.message());
            }
        }
        std::string textures;
        size_t accepted = 0;
        for (auto& [key, m] : matches) {
            check_cancel();
            if (conflicts.count(key))
                continue;
            accepted++;
            std::set<std::string> alts(m.alternatives.begin(), m.alternatives.end());
            std::set<std::string> origs(m.originals.begin(), m.originals.end());
            std::string list;
            const auto join = [&](const std::set<std::string>& items) {
                std::string out = "[";
                for (const auto& s : items)
                    out += (out.size() > 1 ? ", " : "") + json_string(s);
                return out + "]";
            };
            std::string files_sha = "{";
            for (size_t i = 0; i < m.generated_sha256.size() && i < m.generated.size(); i++)
                files_sha += (i ? ", " : "") + json_string(m.generated[i]) + ": " + json_string(m.generated_sha256[i]);
            files_sha += "}";
            textures += std::string(textures.empty() ? "" : ",\n") + "    {\"file\": " + json_string(m.file) +
                        ", \"hd\": " + json_string(m.hd) + ", \"originals\": " + join(origs) +
                        ", \"alternatives\": " + join(alts) + ", \"original_size\": [" + std::to_string(m.original_w) +
                        ", " + std::to_string(m.original_h) + "], \"hd_size\": [" + std::to_string(m.hd_w) + ", " +
                        std::to_string(m.hd_h) + "], \"mipmaps\": " + std::to_string(m.mipmaps) +
                        ", \"image_sha256\": " + json_string(m.image_sha256) + ", \"files_sha256\": " + files_sha +
                        (m.priority ? ", \"selection\": \"audited shared-artwork variant\"" : "") + "}";
        }
        if (!accepted)
            throw FormatError("no compatible textures matched; nothing installed");
        result.replacements = accepted;
        result.conflicts = conflicts.size();
        std::string errors;
        for (const ImportError& e : result.errors) {
            check_cancel();
            errors += std::string(errors.empty() ? "" : ",\n") + "    {\"source\": " + json_string(e.source) +
                      ", \"reason\": " + json_string(e.reason) + "}";
        }
        const std::time_t now = std::time(nullptr);
        char created[32];
        std::strftime(created, sizeof created, "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
        const std::string json = std::string("{\n  \"format\": 1,\n  \"name\": \"Wind Waker HD (local disc import)\",\n") +
                                 "  \"tool\": \"BlueWake built-in importer\",\n  \"created_utc\": \"" + created + "\",\n" +
                                 "  \"replacement_count\": " + std::to_string(accepted) + ",\n" +
                                 "  \"conflicts_skipped\": " + std::to_string(conflicts.size()) + ",\n" +
                                 "  \"unsupported\": [\n" + errors + "\n  ],\n" + "  \"textures\": [\n" + textures + "\n  ]\n}\n";
        write_file(staging / "import-report.json", Bytes(json.begin(), json.end()));
        const std::string readme = "Private artwork extracted from your own Wind Waker HD disc.\n"
                                   "Choose this folder in Mods > Wind Waker HD textures, then restart.\n"
                                   "Do not distribute this folder with app builds or source releases.\n";
        write_file(staging / "README.txt", Bytes(readme.begin(), readme.end()));
        // Publication is the commit point; cancellation after it is success.
        WWHD_CHECKPOINT("publish");
        temporary.publish();
    } catch (...) {
        const auto failure = std::current_exception();
#ifdef WWHD_IMPORT_TESTING
        import_test_checkpoint("staging-cleanup");
#endif
        try { temporary.discard(); }
        catch (const std::exception& cleanup) {
            try { std::rethrow_exception(failure); }
            catch (const std::exception& original) {
                throw WriteError(std::string(original.what()) + "; temporary output retained: " + cleanup.what());
            }
        }
        std::rethrow_exception(failure);
    }
    progress.replacements = result.replacements;
    report("Done");
    return result;
}

} // namespace wwhd
