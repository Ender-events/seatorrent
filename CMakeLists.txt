cmake_minimum_required(VERSION 3.22)
project(seatorrent
  VERSION 0.1.0
  DESCRIPTION "A BitTorrent library based on seastar"
  HOMEPAGE_URL "https://github.com/Ender-events/seatorrent"
  LANGUAGES CXX
)

include(FetchContent)
# FetchContent_Declare(
#   stdexec
#   GIT_REPOSITORY "https://github.com/NVIDIA/stdexec.git"
#   GIT_TAG main
# )
# set(STDEXEC_BUILD_TESTS Off)
# set(STDEXEC_BUILD_EXAMPLES Off)
# FetchContent_MakeAvailable(stdexec)

# FetchContent_Declare(
#   stdnet
#   SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../dietmarkuehl/stdnet/"
#   GIT_REPOSITORY "https://github.com/dietmarkuehl/stdnet.git"
#   GIT_TAG main
# )
# FetchContent_MakeAvailable(stdnet)
add_subdirectory(stdnet)
include_directories(stdnet/include)

add_library(seatorrent
  src/bencode/coroutine.cpp
  src/bencode/metadata.cpp
  src/bencode/parser.cpp
  src/tracker.cpp
)

# add_compile_options(-fsanitize=address -g)
# add_link_options(-fsanitize=address)
target_compile_features(seatorrent PUBLIC cxx_std_23)
target_link_libraries(seatorrent PRIVATE ssl crypto)
target_link_libraries(seatorrent PUBLIC STDEXEC::stdexec event_core_shared)
target_compile_options(seatorrent PUBLIC -Wall -Wextra -Wpedantic) # -Werror)

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


add_executable(dump_peers_from_tracker
    apps/dump_peers_from_tracker.cpp
)

target_link_libraries(dump_peers_from_tracker PUBLIC seatorrent)

set_property(
  TARGET seatorrent dump_peers_from_tracker
  PROPERTY EXPORT_COMPILE_COMMANDS ON
)

set_property(
  TARGET seatorrent dump_peers_from_tracker
  PROPERTY CXX_EXTENSIONS OFF
)

add_executable(dump_url
    apps/dump_url.cpp
)

target_link_libraries(dump_url PUBLIC seatorrent)
