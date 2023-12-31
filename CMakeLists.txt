cmake_minimum_required(VERSION 3.22)
project(seatorrent
  VERSION 0.1.0
  DESCRIPTION "A BitTorrent library based on seastar"
  HOMEPAGE_URL "https://github.com/Ender-events/seatorrent"
  LANGUAGES CXX
)

add_library(seatorrent
  src/bencode/parser.cpp
)

target_compile_features(seatorrent PUBLIC cxx_std_20)
target_compile_options(seatorrent PUBLIC -Wall -Wextra -Wpedantic -Werror)

target_include_directories(
    seatorrent
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(dump_bencode
    apps/dump_bencode.cpp
)

target_link_libraries(dump_bencode PUBLIC seatorrent)

set_property(
  TARGET seatorrent dump_bencode
  PROPERTY EXPORT_COMPILE_COMMANDS ON
)

set_property(
  TARGET seatorrent dump_bencode
  PROPERTY CXX_EXTENSIONS OFF
)
