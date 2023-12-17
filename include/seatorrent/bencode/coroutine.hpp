#pragma once

#include "seatorrent/bencode/metadata.hpp"
#include "seatorrent/bencode/sax_interface.hpp"
#include "seatorrent/bencode/lazy_parse.hpp"
#include "seatorrent/util/log.hpp"

#include <algorithm>
#include <coroutine>
#include <format>
#include <iomanip>
#include <type_traits>
#include <variant>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

namespace seatorrent::bencode {
  class element;

  namespace type {
    struct start_array { };

    struct end_array { };

    struct start_object { };

    struct end_object { };

    struct key : public std::string_view { };

    using data = std::variant<
      std::monostate,
      long int,
      std::string_view,
      start_array,
      end_array,
      start_object,
      end_object,
      key>;

  }

  namespace details {
    class async;
  }

  struct sax_coroutine : public seatorrent::bencode::sax_t {
    friend details::async;
    void number(long int value) override;
    void string(std::string_view value) override;
    void start_array() override;
    void end_array() override;
    void start_object() override;
    void end_object() override;
    void key(std::string_view value) override;

   private:
    details::async* async_;
  };

  namespace details {
    class async {
     public:
      async(sax_coroutine* sax, type::data data)
        : sax_{sax}
        , data_{data} {};

      [[nodiscard]] bool await_ready() const {
        return !std::holds_alternative<std::monostate>(data_);
      }

      std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) {
        TRACE_CORO("async::await_suspend: " << handle.address());
        coro_ = handle;
        sax_->async_ = this;
        return std::noop_coroutine();
      }

      type::data await_resume() {
        return data_;
      }

      void resume(type::data data) {
        data_ = data;
        TRACE_CORO("resume: " << coro_.address());
        coro_.resume();
      }
     private:
      sax_coroutine* sax_ = nullptr;
      type::data data_{};
      std::coroutine_handle<> coro_ = nullptr;
    };
  }

  class element {
   public:
    element(sax_coroutine* sax)
      : sax_{sax} {};

    template <typename T>
    lazy_parse get_to(T& value) {
      TRACE_PARSE("begin get to object");
      auto data = co_await next_token();
      TRACE_PARSE("get to next token");
      if (!std::holds_alternative<type::start_object>(data)) {
        throw std::runtime_error{std::format("expected start_object ({})", data.index())};
      }
      TRACE_PARSE("get to object from bencode");
      co_await from_bencode(this, value);
      TRACE_PARSE("end get to object");
    }

    lazy_parse get_to(std::integral auto& value) {
      auto token = co_await next_token();
      value = std::get<long int>(token);
      TRACE_PARSE("get to long int: " << value);
    }

    template <typename T>
    lazy_parse get_to(std::vector<T>& value) {
      TRACE_PARSE("begin get to vector");
      auto data = co_await next_token();
      if (!std::holds_alternative<type::start_array>(data)) {
        throw std::runtime_error{std::format("expected start_array ({})", data.index())};
      }
      TRACE_PARSE("get to vector from bencode");
      data = co_await next_token();
      while (!std::holds_alternative<type::end_array>(data)) {
        peek_ = data; // TODO: have a co_await peek_token();
        co_await get_to(value.emplace_back());
        TRACE_PARSE("get to vector from bencode");
        data = co_await next_token();
      }
      TRACE_PARSE("end get to vector");
    }

    lazy_parse get_to(std::string& value) {
      auto token = co_await next_token();
      value = std::get<std::string_view>(token);
      TRACE_PARSE(
        "get to string: "
        << ((std::ranges::all_of(value, [](char c) { return std::isprint(c) != 0; }))
              ? std::quoted(std::string_view{value})
              : std::quoted(std::string_view{"<binary>"})));
    }

    lazy_parse ignore_token() {
      uint64_t nb_array = 0;
      uint64_t nb_object = 0;
      do {
        auto token = co_await next_token();
        std::visit(
          overloaded{
            [&nb_array](const type::start_array&) { ++nb_array; },
            [&nb_array](const type::end_array&) { --nb_array; },
            [&nb_object](const type::start_object&) { ++nb_object; },
            [&nb_object](const type::end_object&) { --nb_object; },
            [](const auto&) {},
          },
          token);
        TRACE_PARSE("ignored: " << nb_array << ", " << nb_object);
      } while (nb_array != 0 || nb_object != 0);
    }

    details::async next_token() {
      auto data = peek_;
      peek_ = std::monostate();
      return details::async{sax_, data};
    };

   private:
    sax_coroutine* sax_ = nullptr;
    type::data peek_{};
  };
}
