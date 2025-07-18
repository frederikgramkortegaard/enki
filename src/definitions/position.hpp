#pragma once

#include <format>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

struct Location {
  int row, col, pos;
  std::string_view file_name;
  std::string to_string() const {
    return std::format("{}:{}:{}", file_name, row + 1, col + 1);
  }
};

struct Span {
  Location start, end;
  std::string to_string() const {
    return std::format("Start {}, End {}", start.to_string(), end.to_string());
  }
};