#pragma once

#include <array>
#include <openssl/sha.h>
#include <span>
#include <string>

inline constexpr auto hash_djb2a(const std::string_view sv) {
  unsigned long hash{5381};
  for (unsigned char c: sv) {
    hash = ((hash << 5) + hash) ^ c;
  }
  return hash;
}

inline constexpr auto operator"" _sh(const char* str, size_t len) {
  return hash_djb2a(std::string_view{str, len});
}

namespace seatorrent::util {
  inline std::array<unsigned char, SHA_DIGEST_LENGTH> sha1(std::span<const unsigned char> sv) {
    std::array<unsigned char, SHA_DIGEST_LENGTH> result{};
    SHA1(sv.data(), sv.size(), result.data());
    return result;
  }

  inline std::array<unsigned char, SHA_DIGEST_LENGTH> sha1(const std::string_view sv) {
    return sha1(
      std::span<const unsigned char>(reinterpret_cast<const unsigned char*>(sv.data()), sv.size()));
  }
} // namespace seatorrent::util
