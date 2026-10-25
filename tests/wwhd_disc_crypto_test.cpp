// Public NIST CBC vector and generated key-file cases. No console keys.
#include "wwhd/disc_crypto.h"
#include "wwhd/filesystem.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
namespace {
using wwhd::Bytes;
void check(bool ok, const char *why) {
  if (!ok)
    throw std::runtime_error(why);
}
Bytes hex(const char *text) {
  Bytes out;
  for (size_t i = 0; text[i]; i += 2) {
    unsigned byte = 0;
    check(std::sscanf(text + i, "%2x", &byte) == 1, "bad test vector");
    out.push_back(static_cast<uint8_t>(byte));
  }
  return out;
}
wwhd::DiscKey key(const char *text) {
  const Bytes b = hex(text);
  wwhd::DiscKey k{};
  check(b.size() == k.size(), "bad test key");
  std::copy(b.begin(), b.end(), k.begin());
  return k;
}
template <class F> void rejects(F action) {
  try {
    action();
  } catch (const wwhd::FormatError &) {
    return;
  }
  throw std::runtime_error("invalid input was accepted");
}
struct Fixture {
  std::filesystem::path dir =
      std::filesystem::temp_directory_path() /
      ("bluewake-crypto-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  Fixture() {
    check(std::filesystem::create_directory(dir), "cannot make fixture");
  }
  ~Fixture() {
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }
  std::string write(const Bytes &b) {
    const auto p = dir / "synthetic.key";
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!b.empty())
      f.write(reinterpret_cast<const char *>(b.data()), b.size());
    f.close();
    check(bool(f), "cannot write fixture");
    return p.string();
  }
};
} // namespace
int main() {
  try {
    const auto k = key("2b7e151628aed2a6abf7158809cf4f3c"),
               iv = key("000102030405060708090a0b0c0d0e0f");
    const Bytes cipher =
        hex("7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b273"
            "bed6b8e3c1743b7116e69e222295163ff1caa1681fac09120eca307586e1a7");
    const Bytes plain =
        hex("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130"
            "c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710");
    check(wwhd::disc_aes_cbc_decrypt(k, iv, cipher) == plain,
          "NIST CBC decode mismatch");
    check(wwhd::disc_aes_cbc_decrypt(k, iv, cipher) == plain,
          "CBC mutated key or IV");
    check(wwhd::disc_aes_cbc_decrypt(k, iv, {}).empty(), "empty CBC input");
    rejects([&] { wwhd::disc_aes_cbc_decrypt(k, iv, Bytes(17)); });
    const auto digest = wwhd::disc_sha1(Bytes{'a', 'b', 'c'});
    check(Bytes(digest.begin(), digest.end()) ==
              hex("a9993e364706816aba3e25717850c26c9cd0d89d"),
          "SHA-1 abc mismatch");
    const auto empty = wwhd::disc_sha1({});
    check(Bytes(empty.begin(), empty.end()) ==
              hex("da39a3ee5e6b4b0d3255bfef95601890afd80709"),
          "SHA-1 empty mismatch");
    Fixture fixture;
    const std::vector<wwhd::DiscKey> one{k};
    check(wwhd::read_disc_keys(fixture.write(Bytes(k.begin(), k.end()))) == one,
          "binary key mismatch");
    const std::string text = "  2B7E151628AED2A6ABF7158809CF4F3C\r\n";
    check(wwhd::read_disc_keys(fixture.write(Bytes(text.begin(), text.end()))) ==
              one,
          "hex key mismatch");
    for (const Bytes &invalid :
         {Bytes{}, Bytes(15), Bytes(17), Bytes(32, 'g'), Bytes(1025, '0'),
          Bytes(1024 * 1024 + 1, '0')})
      rejects([&] { wwhd::read_disc_keys(fixture.write(invalid)); });
    rejects([&] { wwhd::read_disc_keys((fixture.dir / "absent").string()); });
    // A keys file as the emulators write it: one key per line, '#' comments,
    // blank lines and "name = key" lines. Both keys have to come back, in
    // order, so the image can pick the one that fits.
    const auto second = key("0f0e0d0c0b0a09080706050403020100");
    const std::string keys_file =
        "# keys needed for decryption of file system data (WUD/WUX)\n"
        "# 1 key per line, any text after a '#' character is a comment\n"
        "\n"
        "Common = 2b7e151628aed2a6abf7158809cf4f3c # the common key\n"
        "0f0e0d0c0b0a09080706050403020100\n"
        "not a key\n";
    const std::string path =
        fixture.write(Bytes(keys_file.begin(), keys_file.end()));
    check(wwhd::read_disc_keys(path) ==
              (std::vector<wwhd::DiscKey>{k, second}),
          "keys file mismatch");
    check(wwhd::key_candidates(path) ==
              (std::vector<wwhd::DiscKey>{k, second}),
          "key file path mismatch");
    check(wwhd::key_candidates("0F0E0D0C0B0A09080706050403020100") ==
              (std::vector<wwhd::DiscKey>{second}),
          "pasted key mismatch");
    check(wwhd::key_candidates("not a key").empty(),
          "pasted non-key accepted");
    std::puts("Disc AES, SHA-1 and synthetic key-file checks passed");
    return 0;
  } catch (const std::exception &e) {
    std::fprintf(stderr, "%s\n", e.what());
    return 1;
  }
}
