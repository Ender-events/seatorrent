#pragma once

#include "seatorrent/message/message.hpp"
#include "seatorrent/util/net.hpp"

#include <cstddef>
#include <cstdint>
#include <exec/task.hpp>
#include <iostream>
#include <stdnet/internet.hpp>
#include <stdnet/io_context.hpp>
#include <stdnet/socket.hpp>
#include <string>
#include <string_view>
#include <vector>

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

    // Element access

    const std::vector<uint8_t>& get_bitfield() const {
      return bitfield_;
    }

    // network
    exec::task<void> handshake(std::string_view info_hash, std::string_view peer_id) {
      std::string buf{};
      auto out = std::back_inserter(buf);
      std::string_view protocol_id{"BitTorrent protocol"};
      out = std::format_to(out, "{:c}{}", protocol_id.size(), protocol_id);
      out = std::format_to(
        out, "{:c}{:c}{:c}{:c}{:c}{:c}{:c}{:c}", 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      out = std::format_to(out, "{}{}", info_hash, peer_id);
      co_await util::send_all(tcp_, buf);

      std::array<char, 68> buffer{};
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

    exec::task<message::message_view> recv_message(message::buffer& buffer) {
      auto lenght = co_await recv_lenght();
      buffer.resize(lenght);
      co_await recv(buffer.data(), lenght);
      co_return message::message_view{&buffer};
    }

    exec::task<uint32_t> recv_lenght() {
      uint32_t lenght = 0;
      co_await recv((char*) &lenght, sizeof(lenght));
      co_return ntohl(lenght);
    }

    exec::task<message::type> recv_type() {
      message::type type{};
      co_await recv((char*) &type, sizeof(type));
      co_return type;
    }

    exec::task<void> recv_bitfield(size_t lenght) {
      bitfield_.resize(lenght);
      co_await recv((char*) bitfield_.data(), lenght);
    }

    exec::task<void> send_choke() {
      co_await send_type_empty_payload(message::type::choke);
    }

    exec::task<void> send_unchoke() {
      co_await send_type_empty_payload(message::type::unchoke);
    }

    exec::task<void> send_interested() {
      co_await send_type_empty_payload(message::type::interested);
    }

    exec::task<void> send_not_interested() {
      co_await send_type_empty_payload(message::type::not_interested);
    }

   private:
    peer(stdnet::basic_stream_socket<stdnet::ip::tcp>&& tcp)
      : tcp_{std::move(tcp)} {
    }

    exec::task<void> recv(char* buffer, size_t size) {
      std::size_t end = 0;
      while (true) {
        auto n = co_await stdnet::async_receive(tcp_, stdnet::buffer(buffer + end, size - end));
        end += n;
        if (n == 0u || end == size) {
          break;
        }
      }
    }

    exec::task<void> send_type_empty_payload(message::type type) {
      char buf[5] = {0, 0, 0, 1, static_cast<char>(type)};
      std::string_view view{buf, sizeof(buf)};
      co_await util::send_all(tcp_, view);
    }

    stdnet::basic_stream_socket<stdnet::ip::tcp> tcp_;
    std::string peer_id_;
    std::vector<uint8_t> bitfield_{};
  };
} // namespace seatorrent
