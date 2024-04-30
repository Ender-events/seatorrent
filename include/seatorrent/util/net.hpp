#pragma once

#include <cstddef>
#include <format>
#include <iterator>
#include <memory>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>

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

    std::string_view substr(size_t begin, size_t end) {
      return std::string_view(url_).substr(begin, end - begin);
    }
  };
}
