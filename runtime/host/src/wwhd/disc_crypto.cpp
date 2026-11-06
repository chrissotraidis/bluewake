#include "disc_crypto.h"
#include "cancel.h"

#include <mbedtls/aes.h>
#include <mbedtls/sha1.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace wwhd {
namespace {
// A keys file holds hundreds of keys; a disc image or a mistyped pick does not.
constexpr size_t kMaxKeyFile = 1024 * 1024;
constexpr size_t kMaxKeys = 4096;

int hex_digit(unsigned char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

// One line of a keys file or pasted text: an optional "name =", then the key,
// then an optional "# comment". False when the line holds no single key.
bool key_on_line(std::string line, DiscKey &key) {
  const size_t comment = line.find('#');
  if (comment != std::string::npos)
    line.resize(comment);
  const size_t equals = line.find('=');
  if (equals != std::string::npos)
    line.erase(0, equals + 1);
  std::string digits;
  for (const unsigned char c : line)
    if (!std::isspace(c))
      digits.push_back(static_cast<char>(c));
  if (digits.size() != key.size() * 2)
    return false;
  for (size_t i = 0; i < key.size(); ++i) {
    const int hi = hex_digit(digits[2 * i]), lo = hex_digit(digits[2 * i + 1]);
    if (hi < 0 || lo < 0)
      return false;
    key[i] = static_cast<uint8_t>((hi << 4) | lo);
  }
  return true;
}

std::vector<DiscKey> keys_in(const std::string &text) {
  std::vector<DiscKey> keys;
  if (text.size() == DiscKey{}.size()) {
    DiscKey key{};
    std::copy(text.begin(), text.end(), key.begin());
    keys.push_back(key);
    return keys;
  }
  size_t at = 0;
  while (at <= text.size() && keys.size() < kMaxKeys) {
    check_cancel();
    const size_t end = text.find('\n', at);
    DiscKey key{};
    if (key_on_line(text.substr(at, end == std::string::npos ? std::string::npos : end - at), key))
      keys.push_back(key);
    if (end == std::string::npos)
      break;
    at = end + 1;
  }
  return keys;
}
} // namespace

std::vector<DiscKey> read_disc_keys(const std::string &path) {
  check_cancel();
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file)
    throw FormatError("could not open the selected key file");
  const std::streamoff length = file.tellg();
  if (length <= 0)
    throw FormatError("the selected key file is empty");
  if (static_cast<size_t>(length) > kMaxKeyFile)
    throw FormatError("key files must be under 1 MiB");
  std::string text(static_cast<size_t>(length), '\0');
  file.seekg(0);
  file.read(text.data(), static_cast<std::streamsize>(text.size()));
  if (!file)
    throw FormatError("could not read the selected key file");
  check_cancel();
  std::vector<DiscKey> keys = keys_in(text);
  if (keys.empty())
    throw FormatError("this key file holds no 32-digit hex key");
  return keys;
}

std::vector<DiscKey> parse_disc_keys(const std::string &text) { return keys_in(text); }

std::vector<DiscKey> key_candidates(const std::string &source) {
  std::error_code ec;
  if (!source.empty() && std::filesystem::is_regular_file(source, ec))
    return read_disc_keys(source);
  return parse_disc_keys(source);
}

Bytes disc_aes_cbc_decrypt(const DiscKey &key, const DiscKey &iv,
                           const Bytes &encrypted) {
  check_cancel();
  if (encrypted.size() > kMaxFile || encrypted.size() % 16 != 0)
    throw FormatError("invalid encrypted disc block size");
  if (encrypted.empty())
    return {};
  Bytes plain(encrypted.size());
  DiscKey working_iv = iv;
  mbedtls_aes_context context;
  mbedtls_aes_init(&context);
  struct Cleanup {
    mbedtls_aes_context* context;
    ~Cleanup() { mbedtls_aes_free(context); }
  } cleanup{&context};
  int error = mbedtls_aes_setkey_dec(&context, key.data(), 128);
  // CBC updates working_iv, preserving chaining across bounded chunks.
  for (size_t at = 0; error == 0 && at < encrypted.size(); at += 65536) {
    check_cancel();
    error = mbedtls_aes_crypt_cbc(&context, MBEDTLS_AES_DECRYPT,
                                  std::min(size_t{65536}, encrypted.size() - at), working_iv.data(),
                                  encrypted.data() + at, plain.data() + at);
  }
  if (error != 0)
    throw FormatError("could not decrypt the disc block");
  return plain;
}

DiscSha1 disc_sha1(const Bytes &bytes) {
  check_cancel();
  DiscSha1 digest{};
  mbedtls_sha1_context context;
  mbedtls_sha1_init(&context);
  struct Cleanup {
    mbedtls_sha1_context* context;
    ~Cleanup() { mbedtls_sha1_free(context); }
  } cleanup{&context};
  int error = mbedtls_sha1_starts_ret(&context);
  for (size_t at = 0; error == 0 && at < bytes.size(); at += 65536) {
    check_cancel();
    error = mbedtls_sha1_update_ret(&context, bytes.data() + at, std::min(size_t{65536}, bytes.size() - at));
  }
  if (error != 0 || mbedtls_sha1_finish_ret(&context, digest.data()) != 0)
    throw FormatError("could not check the disc block checksum");
  return digest;
}
} // namespace wwhd
