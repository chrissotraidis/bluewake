// Random access to raw WUD and mapped WUX0 images without expanding them.
#pragma once

#include "wwhd.h"

#include <fstream>

namespace wwhd {

// Owns one file stream; calls on an instance must not run concurrently.
// Non-WUX input is raw bytes: title validation and decryption belong above
// this layer. An empty raw file is allowed, but a WUX must have a sector map.
class DiscImage {
  public:
    explicit DiscImage(const std::string& path);

    uint64_t size() const { return size_; }
    // At most kMaxFile bytes per call; a zero-length read at EOF is valid.
    Bytes read(uint64_t at, uint64_t size);

  private:
    void read_exact(uint64_t at, uint8_t* data, size_t size);

    std::ifstream file_;
    uint64_t physical_size_ = 0;
    uint64_t size_ = 0;
    uint64_t base_ = 0;
    std::vector<uint32_t> table_;
};

} // namespace wwhd
