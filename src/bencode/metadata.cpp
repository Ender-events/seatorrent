#include "seatorrent/bencode/metadata.hpp"

#include "seatorrent/bencode/coroutine.hpp"
#include "seatorrent/util/hash.hpp"
#include "seatorrent/bencode/lazy_parse.hpp"
#include <ostream>
#include <stdexcept>
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
      case "info"_sh:
        co_await bencode->get_to(metadata.info);
        break;
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
}
