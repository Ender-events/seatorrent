#include "seatorrent/util/net.hpp"

#include <iostream>
#include <span>

int main(int argc, char* argv[]) {
  auto args = std::span(argv, argc);
  for (int i = 1; i < argc; ++i) {
    seatorrent::util::url url(args[i]);
    std::cout << "scheme: " << url.scheme() << '\n';
    std::cout << "userinfo: " << url.userinfo() << '\n';
    std::cout << "host: " << url.host() << '\n';
    std::cout << "port: " << url.port() << '\n';
    std::cout << "path: " << url.path() << '\n';
    std::cout << "query: " << url.query() << '\n';
    std::cout << "fragment: " << url.fragment() << '\n';
  }
}
