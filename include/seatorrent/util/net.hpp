#pragma once

#include <cstddef>
#include <cstdlib>
#include <exec/task.hpp>
#include <format>
#include <generator>
#include <iterator>
#include <memory>
#include <netdb.h>
#include <stdexcept>
#include <stdnet/buffer.hpp>
#include <stdnet/internet.hpp>
#include <stdnet/io_context.hpp>
#include <stdnet/socket.hpp>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>

namespace seatorrent::util {
  class url {
    constexpr static std::string_view valid_host =
      "-.0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

   public:
    url(std::string url)
      : url_{std::move(url)}
      , scheme_{url_.find(":")}
      , userinfo_{scheme_ + 3}
      , host_{url_.size()}
      , port_{url_.size()}
      , path_{url_.size()}
      , query_{url_.size()} {
      if (scheme_ == std::string::npos) {
        throw std::invalid_argument{std::format("Can't find scheme in {}", url_)};
      }
      if (url_[scheme_ + 1] == '/' && url_[scheme_ + 2] == '/') {
        userinfo_ = url_.find("@", scheme_ + 2);
        if (userinfo_ == std::string::npos) {
          userinfo_ = scheme_ + 3;
        }
        host_ = url_.find_first_not_of(valid_host, userinfo_);
        if (host_ == std::string::npos) {
          throw std::invalid_argument{std::format("Can't find host in {}", url_)};
        }
        if (url_[host_] == ':') {
          port_ = url_.find_first_not_of("0123456789", host_ + 1);
        } else {
          port_ = host_ + 1;
        }
      } else {
        port_ = scheme_ + 1;
      }
      path_ = url_.find_first_of("?#", port_);
      if (path_ == std::string::npos) {
        return;
      }
      if (url_[path_] == '?') {
        query_ = url_.find('#', path_ + 1);
        if (query_ == std::string::npos) {
          return;
        }
      } else {
        query_ = path_;
      }
    }

    std::string_view scheme() const {
      return substr(0, scheme_);
    }

    std::string_view userinfo() const {
      return substr(scheme_ + 3, userinfo_);
    }

    std::string_view host() const {
      return substr(userinfo_, host_);
    }

    std::string_view port() const {
      return substr(host_ + 1, port_);
    }

    std::string_view path() const {
      return substr(port_, path_);
    }

    std::string_view query() const {
      if (path_ >= url_.size() || query_ == path_) {
        return "";
      }
      return substr(path_ + 1, query_);
    }

    std::string_view fragment() const {
      if (query_ >= url_.size()) {
        return "";
      }
      return view().substr(query_ + 1);
    }

    std::string_view view() const {
      return url_;
    }

   private:
    std::string url_;
    size_t scheme_;
    size_t userinfo_;
    size_t host_;
    size_t port_;
    size_t path_;
    size_t query_;

    std::string_view substr(size_t begin, size_t end) const {
      return view().substr(begin, end - begin);
    }
  };

  std::generator<stdnet::ip::basic_endpoint<stdnet::ip::tcp>>
    resolve(const std::string& node, const std::string& service) {
    struct addrinfo hints;
    struct addrinfo* raw = nullptr;
    // get host info, make socket, and connect it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    if (auto err = getaddrinfo(node.data(), service.data(), &hints, &raw); err) {
      throw std::runtime_error{gai_strerror(err)};
    }
    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> addrs{raw, freeaddrinfo};
    for (auto* addr = addrs.get(); addr != nullptr; addr = addr->ai_next) {
      if (addr->ai_family == AF_INET) {
        auto* addrv4 = reinterpret_cast<struct sockaddr_in*>(addr->ai_addr);
        co_yield stdnet::ip::basic_endpoint<stdnet::ip::tcp>{
          stdnet::ip::address_v4(ntohl(addrv4->sin_addr.s_addr)), ntohs(addrv4->sin_port)};
      }
    }
  }

  exec::task<stdnet::basic_stream_socket<stdnet::ip::tcp>>
    dial(stdnet::io_context& context, const util::url& url) {
    std::string host{url.host()};
    std::string service{url.port()};
    if (service.empty()) {
      service = url.scheme();
    }
    for (auto endpoint: resolve(host, service)) {
      stdnet::basic_stream_socket<stdnet::ip::tcp> client{context, endpoint};
      co_await stdnet::async_connect(client);
      co_return client;
    }
    throw std::runtime_error(std::format("Can't find endpoint for {}", url.view()));
  }

  exec::task<void>
    send_all(stdnet::basic_stream_socket<stdnet::ip::tcp>& client, std::string_view data) {
    std::size_t p{}, n{};
    do
      n = co_await stdnet::async_send(client, stdnet::buffer(data.data() + p, data.size() - p));
    while (0 < n && (p += n) != data.size());
    co_return;
  }

  exec::task<std::vector<char>> recv_all(stdnet::basic_stream_socket<stdnet::ip::tcp>& client) {
    std::vector<char> buffer(1024);
    std::size_t end = 0;
    while (true) {
      if (buffer.size() == end)
        buffer.resize(buffer.size() * 2);
      auto n = co_await stdnet::async_receive(
        client, stdnet::buffer(buffer.data() + end, buffer.size() - end));
      if (n == 0u)
        break;
      end += n;
    }
    buffer.resize(end);
    std::cout << "recv_all: " << end << "=" << buffer.size() << "\n";
    std::string_view res{buffer.data(), end};
    std::cout << res << std::endl;
    co_return buffer;
  }
} // namespace seatorrent::util
