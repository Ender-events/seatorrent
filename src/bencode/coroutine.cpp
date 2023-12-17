#include "seatorrent/bencode/coroutine.hpp"

namespace seatorrent::bencode {

  void sax_coroutine::number(long int value) {
    TRACE_CORO("number: " << value);
    async_->resume(value);
    TRACE_CORO("number resumed");
  }

  void sax_coroutine::string(std::string_view value) {
    TRACE_CORO(
      "string: "
      << ((std::ranges::all_of(value, [](char c) { return std::isprint(c) != 0; }))
            ? std::quoted(value)
            : std::quoted(std::string_view{"<binary>"})));
    async_->resume(value);
    TRACE_CORO("string resumed");
  }

  void sax_coroutine::start_array() {
    TRACE_CORO("start_array");
    async_->resume(type::start_array{});
    TRACE_CORO("start_array resumed");
  }

  void sax_coroutine::end_array() {
    TRACE_CORO("end_array");
    async_->resume(type::end_array{});
    TRACE_CORO("end_array resumed");
  }

  void sax_coroutine::start_object() {
    TRACE_CORO("start_object");
    async_->resume(type::start_object{});
    TRACE_CORO("start_object resumed");
  }

  void sax_coroutine::end_object() {
    TRACE_CORO("end_object");
    async_->resume(type::end_object{});
    TRACE_CORO("end_object resumed");
  }

  void sax_coroutine::key(std::string_view value) {
    TRACE_CORO("key: " << value);
    async_->resume(type::key{value});
    TRACE_CORO("key resumed");
  }
}
