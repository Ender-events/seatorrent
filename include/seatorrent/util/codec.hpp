#pragma once

#include <string_view>

#include <span>

namespace seatorrent::util {
  struct url_encode_sv : std::string_view {
    template <typename... T>
    constexpr url_encode_sv(T&&... t)
      : std::string_view(std::forward<T>(t)...) {
    }
  };
} // namespace seatorrent::util

template <>
struct std::formatter<seatorrent::util::url_encode_sv> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  auto format(const seatorrent::util::url_encode_sv& data, std::format_context& ctx) const {
    auto out = ctx.out();
    for (char c: data) {
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || c == '-' || c == '_') {
        out = std::format_to(out, "{}", c);
      } else {
        out = std::format_to(out, "%{:02x}", static_cast<unsigned char>(c));
      }
    }
    return out;
  }
};

namespace seatorrent::util {
  inline std::string url_encode(std::span<unsigned char> data) {
    return std::format("{}", url_encode_sv{reinterpret_cast<const char*>(data.data()), data.size()});
  }
} // namespace seatorrent::util
