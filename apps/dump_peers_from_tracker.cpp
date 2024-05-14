#include "seatorrent/bencode/metadata.hpp"
#include "seatorrent/bencode/parser.hpp"
#include "seatorrent/tracker.hpp"
#include "seatorrent/util/net.hpp"

#include <algorithm>
#include <bits/ranges_algo.h>
#include <cctype>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exec/async_scope.hpp>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

bool is_printable(char value) {
  return std::isprint(value) != 0;
}

exec::task<void>
  dump_tracker(stdnet::io_context& context, const seatorrent::bencode::metadata& meta) {
  seatorrent::tracker::request request{
    .info_hash = meta.info_hash,
    .peer_id = "-st0001-1J_mP.p3e45-",
    .port = 6881,
    .uploaded = 0,
    .downloaded = 0,
    .left = 0,
    .corrupt = 0,
    .key = (uint32_t) -1,
    .event = seatorrent::tracker::request::event_t::started,
    .numwant = 5,
    .redundant = 0,
  };
  seatorrent::util::url url{meta.announce};
  auto response = co_await seatorrent::tracker::announce(context, url, request, "seatorrent/0.1");
  if (!response.failure_reason.empty()) {
    std::cout << "failure reason: " << response.failure_reason << '\n';
    co_return;
  }
  for (const auto& peer: response.peers) {
    std::cout << peer << '\n';
  }
  std::cout << "complete: " << response.complete << '\n';
}

exec::task<void> dump_stdin(stdnet::io_context& context) {
  std::string stdin_buffer{};
  std::getline(std::cin, stdin_buffer, static_cast<char>(EOF));
  seatorrent::bencode::parser parser{stdin_buffer};
  seatorrent::bencode::metadata meta{};
  parser.lazy_parser_to(meta);
  co_await dump_tracker(context, meta);
}

exec::task<void> dump_file(stdnet::io_context& context, const std::string& file_path) {
  int fd = open(file_path.c_str(), O_RDONLY); // NOLINT
  char* buffer = nullptr;
  size_t len = 0;
  try {
    struct stat stat = {};
    fstat(fd, &stat);
    len = stat.st_size;
    buffer = static_cast<char*>(mmap(nullptr, len, PROT_READ, MAP_PRIVATE, fd, 0));
    seatorrent::bencode::parser parser{
      std::string_view{buffer, len}
    };
    seatorrent::bencode::metadata meta{};
    parser.lazy_parser_to(meta);
    co_await dump_tracker(context, meta);
  } catch (...) {
    close(fd);
    if (buffer != nullptr) {
      munmap(buffer, len);
    }
    throw;
  }
}

int main(int argc, char* argv[]) {
  auto args = std::span(argv, argc);
  auto context = stdnet::io_context{};
  exec::async_scope scope;
  scope.spawn(std::invoke(
    [](auto& context, std::span<char*> args) -> exec::task<void> {
      if (args.size() <= 1 || std::string_view{args[1]} == "-") {
        co_await dump_stdin(context);
      } else {
        for (size_t i = 1; i < args.size(); ++i) {
          co_await dump_file(context, args[i]);
        }
      }
    },
    context,
    args));
  context.run();
}
