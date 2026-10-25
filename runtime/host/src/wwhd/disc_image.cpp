#include "disc_image.h"
#include "file_io.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace wwhd {
namespace {
constexpr uint64_t kSectorSize = 0x8000;
constexpr uint64_t kMaxDiscSize = uint64_t{64} * 1024 * 1024 * 1024;
constexpr uint64_t kHeaderSize = 32;

uint32_t le32(const uint8_t* data) {
    return uint32_t(data[0]) | (uint32_t(data[1]) << 8) | (uint32_t(data[2]) << 16) | (uint32_t(data[3]) << 24);
}

uint64_t le64(const uint8_t* data) {
    return uint64_t(le32(data)) | (uint64_t(le32(data + 4)) << 32);
}
} // namespace

DiscImage::DiscImage(const std::string& path) {
    check_cancel();
    file_.open(path, std::ios::binary | std::ios::ate);
    if (!file_)
        throw FormatError("cannot open disc image");
    const std::streamoff end = file_.tellg();
    if (end < 0)
        throw FormatError("cannot determine disc image size");
    physical_size_ = size_ = static_cast<uint64_t>(end);

    std::array<uint8_t, kHeaderSize> header{};
    const auto header_size = static_cast<size_t>(std::min(physical_size_, kHeaderSize));
    read_exact(0, header.data(), header_size);
    // Reject unknown WUX versions instead of silently interpreting them as raw.
    if (header_size >= 3 && std::memcmp(header.data(), "WUX", 3) == 0) {
        if (header_size != kHeaderSize || std::memcmp(header.data(), "WUX0", 4) != 0 ||
            le32(header.data() + 4) != 0x1099D02E)
            throw FormatError("invalid WUX header");
        size_ = le64(header.data() + 16);
        if (le32(header.data() + 8) != kSectorSize || le32(header.data() + 12) != 0 || size_ == 0 ||
            size_ > kMaxDiscSize)
            throw FormatError("unsupported WUX layout");

        // The logical size cap bounds this to 2^21 entries (8 MiB) and makes
        // the additions, multiplication and alignment below safe in uint64_t.
        const uint64_t sectors = (size_ + kSectorSize - 1) / kSectorSize;
        const uint64_t map_size = sectors * 4;
        if (map_size > physical_size_ - kHeaderSize)
            throw FormatError("truncated WUX sector map");
        base_ = ((kHeaderSize + map_size + kSectorSize - 1) / kSectorSize) * kSectorSize;
        if (base_ > physical_size_)
            throw FormatError("truncated WUX sector data");

        Bytes map(static_cast<size_t>(map_size));
        read_exact(kHeaderSize, map.data(), map.size());
        table_.reserve(static_cast<size_t>(sectors));
        const uint64_t physical_sectors = (physical_size_ - base_) / kSectorSize;
        for (size_t i = 0; i < map.size(); i += 4) {
            if (i % kIoChunk == 0)
                check_cancel();
            const uint32_t sector = le32(map.data() + i);
            // A complete physical sector is required even for a partial last
            // logical sector, matching scripts/wwhd/disc.py.
            if (sector >= physical_sectors)
                throw FormatError("WUX sector map references missing or truncated data");
            table_.push_back(sector);
        }
    } else if (size_ > kMaxDiscSize) {
        throw FormatError("disc image exceeds size limit");
    }
}

void DiscImage::read_exact(uint64_t at, uint8_t* data, size_t size) {
    if (size > kMaxFile || at > physical_size_ || size > physical_size_ - at)
        throw FormatError("physical disc read outside image");
    if (size == 0)
        return;
    // at is bounded by tellg()'s nonnegative streamoff; size by kMaxFile.
    file_.clear();
    file_.seekg(static_cast<std::streamoff>(at));
    wwhd::read_exact(file_, data, size, "disc data is missing or truncated");
}

Bytes DiscImage::read(uint64_t at, uint64_t size) {
    check_cancel();
    // Subtraction avoids wrapping at + size for hostile caller-supplied values.
    if (size > kMaxFile || at > size_ || size > size_ - at)
        throw FormatError("disc read outside image");
    Bytes result(static_cast<size_t>(size));
    size_t done = 0;
    while (done < result.size()) {
        WWHD_CHECKPOINT("disc-block");
        uint64_t physical = at;
        size_t count = std::min(kIoChunk, result.size() - done);
        if (!table_.empty()) {
            const uint64_t within = at % kSectorSize;
            physical = base_ + uint64_t(table_[static_cast<size_t>(at / kSectorSize)]) * kSectorSize + within;
            count = static_cast<size_t>(std::min<uint64_t>(count, kSectorSize - within));
        }
        read_exact(physical, result.data() + done, count);
        at += count;
        done += count;
    }
    return result;
}

} // namespace wwhd
