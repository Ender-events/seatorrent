#include "seatorrent/bencode/parser.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <iomanip>
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

struct json_dump : public seatorrent::bencode::sax_t {
  void number(long int value) override {
    if (is_array()) {
      print_comma();
    }
    std::cout << value;
  }

  void string(std::string_view value) override {
    if (is_array()) {
      print_comma();
    }
    if (std::ranges::all_of(value, is_printable)) {
      std::cout << std::quoted(value);
    } else {
      std::cout << R"("<binary>")";
    }
  }

  void start_array() override {
    if (is_array()) {
      print_comma();
    }
    std::cout << "[";
    comma_ = false;
    array_state_.push_back(true);
  }

  void end_array() override {
    std::cout << "]";
    array_state_.pop_back();
  }

  void start_object() override {
    if (is_array()) {
      print_comma();
    }
    std::cout << "{";
    comma_ = false;
    array_state_.push_back(false);
  }

  void end_object() override {
    std::cout << "}";
    array_state_.pop_back();
  }

  void key(std::string_view value) override {
    print_comma();
    std::cout << '"' << value << R"(":)";
  }
 private:
  bool comma_ = false;
  std::vector<bool> array_state_{};

  void print_comma() {
    if (comma_) {
      std::cout << ',';
    } else {
      comma_ = true;
    }
  }

  bool is_array() {
    return !array_state_.empty() && array_state_.back();
  }
};

void dump_stdin() {
  std::string stdin_buffer{};
  std::getline(std::cin, stdin_buffer, static_cast<char>(EOF));
  seatorrent::bencode::parser parser{stdin_buffer};
  json_dump dump{};
  parser.sax_parser(&dump);
  std::cout << std::endl;
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
    json_dump dump{};
    parser.sax_parser(&dump);
    std::cout << std::endl;
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
