#include "seatorrent/bencode/metadata.hpp"
#include "seatorrent/bencode/parser.hpp"

#include <algorithm>
#include <bits/ranges_algo.h>
#include <cctype>
#include <coroutine>
#include <cstddef>
#include <cstdio>
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

void dump_stdin() {
  std::string stdin_buffer{};
  std::getline(std::cin, stdin_buffer, static_cast<char>(EOF));
  seatorrent::bencode::parser parser{stdin_buffer};
  seatorrent::bencode::metadata meta{};
  parser.lazy_parser_to(meta);
}

void dump_file(const std::string& file_path) {
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
  if (argc <= 1 || std::string_view{args[1]} == "-") {
    dump_stdin();
  } else {
    for (int i = 1; i < argc; ++i) {
      dump_file(args[i]);
    }
  }
}
