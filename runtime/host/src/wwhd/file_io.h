#pragma once

#include "cancel.h"
#include "wwhd.h"

#include <algorithm>
#include <istream>

namespace wwhd {
constexpr size_t kIoChunk = 64u * 1024u;

inline void read_exact(std::istream& file, uint8_t* data, size_t size, const char* error) {
    check_cancel();
    for (size_t done = 0; done < size;) {
        WWHD_CHECKPOINT("read-block");
        const size_t count = std::min(size - done, kIoChunk);
        if (!file.read(reinterpret_cast<char*>(data + done), static_cast<std::streamsize>(count)))
            throw FormatError(error);
        done += count;
    }
    check_cancel();
}
} // namespace wwhd
