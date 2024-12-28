#include "seatorrent/bencode/metadata.hpp"

#include "seatorrent/bencode/coroutine.hpp"
#include "seatorrent/bencode/lazy_parse.hpp"
#include "seatorrent/bencode/parser.hpp"
#include "seatorrent/util/hash.hpp"

#include <fcntl.h>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <variant>

namespace seatorrent::bencode {
  lazy_parse from_bencode(element* bencode, metadata& metadata) {
    TRACE_PARSE("begin from bencode metadata");
    while (true) {
      auto key_tk = co_await bencode->next_token();
      if (std::holds_alternative<type::end_object>(key_tk)) {
        break;
      }
      auto key = std::get<type::key>(key_tk);
      TRACE_PARSE(
        "NEXT_TOKEN (meta)"
        << ": " << key);
      switch (hash_djb2a(key)) {
      case "announce"_sh:
        co_await bencode->get_to(metadata.announce);
        break;
      case "announce-list"_sh:
        co_await bencode->get_to(metadata.announce_list);
        break;
      case "info"_sh: {
        auto b = bencode->get_current();
        co_await bencode->get_to(metadata.info);
        auto e = bencode->get_current();
        std::string_view info_span{b, e};
        auto hash = seatorrent::util::sha1(info_span);
        metadata.info_hash = {reinterpret_cast<const char*>(hash.data()), hash.size()};
        break;
      }
      default:
        co_await bencode->ignore_token();
        break;
      }
    }
  }

  lazy_parse from_bencode(element* bencode, struct metadata::info& info) {
    TRACE_PARSE("begin from bencode metadata info");
    while (true) {
      auto key_tk = co_await bencode->next_token();
      if (std::holds_alternative<type::end_object>(key_tk)) {
        break;
      }
      auto key = std::get<type::key>(key_tk);
      TRACE_PARSE(
        "NEXT_TOKEN (info)"
        << ": " << key);
      switch (hash_djb2a(key)) {
      case "name"_sh:
        co_await bencode->get_to(info.name);
        break;
      case "piece length"_sh:
        co_await bencode->get_to(info.piece_length);
        break;
      case "pieces"_sh:
        co_await bencode->get_to(info.pieces);
        break;
      case "length"_sh:
        co_await bencode->get_to(info.length);
        break;
      case "files"_sh:
        co_await bencode->get_to(info.files);
        break;
      default:
        co_await bencode->ignore_token();
        break;
      }
    }
  }

  lazy_parse from_bencode(element* bencode, struct metadata::info::file& file) {
    TRACE_PARSE("begin from bencode metadata info file");
    while (true) {
      auto key_tk = co_await bencode->next_token();
      if (std::holds_alternative<type::end_object>(key_tk)) {
        break;
      }
      auto key = std::get<type::key>(key_tk);
      TRACE_PARSE(
        "NEXT_TOKEN (file)"
        << ": " << key);
      switch (hash_djb2a(key)) {
      case "length"_sh:
        co_await bencode->get_to(file.length);
        break;
      case "path"_sh:
        co_await bencode->get_to(file.path);
        break;
      default:
        co_await bencode->ignore_token();
        break;
      }
    }
  }

  metadata metadata::from_file(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    char* buffer = nullptr;
    size_t len = 0;
    seatorrent::bencode::metadata meta{};
    try {
      struct stat stat = {};
      fstat(fd, &stat);
      len = stat.st_size;
      buffer = static_cast<char*>(mmap(nullptr, len, PROT_READ, MAP_PRIVATE, fd, 0));
      seatorrent::bencode::parser parser{
        std::string_view{buffer, len}
      };
      parser.lazy_parser_to(meta);
    } catch (...) {
      close(fd);
      if (buffer != MAP_FAILED) {
        munmap(buffer, len);
      }
      throw;
    }
    close(fd);
    if (buffer != MAP_FAILED) {
      munmap(buffer, len);
    }
    return meta;
  }
} // namespace seatorrent::bencode
