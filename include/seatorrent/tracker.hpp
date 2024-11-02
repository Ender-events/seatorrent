#pragma once

#include "seatorrent/bencode/parser.hpp"
#include "seatorrent/util/hash.hpp"
#include "seatorrent/util/net.hpp"

#include <bits/types/struct_iovec.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exec/task.hpp>
#include <format>
#include <iterator>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <memory>
#include <netinet/in.h>
#include <optional>
#include <stdnet/buffer.hpp>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

namespace seatorrent::tracker {

  struct request {
    enum class event_t {
      empty,
      started,
      completed,
      stopped,
    };
    std::string info_hash{};
    std::string_view peer_id{};
    stdnet::ip::port_type port{};
    int64_t uploaded{};
    int64_t downloaded{};
    int64_t left{};
    int64_t corrupt{};
    uint32_t key{};
    event_t event{};
    int numwant{};
    // compact=1
    // no_peer_id=1
    // supportcrypto=0
    int64_t redundant{};
    std::string ip{};
  };

  struct response {
    std::string failure_reason{};
    int64_t complete{};
    int64_t downloaded{};
    int64_t incomplete{};
    int64_t interval{};
    std::optional<int64_t> min_interval{};
    std::vector<stdnet::ip::basic_endpoint<stdnet::ip::tcp>> peers{};
    std::vector<stdnet::ip::basic_endpoint<stdnet::ip::tcp>> peers6{};
  };
} // namespace seatorrent::tracker

template <>
struct std::formatter<seatorrent::tracker::request::event_t> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  auto format(const seatorrent::tracker::request::event_t& event, std::format_context& ctx) const {
    auto out = ctx.out();
    switch (event) {
      using enum seatorrent::tracker::request::event_t;
    case empty:
      return std::format_to(out, "empty");
    case started:
      return std::format_to(out, "started");
    case completed:
      return std::format_to(out, "completed");
    case stopped:
      return std::format_to(out, "stopped");
    }
    return out;
  }
};

template <>
struct std::formatter<seatorrent::tracker::request> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin(); // TODO: option to dump in query or udp
  }

  auto format(const seatorrent::tracker::request& req, std::format_context& ctx) const {
    auto out = ctx.out();
    out = std::format_to(
      ctx.out(),
      "info_hash={}&peer_id={}&port={}&uploaded={}&downloaded={}&left={}&corrupt={}&key={:08X}",
      req.info_hash,
      req.peer_id,
      req.port,
      req.uploaded,
      req.downloaded,
      req.left,
      req.corrupt,
      req.key);
    if (req.event != seatorrent::tracker::request::event_t::empty) {
      out = std::format_to(ctx.out(), "&event={}", req.event);
    }
    out = std::format_to(
      ctx.out(),
      "&numwant={}&compact=1&no_peer_id=1&supportcrypto=1&redundant={}",
      req.numwant,
      req.redundant);
    if (!req.ip.empty()) {
      out = std::format_to(ctx.out(), "&ip={}", req.ip);
    }
    return out;
  }
};

// tmp stdnet have hpp
namespace seatorrent::tracker {
  exec::task<response> announce(
    stdnet::io_context& context,
    const util::url& url,
    const request& request,
    std::string_view user_agent) {
    auto client = co_await dial(context, url);
    std::string buf{};
    auto out = std::back_inserter(buf);
    out = std::format_to(out, "GET {}?{} HTTP/1.1\r\n", url.path(), request);
    out = std::format_to(out, "Host: {}\r\n", url.host());
    out = std::format_to(out, "User-Agent: {}\r\n", user_agent);
    out = std::format_to(out, "Connection: close\r\n\r\n");
    co_await util::send_all(client, buf);
    auto res = co_await util::recv_all(client);
    std::string_view res_sv{res.data(), res.size()};
    auto header = res_sv.find("\r\n\r\n");
    if (header == std::string_view::npos) {
      co_return response{"invalid response header"};
    }
    seatorrent::bencode::parser parser{res_sv.substr(header + 4)};
    response resp{};
    parser.lazy_parser_to(resp);
    co_return resp;
  }
} // namespace seatorrent::tracker

namespace seatorrent::bencode {
  lazy_parse from_bencode(element* bencode, seatorrent::tracker::response& resp) {
    TRACE_PARSE("begin from bencode tracker::response");
    while (true) {
      auto key_tk = co_await bencode->next_token();
      if (std::holds_alternative<type::end_object>(key_tk)) {
        break;
      }
      auto key = std::get<type::key>(key_tk);
      TRACE_PARSE(
        "NEXT_TOKEN (response)"
        << ": " << key);
      switch (hash_djb2a(key)) {
      case "failure_reason"_sh:
        co_await bencode->get_to(resp.failure_reason);
        break;
      case "complete"_sh:
        co_await bencode->get_to(resp.complete);
        break;
      case "downloaded"_sh:
        co_await bencode->get_to(resp.downloaded);
        break;
      case "incomplete"_sh:
        co_await bencode->get_to(resp.incomplete);
        break;
      case "interval"_sh:
        co_await bencode->get_to(resp.interval);
        break;
      case "min_interval"_sh:
        co_await bencode->get_to(resp.min_interval.emplace());
        break;
      case "peers"_sh: {
        struct __attribute__((packed)) peers_t {
          uint32_t ip;
          uint16_t port;
        };

        std::span<const peers_t> peers_span{};
        co_await bencode->get_to(peers_span);
        for (const auto& peer: peers_span) {
          resp.peers.emplace_back(stdnet::ip::basic_endpoint<stdnet::ip::tcp>{
            stdnet::ip::address_v4{ntohl(peer.ip)}, ntohs(peer.port)});
        }
        break;
      }
      case "peers6"_sh: {
        // TODO parse ipv6
        co_await bencode->ignore_token();
        break;
      }
      default:
        co_await bencode->ignore_token();
        break;
      }
    }
  }
} // namespace seatorrent::bencode
