#include "seatorrent/bencode/parser.hpp"

#include <charconv>
#include <format>
#include <stdexcept>
#include <string>

namespace seatorrent::bencode {
  parser::parser(std::string_view buffer)
    : buffer_{buffer} {
  }

  void parser::sax_parser(sax_t* sax) {
    sax_ = sax;
    position_ = 0;
    parser_content();
    sax_ = nullptr;
  }

  void parser::parser_content() {
    switch (buffer_[position_]) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
      parser_string();
      break;
    case 'i':
      parser_number();
      break;
    case 'l':
      parser_array();
      break;
    case 'd':
      parser_object();
      break;
    default:
      throw std::runtime_error(std::format("Invalid encoded value at pos {}", position_));
    }
  }

  void parser::parser_array() {
    sax_->start_array();
    ++position_; // pass 'l'
    while (buffer_[position_] != 'e') {
      parser_content();
    }
    ++position_; // pass 'e'
    sax_->end_array();
  }

  void parser::parser_number() {
    size_t end_index = buffer_.find('e', position_);
    if (end_index == std::string::npos) {
      throw std::runtime_error(std::format("Invalid encoded value at pos {}", position_));
    }
    int64_t number = 0;
    if (
      std::from_chars(buffer_.data() + position_ + 1, buffer_.data() + end_index, number).ec
      != std::errc()) {
      throw std::runtime_error(std::format("Invalid encoded value at pos {}", position_));
    }
    position_ = end_index + 1; // pass 'i<number>e'
    sax_->number(number);
  }

  void parser::parser_object() {
    sax_->start_object();
    ++position_; // pass 'd'
    while (buffer_[position_] != 'e') {
      size_t colon_index = buffer_.find(':', position_);
      if (colon_index == std::string::npos) {
        throw std::runtime_error(std::format("Invalid encoded value at pos {}", position_));
      }
      int64_t number = 0;
      if (
        std::from_chars(buffer_.data() + position_, buffer_.data() + colon_index, number).ec
        != std::errc()) {
        throw std::runtime_error(std::format("Invalid encoded value at pos {}", position_));
      }
      position_ = colon_index + 1; // pass '<number>:'
      auto str = buffer_.substr(position_, number);
      position_ += number; // pass '<string>'
      sax_->key(str);
      parser_content();
    }
    ++position_; // pass 'e'
    sax_->end_object();
  }

  void parser::parser_string() {
    size_t colon_index = buffer_.find(':', position_);
    if (colon_index == std::string::npos) {
      throw std::runtime_error(std::format("Invalid encoded value at pos {}", position_));
    }
    int64_t number = 0;
    if (
      std::from_chars(buffer_.data() + position_, buffer_.data() + colon_index, number).ec
      != std::errc()) {
      throw std::runtime_error(std::format("Invalid encoded value at pos {}", position_));
    }
    position_ = colon_index + 1; // pass '<number>:'
    auto str = buffer_.substr(position_, number);
    position_ += number; // pass '<string>'
    sax_->string(str);
  }
}
