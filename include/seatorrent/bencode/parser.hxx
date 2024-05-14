#pragma once

#include "seatorrent/bencode/coroutine.hpp"
#include "seatorrent/bencode/lazy_parse.hpp"

namespace seatorrent::bencode {
  template <typename T>
  void parser::lazy_parser_to(T& obj) {
    sax_coroutine sax{};
    element bencode{&sax};
    auto lazy = bencode.get_to(obj);
    lazy_parse::nested_awaiter nested{lazy.continuation};
    nested.await_suspend(std::noop_coroutine()).resume();
    sax_parser(&sax);
  }
} // namespace seatorrent::bencode
