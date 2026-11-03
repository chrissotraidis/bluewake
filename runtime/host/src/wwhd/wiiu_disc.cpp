// Port of scripts/wwhd/disc.py. All keys come from the caller; disc members
// stay in memory. The partition and hashed CBC layouts follow that reader.
#include "wiiu_disc.h"
#include "cancel.h"

#include <algorithm>
#include <map>
#include <set>
#include <utility>

namespace wwhd {
namespace {
constexpr uint64_t kSector = 0x8000;
constexpr uint64_t kBlock = 0x10000;
constexpr uint64_t kHashBytes = 0x400;
constexpr uint64_t kPayload = kBlock - kHashBytes;
constexpr uint64_t kTitleId = 0x0005000010143500;
// FSTs and title metadata are small. Bound both their input and the expanded
// paths independently, so a hostile table cannot multiply memory use by depth.
constexpr size_t kMaxMetadata = 16u * 1024u * 1024u;
constexpr size_t kMaxPath = 4096;

void range(uint64_t at, uint64_t size, uint64_t limit) {
    if (at > limit || size > limit - at)
        throw FormatError("Wii U data range is outside its content or partition");
}

uint64_t u64(const Reader& r, size_t at) {
    return (uint64_t(r.u32(at)) << 32) | r.u32(at + 4);
}

uint64_t align16(uint64_t size) {
    // All callers first bound size to an image extent or kMaxFile.
    return (size + 15) / 16 * 16;
}

void component(const std::string& name) {
    if (name.empty() || name == "." || name == ".." || name.find_first_of("/\\:") != std::string::npos)
        throw FormatError("unsafe Wii U file name");
    for (char byte : name) {
        const auto c = static_cast<unsigned char>(byte);
        if (c < 32 || c == 127)
            throw FormatError("unsafe Wii U file name");
    }
}

struct Region {
    uint64_t start = 0, end = 0;
    uint64_t size() const { return end - start; }
};

struct Cluster {
    uint32_t sector = 0, sectors = 0;
};

struct FileTable {
    std::vector<Cluster> clusters;
    std::vector<WiiUDiscEntry> entries;
};

FileTable file_table(const Bytes& data) {
    const Reader r(data);
    if (data.size() > kMaxMetadata || r.u32(0) != 0x46535400)
        throw FormatError("incorrect title key or invalid Wii U file table");
    const uint32_t scale = r.u32(4), cluster_count = r.u32(8);
    if (!scale || !cluster_count || cluster_count > 65536)
        throw FormatError("invalid Wii U content table");
    r.take(32, size_t(cluster_count) * 32);
    const size_t base = 32 + size_t(cluster_count) * 32;
    const uint32_t count = r.u32(base + 8);
    if (!count || count > kMaxMetadata / 16 || !(r.u8(base) & 1) || r.u32(base + 4) != 0)
        throw FormatError("invalid Wii U root directory");
    r.take(base, size_t(count) * 16);
    const size_t strings = base + size_t(count) * 16;

    FileTable table;
    table.clusters.reserve(cluster_count);
    for (size_t i = 0; i < cluster_count; ++i) {
        check_cancel();
        table.clusters.push_back({r.u32(32 + i * 32), r.u32(36 + i * 32)});
    }

    std::set<std::string> paths;
    size_t path_bytes = 0;
    const auto walk = [&](const auto& self, uint32_t first, uint32_t end, uint32_t parent,
                          const std::string& prefix, unsigned depth) -> void {
        if (depth > 32 || first > end || end > count)
            throw FormatError("invalid Wii U directory tree");
        for (uint32_t i = first; i < end;) {
            WWHD_CHECKPOINT("wiiu-entry");
            const size_t at = base + size_t(i) * 16;
            const uint32_t type_name = r.u32(at), off = r.u32(at + 4), size = r.u32(at + 8);
            const std::string name = r.string(strings + (type_name & 0xFFFFFF));
            component(name);
            if (name.size() > kMaxPath || prefix.size() > kMaxPath - name.size())
                throw FormatError("Wii U file path is too long");
            const std::string path = prefix + name;
            if (path.size() > kMaxMetadata - path_bytes)
                throw FormatError("Wii U file paths exceed size limit");
            path_bytes += path.size();
            // Reject file/directory collisions and case aliases, including in
            // system metadata, rather than selecting an arbitrary title/ticket.
            if (!paths.insert(lower(path)).second)
                throw FormatError("duplicate Wii U file path");
            if ((type_name >> 24) & 1) {
                if (off != parent || size <= i || size > end)
                    throw FormatError("invalid Wii U directory range");
                self(self, i + 1, size, i, path + "/", depth + 1);
                i = size;
            } else {
                const uint16_t cluster = r.u16(at + 14);
                if (cluster >= cluster_count)
                    throw FormatError("invalid Wii U content index");
                table.entries.push_back({path, uint64_t(off) * scale, size, cluster, r.u16(at + 12)});
                ++i;
            }
        }
    };
    walk(walk, 1, count, 0, "", 0);
    return table;
}

struct Partition {
    Region data;
    uint32_t fst_size = 0;
};

struct Content {
    uint16_t type = 0;
    uint64_t size = 0;
    DiscSha1 digest{};
};

// Bytes a content occupies on the disc: a hashed content stores a hash block
// with its payload, and an encrypted one is read in whole AES blocks.
uint64_t physical_bytes(const Content& content) {
    return (content.type & 1) ? align16(content.size) : content.size;
}

Region cluster_region(const Partition& partition, const FileTable& table, uint16_t index, uint64_t bytes = 0) {
    if (index >= table.clusters.size())
        throw FormatError("invalid Wii U content index");
    const Cluster& cluster = table.clusters[index];
    const uint64_t offset = uint64_t(cluster.sector ? cluster.sector - 1 : 0) * kSector;
    range(offset, 0, partition.data.size());
    Region result{partition.data.start + offset, partition.data.end};
    // A zero allocation size falls back to partition bounds; game contents
    // additionally have their TMD size.
    if (cluster.sectors) {
        // A hashed content's allocation counts payload sectors, so on a real
        // disc it falls short of the hash blocks stored with that payload. The
        // title metadata's own size is the authority when it asks for more.
        const uint64_t allocated = std::max(uint64_t(cluster.sectors) * kSector, bytes);
        range(offset, allocated, partition.data.size());
        result.end = result.start + allocated;
    } else if (bytes) {
        range(offset, bytes, partition.data.size());
        result.end = result.start + bytes;
    }
    return result;
}
} // namespace

struct WiiUDisc::Impl {
    DiscImage image;
    DiscKey disc_key{}, title_key{};
    Partition game;
    FileTable files;
    std::map<uint16_t, Content> contents;
    // Sequential extraction frequently reuses the last block for small files.
    // Keep just this verified 63 KiB payload, never a whole decrypted content.
    Bytes cached_block;
    uint16_t cached_cluster = 0;
    uint64_t cached_index = 0;
    uint64_t system_bytes_read = 0;

    Bytes read_region(const Region& region, uint64_t at, uint64_t size) {
        if (region.start > region.end || region.end > image.size())
            throw FormatError("invalid Wii U partition extent");
        range(at, size, region.size());
        return image.read(region.start + at, size);
    }

    Bytes metadata(const Region& region, uint64_t size, const DiscKey& key) {
        if (!size || size > kMaxMetadata)
            throw FormatError("Wii U metadata exceeds size limit");
        Bytes plain = disc_aes_cbc_decrypt(key, DiscKey{}, read_region(region, 0, align16(size)));
        plain.resize(static_cast<size_t>(size));
        return plain;
    }

    Partition partition_header(uint64_t start, uint64_t end) {
        const Bytes bytes = read_region({start, end}, 0, 32);
        const Reader r(bytes);
        if (r.u32(0) != 0xCC93A4F5)
            throw FormatError("invalid Wii U partition header");
        const uint32_t offset = r.u32(4), size = r.u32(20);
        if (offset < 32 || offset % 16 || size < 32 || size > kMaxMetadata)
            throw FormatError("invalid Wii U partition file table");
        range(offset, align16(size), end - start);
        return {{start + offset, end}, size};
    }

    Bytes system_content(const Partition& system, const FileTable& table, const WiiUDiscEntry& entry) {
        if (entry.size > kMaxMetadata)
            throw FormatError("Wii U system metadata exceeds size limit");
        const Region region = cluster_region(system, table, entry.cluster);
        range(entry.offset, entry.size, region.size());
        Bytes out(static_cast<size_t>(entry.size));
        uint64_t at = entry.offset;
        size_t done = 0;
        while (done < out.size()) {
            WWHD_CHECKPOINT("metadata-block");
            // Charge actual block work, even when many tiny metadata entries
            // point at the same block. Their file sizes alone undercount it.
            range(system_bytes_read, kBlock, kMaxFile);
            system_bytes_read += kBlock;
            const uint64_t index = at / kBlock;
            const size_t within = static_cast<size_t>(at % kBlock);
            DiscKey iv{};
            for (unsigned i = 0; i < 8; ++i)
                iv[15 - i] = static_cast<uint8_t>(index >> (i * 8));
            const Bytes plain = disc_aes_cbc_decrypt(disc_key, iv, read_region(region, index * kBlock, kBlock));
            const size_t n = std::min(out.size() - done, static_cast<size_t>(kBlock) - within);
            std::copy_n(Reader(plain).take(within, n), n, out.data() + done);
            at += n;
            done += n;
        }
        return out;
    }

    explicit Impl(const std::string& path, const WiiUDiscKeys& keys) : image(path) {
        const Bytes identity = image.read(0, 64);
        const auto identity_end = std::find(identity.begin(), identity.end(), 0);
        const std::string identifier(identity.begin(), identity_end);
        if (identifier.compare(0, 11, "WUP-P-BCZE-") != 0 ||
            std::any_of(identifier.begin(), identifier.end(), [](unsigned char c) { return c < 32 || c > 126; }))
            throw FormatError("expected Wind Waker HD USA disc (BCZE)");

        // Every key in a keys file is tried: only this disc's own decrypts its
        // partition table, which names the partitions below.
        const Bytes toc_raw = read_region({0x18000, image.size()}, 0, 0x8000);
        Bytes toc_bytes;
        for (const DiscKey& candidate : keys.disc) {
            check_cancel();
            Bytes plain = disc_aes_cbc_decrypt(candidate, DiscKey{}, toc_raw);
            if (Reader(plain).u32(0) == 0xCCA6E67B) {
                disc_key = candidate;
                toc_bytes = std::move(plain);
                break;
            }
        }
        if (toc_bytes.empty()) {
            if (keys.disc.size() > 1)
                throw FormatError("none of the " + std::to_string(keys.disc.size()) +
                                  " keys in the disc key file decrypt this disc");
            throw FormatError("incorrect WUD disc key");
        }
        const Reader toc(toc_bytes);
        const uint32_t count = toc.u32(28);
        if (!count || count > (toc.size() - 0x800) / 0x80)
            throw FormatError("invalid Wii U partition count");
        std::map<std::string, uint64_t> partitions;
        std::set<std::string> names;
        std::set<uint64_t> starts;
        std::string game_name;
        for (size_t i = 0; i < count; ++i) {
            check_cancel();
            const size_t at = 0x800 + i * 0x80;
            const std::string name = Reader(toc.take(at, 32), 32).string(0);
            component(name);
            const uint64_t start = uint64_t(toc.u32(at + 32)) * kSector;
            if (start < 0x20000)
                throw FormatError("invalid Wii U partition offset");
            range(start, 32, image.size());
            if (!names.insert(lower(name)).second || !starts.insert(start).second)
                throw FormatError("duplicate Wii U partition");
            partitions.emplace(name, start);
            if (name.compare(0, 18, "GM0005000010143500") == 0) {
                if (!game_name.empty())
                    throw FormatError("ambiguous Wind Waker HD game partition");
                game_name = name;
            }
        }
        const auto si = partitions.find("SI");
        if (si == partitions.end() || game_name.empty())
            throw FormatError("USA Wind Waker HD game partition not found");
        const auto end_of = [&](uint64_t start) {
            const auto next = starts.upper_bound(start);
            return next == starts.end() ? image.size() : *next;
        };
        const Partition system = partition_header(si->second, end_of(si->second));
        const FileTable system_files = file_table(metadata(system.data, system.fst_size, disc_key));

        Bytes tmd_bytes;
        std::string tmd_path;
        uint64_t scanned = 0;
        for (const WiiUDiscEntry& entry : system_files.entries) {
            check_cancel();
            const std::string path_lower = lower(entry.path);
            constexpr std::string_view suffix = "/title.tmd";
            if (path_lower.size() < suffix.size() ||
                path_lower.compare(path_lower.size() - suffix.size(), suffix.size(), suffix) != 0)
                continue;
            range(scanned, entry.size, kMaxFile);
            scanned += entry.size;
            Bytes raw = system_content(system, system_files, entry);
            if (u64(Reader(raw), 0x18C) != kTitleId)
                continue;
            if (!tmd_bytes.empty())
                throw FormatError("ambiguous Wind Waker HD title metadata");
            tmd_bytes = std::move(raw);
            tmd_path = path_lower;
        }
        if (tmd_bytes.empty())
            throw FormatError("Wind Waker HD title metadata not found");

        // The same for the title: each title key given, or each common key's
        // ticket, is tried until the game file table's checksum matches.
        std::vector<DiscKey> title_keys = keys.title;
        if (title_keys.empty()) {
            if (keys.common.empty())
                throw FormatError("provide the disc title key, or a common key to decrypt its ticket");
            const std::string ticket_path = tmd_path.substr(0, tmd_path.size() - 4) + ".tik";
            const auto ticket_entry = std::find_if(system_files.entries.begin(), system_files.entries.end(),
                [&](const WiiUDiscEntry& entry) { return lower(entry.path) == ticket_path; });
            if (ticket_entry == system_files.entries.end())
                throw FormatError("Wind Waker HD title ticket not found");
            const Bytes ticket_bytes = system_content(system, system_files, *ticket_entry);
            const Reader ticket(ticket_bytes);
            if (u64(ticket, 0x1DC) != kTitleId)
                throw FormatError("Wii U ticket does not match title metadata");
            DiscKey iv{};
            std::copy_n(ticket.take(0x1DC, 8), 8, iv.begin());
            const uint8_t* encrypted = ticket.take(0x1BF, 16);
            for (const DiscKey& common : keys.common) {
                const Bytes key = disc_aes_cbc_decrypt(common, iv, Bytes(encrypted, encrypted + 16));
                DiscKey derived{};
                std::copy_n(Reader(key).take(0, 16), 16, derived.begin());
                if (std::find(title_keys.begin(), title_keys.end(), derived) == title_keys.end())
                    title_keys.push_back(derived);
            }
        }

        const Reader tmd(tmd_bytes);
        const uint16_t content_count = tmd.u16(0x1DE);
        if (!content_count)
            throw FormatError("empty Wii U title content table");
        tmd.take(0xB04, size_t(content_count) * 48);
        std::set<uint32_t> content_ids;
        for (size_t i = 0; i < content_count; ++i) {
            check_cancel();
            const size_t at = 0xB04 + i * 48;
            Content content;
            content.type = tmd.u16(at + 6);
            content.size = u64(tmd, at + 8);
            range(0, content.size, image.size());
            std::copy_n(tmd.take(at + 16, 20), 20, content.digest.begin());
            if (!contents.emplace(tmd.u16(at + 4), content).second || !content_ids.insert(tmd.u32(at)).second)
                throw FormatError("duplicate Wii U title content");
        }
        const auto fst_content = contents.find(0);
        if (fst_content == contents.end() || (fst_content->second.type & 2))
            throw FormatError("Wii U game file table content not found");
        const uint64_t game_start = partitions.at(game_name);
        game = partition_header(game_start, end_of(game_start));
        const Content& fst = fst_content->second;
        if (fst.size < game.fst_size)
            throw FormatError("Wii U game file table size does not match title metadata");
        Bytes raw;
        for (const DiscKey& candidate : title_keys) {
            check_cancel();
            Bytes plain = metadata(game.data, fst.size, candidate);
            if (disc_sha1(plain) == fst.digest) {
                title_key = candidate;
                raw = std::move(plain);
                break;
            }
        }
        if (raw.empty()) {
            if (title_keys.size() > 1)
                throw FormatError("none of the " + std::to_string(title_keys.size()) +
                                  " keys supplied for the title key match this disc");
            throw FormatError("title key or game file table checksum is incorrect");
        }
        raw.resize(game.fst_size);
        files = file_table(raw);
        // Validate every file against its content before exposing the table.
        for (const WiiUDiscEntry& entry : files.entries)
            checked_content(entry);
    }

    const Content& checked_content(const WiiUDiscEntry& entry) const {
        const auto found = contents.find(entry.cluster);
        if (found == contents.end())
            throw FormatError("Wii U file references missing title content");
        const Content& content = found->second;
        const Region region = cluster_region(game, files, entry.cluster, physical_bytes(content));
        uint64_t logical_size = content.size;
        if (content.type & 2) {
            if (content.size % kBlock)
                throw FormatError("invalid Wii U hashed content size");
            logical_size = content.size / kBlock * kPayload;
        }
        range(0, physical_bytes(content), region.size());
        range(entry.offset, entry.size, logical_size);
        return content;
    }

    const Bytes& hashed_block(uint16_t cluster, uint64_t index, const Region& region) {
        if (!cached_block.empty() && cached_cluster == cluster && cached_index == index)
            return cached_block;
        // index is bounded by checked_content's physical-size-to-payload map.
        const Bytes raw = read_region(region, index * kBlock, kBlock);
        const Bytes hashes = disc_aes_cbc_decrypt(title_key, DiscKey{}, Bytes(raw.begin(), raw.begin() + kHashBytes));
        DiscSha1 h0{};
        std::copy_n(Reader(hashes).take(static_cast<size_t>(index % 16) * 20, 20), 20, h0.begin());
        DiscKey iv{};
        std::copy_n(h0.begin(), iv.size(), iv.begin());
        Bytes plain = disc_aes_cbc_decrypt(title_key, iv, Bytes(raw.begin() + kHashBytes, raw.end()));
        if (disc_sha1(plain) != h0)
            throw FormatError("Wii U content block checksum failed");
        cached_block = std::move(plain);
        cached_cluster = cluster;
        cached_index = index;
        return cached_block;
    }

    Bytes read(const WiiUDiscEntry& entry) {
        check_cancel();
        if (entry.size > kMaxFile)
            throw FormatError("disc file is too large");
        const Content& content = checked_content(entry);
        const Region region = cluster_region(game, files, entry.cluster, physical_bytes(content));
        if (!(content.type & 3))
            return read_region(region, entry.offset, entry.size);

        Bytes out(static_cast<size_t>(entry.size));
        if (out.empty())
            return out;
        uint64_t at = entry.offset;
        size_t done = 0;
        if (content.type & 2) {
            while (done < out.size()) {
                WWHD_CHECKPOINT("content-block");
                const Bytes& plain = hashed_block(entry.cluster, at / kPayload, region);
                const size_t within = static_cast<size_t>(at % kPayload);
                const size_t n = std::min(out.size() - done, static_cast<size_t>(kPayload) - within);
                std::copy_n(Reader(plain).take(within, n), n, out.data() + done);
                at += n;
                done += n;
            }
        } else {
            DiscKey iv{};
            const uint64_t aligned = at / 16 * 16;
            if (aligned) {
                const Bytes previous = read_region(region, aligned - 16, 16);
                std::copy(previous.begin(), previous.end(), iv.begin());
            } else {
                iv[0] = static_cast<uint8_t>(entry.cluster >> 8);
                iv[1] = static_cast<uint8_t>(entry.cluster);
            }
            while (done < out.size()) {
                WWHD_CHECKPOINT("content-block");
                const size_t within = static_cast<size_t>(at % 16);
                const size_t n = std::min(out.size() - done, static_cast<size_t>(kBlock) - within);
                const Bytes raw = read_region(region, at - within, align16(within + n));
                const Bytes plain = disc_aes_cbc_decrypt(title_key, iv, raw);
                std::copy_n(Reader(plain).take(within, n), n, out.data() + done);
                std::copy(raw.end() - 16, raw.end(), iv.begin());
                at += n;
                done += n;
            }
        }
        return out;
    }
};

WiiUDisc::WiiUDisc(const std::string& path, const WiiUDiscKeys& keys) : impl_(std::make_unique<Impl>(path, keys)) {}
WiiUDisc::~WiiUDisc() = default;
const std::vector<WiiUDiscEntry>& WiiUDisc::entries() const { return impl_->files.entries; }
Bytes WiiUDisc::read(const WiiUDiscEntry& entry) { return impl_->read(entry); }

} // namespace wwhd
