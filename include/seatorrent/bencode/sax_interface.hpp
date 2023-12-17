#pragma once

#include <string_view>

namespace seatorrent::bencode {
  class sax_t {
   public:
    sax_t() = default;
    sax_t(const sax_t&) = delete;
    sax_t& operator=(const sax_t&) = delete;
    sax_t(sax_t&&) = delete;
    sax_t& operator=(sax_t&&) = delete;
    virtual ~sax_t() = default;
    virtual void number(long int value) = 0;
    virtual void string(std::string_view value) = 0;
    virtual void start_array() = 0;
    virtual void end_array() = 0;
    virtual void start_object() = 0;
    virtual void end_object() = 0;
    virtual void key(std::string_view value) = 0;
  };
}
