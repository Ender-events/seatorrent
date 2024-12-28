#pragma once

#include "seatorrent/util/net.hpp"

#include <exec/task.hpp>
#include <iostream>
#include <stdnet/internet.hpp>
#include <stdnet/io_context.hpp>
#include <stdnet/socket.hpp>
#include <string_view>

namespace seatorrent {
  class peer {
   public:
    static exec::task<peer>
      dial(stdnet::io_context& context, stdnet::ip::address_v4 address, stdnet::ip::port_type port) {
      stdnet::ip::basic_endpoint<stdnet::ip::tcp> endpoint{address, port};
      stdnet::basic_stream_socket<stdnet::ip::tcp> tcp{context, endpoint};
      co_await stdnet::async_connect(tcp);
      co_return peer{std::move(tcp)};
    }

    exec::task<void> handshake(std::string_view info_hash, std::string_view peer_id) {
      std::string buf{};
      auto out = std::back_inserter(buf);
      std::string_view protocol_id{"BitTorrent protocol"};
      out = std::format_to(out, "{:c}{}", protocol_id.size(), protocol_id);
      out = std::format_to(
        out, "{:c}{:c}{:c}{:c}{:c}{:c}{:c}{:c}", 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      out = std::format_to(out, "{}{}", info_hash, peer_id);
      co_await util::send_all(tcp_, buf);

      std::vector<char> buffer(68);
      std::size_t end = 0;
      while (true) {
        auto n = co_await stdnet::async_receive(
          tcp_, stdnet::buffer(buffer.data() + end, buffer.size() - end));
        if (n == 0u && buffer.size() == end)
          break;
        end += n;
      }
      std::string_view res{buffer.data(), end};
      if (res.substr(28, 20) != info_hash) {
        throw std::runtime_error{std::format("handshake failed: {}", res)};
      }
      peer_id_ = res.substr(48, 20);
      co_return;
    }

    exec::task<std::string> recv() {
      auto res = co_await util::recv(tcp_);
      co_return std::string{res.data(), res.size()};
    }

   private:
    peer(stdnet::basic_stream_socket<stdnet::ip::tcp>&& tcp)
      : tcp_{std::move(tcp)} {
    }

    stdnet::basic_stream_socket<stdnet::ip::tcp> tcp_;
    std::string peer_id_;
  };
} // namespace seatorrent
