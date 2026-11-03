// Local-disc AES-128-CBC and SHA-1 helpers, using the pinned Mbed TLS sources.
#pragma once
#include "wwhd.h"
#include <array>
#include <string>
#include <vector>
namespace wwhd {
using DiscKey = std::array<uint8_t, 16>;
using DiscSha1 = std::array<uint8_t, 20>;

// Every key in a local key file: 16 binary bytes, one key as 32 hexadecimal
// digits, or a keys file with one key per line (blank lines, "#" comments and
// "disc_key = <hex>" forms, as the emulators' keys.txt uses). A Wii U image
// does not say which of them is its key, so the caller gets them all and the
// image decides. Throws FormatError when the file holds no key.
std::vector<DiscKey> read_disc_keys(const std::string &path);
// The same for text a player pasted into the import form.
std::vector<DiscKey> parse_disc_keys(const std::string &text);
// Either source, as the menu collects it: the file's keys when source names a
// readable file, otherwise the text's. Empty when it is neither, so the menu
// can check a field without throwing.
std::vector<DiscKey> key_candidates(const std::string &source);

Bytes disc_aes_cbc_decrypt(const DiscKey &key, const DiscKey &iv,
                           const Bytes &encrypted);
DiscSha1 disc_sha1(const Bytes &bytes);
} // namespace wwhd
