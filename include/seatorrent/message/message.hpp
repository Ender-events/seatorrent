#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace seatorrent::message {

  enum type : uint8_t {
    choke,
    unchoke,
    interested,
    not_interested,
    have,
    bitfield,
    request,
    piece,
    cancel,
  };

  using buffer = std::vector<char>;

  class message_view {
   public:
    message_view(buffer* buf)
      : buffer_{buf} {};

    seatorrent::message::type type() {
      return static_cast<seatorrent::message::type>(buffer_->at(0));
    }
   private:
    buffer* buffer_;
  };
} // namespace seatorrent::message
