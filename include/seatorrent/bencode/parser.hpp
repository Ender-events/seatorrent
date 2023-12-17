#pragma once

#include "seatorrent/bencode/sax_interface.hpp"

#include <cstddef>
#include <string_view>

namespace seatorrent::bencode {
  class parser {
   public:
    parser(std::string_view buffer);
    void sax_parser(sax_t* sax);
    template <typename T>
    void lazy_parser_to(T& obj);
   private:
    void parser_array();
    void parser_content();
    void parser_number();
    void parser_object();
    void parser_string();
    std::string_view buffer_;
    sax_t* sax_ = nullptr;
    std::size_t position_ = 0;
  };
}

#include "parser.hxx" // IWYU pragma: keep
