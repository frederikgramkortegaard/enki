#pragma once

#include <format>
#include <string>
#include <string_view> // for std::string_view
struct Location {
  int row, col, pos;
  std::string_view file_name;
  std::string to_string() const {
    return std::format("row {}, col {}, pos {}", row, col, pos);
  }
};
