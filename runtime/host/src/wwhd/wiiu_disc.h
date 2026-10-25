// Reads the player-provided USA Wind Waker HD WUD/WUX without extracting it.
#pragma once
#include "disc_image.h"
#include "disc_crypto.h"
#include <memory>
#include <vector>
namespace wwhd {
struct WiiUDiscKeys {
    // A keys file holds every title's key, and a WUD/WUX does not name its own,
    // so each role is a list of candidates: the image keeps the one that fits.
    std::vector<DiscKey> disc;   // one of these decrypts the partition table
    std::vector<DiscKey> title;  // one of these is the disc's title key
    std::vector<DiscKey> common; // otherwise one of these decrypts its ticket
};
struct WiiUDiscEntry {
    std::string path;
    uint64_t offset = 0, size = 0;
    uint16_t cluster = 0, flags = 0;
};
class WiiUDisc {
  public:
    WiiUDisc(const std::string& path, const WiiUDiscKeys& keys);
    ~WiiUDisc();
    const std::vector<WiiUDiscEntry>& entries() const;
    Bytes read(const WiiUDiscEntry& entry);
  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace wwhd
