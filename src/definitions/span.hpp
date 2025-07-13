#pragma once

#include "location.hpp"
#include <format>
#include <ostream> // for std::ostream
#include <sstream>
#include <string>      // for std::string
#include <string_view> // for std::string_view
struct Span {
  Location start, end;
  std::string to_string() const {
    return std::format("Start {}, End {}", start.to_string(), end.to_string());
  }
};

struct Spanned {
  Span span_;

  const Span &span() const { return span_; }
  Span &span() { return span_; }
};