#pragma once

// #include "seatorrent/bencode/lazy_parse.hpp"
// #include "seatorrent/bencode/coroutine.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace seatorrent::bencode {
  class lazy_parse;
  class element;

  /**
  * @brief Structure to represent BitTorrent metainfo data.
  */
  struct metadata {
    std::string announce{};                                /**< The URL of the tracker. */
    std::vector<std::vector<std::string>> announce_list{}; /**< List of tracker URLs. BEP 12 */

    struct info {
      std::string name{};      /**< Suggested name to save the file or directory as. */
      uint64_t piece_length{}; /**< Number of bytes in each piece. */
      std::string pieces{};    /**< SHA1 hashes of file pieces. */

      // Either length or files will be present, not both.
      uint64_t length{}; /**< Length of the file in bytes (for single-file torrents). */

      struct file {
        uint64_t length{};               /**< Length of the file in bytes. */
        std::vector<std::string> path{}; /**< Subdirectory names and file name. */
      };

      std::vector<file> files{}; /**< List of files for multi-file torrents. */
    } info;

    std::string info_hash{}; /**< SHA1 hash of the info dictionary. */

    static metadata from_file(const std::string& path);
  };

  lazy_parse from_bencode(element* bencode, metadata& metadata);
} // namespace seatorrent::bencode
