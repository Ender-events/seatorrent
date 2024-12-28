#include "seatorrent/bencode/metadata.hpp"
#include "seatorrent/peer.hpp"
#include "seatorrent/util/net.hpp"
#include "stdnet/netfwd.hpp"

#include <arpa/inet.h>
#include <exec/async_scope.hpp>
#include <span>
#include <string_view>

int main(int argc, char* argv[]) {
  auto args = std::span(argv, argc);
  auto context = stdnet::io_context{};
  exec::async_scope scope;
  scope.spawn(std::invoke(
    [](auto& context, std::span<char*> args) -> exec::task<void> {
      std::string_view endpoint{args[2]};
      auto sep = endpoint.find_last_of(':');
      std::string host{endpoint.substr(0, sep)};
      auto port = std::atoi(endpoint.substr(sep + 1).data());
      stdnet::ip::address_v4::uint_type sin_addr = 0;
      inet_pton(AF_INET, host.c_str(), &sin_addr);
      auto peer =
        co_await seatorrent::peer::dial(context, stdnet::ip::address_v4{ntohl(sin_addr)}, port);
      auto meta = seatorrent::bencode::metadata::from_file(args[1]);
      co_await peer.handshake(meta.info_hash, "-st0001-1J_mP.p3e45-");
      auto res = co_await peer.recv();
      std::cout << res << '\n';
    },
    context,
    args));
  context.run();
}
