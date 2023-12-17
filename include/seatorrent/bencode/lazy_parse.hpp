#pragma once

#include "seatorrent/util/log.hpp"

#include <coroutine>
#include <iostream>

namespace seatorrent::bencode {
  struct lazy_parse {
    ~lazy_parse() {
      if (continuation) {
        TRACE_CORO("destroy " << continuation.address());
        continuation.destroy();
      }
    }

    struct promise_type {
      struct final_awaiter {
        [[nodiscard]] bool await_ready() const noexcept {
          return false;
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
          auto& promise = handle.promise();
          auto coro = promise.continuation ? promise.continuation : std::noop_coroutine();
          TRACE_CORO("final resume " << &promise << "; " << coro.address());
          return coro;
        }

        void await_resume() noexcept {
        }
      };

      lazy_parse get_return_object() {
        continuation = std::coroutine_handle<promise_type>::from_promise(*this);
        TRACE_CORO("get_return_object " << this << "; " << continuation.address());
        return {std::coroutine_handle<promise_type>::from_promise(*this)};
      }

      std::suspend_always initial_suspend() {
        TRACE_CORO("initial_suspend " << this);
        return {};
      }

      final_awaiter final_suspend() noexcept {
        TRACE_CORO("final_suspend " << this);
        return {};
      }

      void return_void() {
      }

      void unhandled_exception() {
        throw;
      }

      std::coroutine_handle<> continuation = nullptr;
    };

    struct nested_awaiter {
      std::coroutine_handle<promise_type> continuation = nullptr;

      [[nodiscard]] bool await_ready() const {
        return false;
      }

      std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) const {
        auto old = continuation.promise().continuation;
        continuation.promise().continuation = handle;
        TRACE_CORO(
          "await suspend "
          << &continuation.promise() << "; handle " << handle.address() << " and resume "
          << continuation.address() << " (old: " << old.address() << ")");
        static_cast<void>(old);
        return continuation;
      }

      void await_resume() {
      }
    };

    nested_awaiter operator co_await() {
      auto& promise = continuation.promise();
      auto coro = std::coroutine_handle<promise_type>::from_promise(promise);
      TRACE_CORO(
        "co_await " << &promise << "; " << continuation.address() << " != " << coro.address());
      return {coro};
    }

    std::coroutine_handle<promise_type> continuation = nullptr;
  };
}
