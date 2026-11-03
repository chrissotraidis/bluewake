// SHA-256 for the built-in Wind Waker HD importer (sha256.cpp).
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

namespace wwhd {

class Sha256 {
  public:
    Sha256();
    void update(const void* data, size_t size);
    // The digest in lower-case hex; the object is finished afterwards.
    std::string hex();

  private:
    void block(const uint8_t* p);
    uint32_t state_[8];
    uint8_t buffer_[64];
    size_t used_ = 0;
    uint64_t length_ = 0;
};

} // namespace wwhd
