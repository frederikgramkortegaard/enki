#pragma once
#include "ast.hpp"
#include "span.hpp"
#include "types.hpp"
struct Symbol {
  std::string_view name;
  std::shared_ptr<Type> type;
  Span span;
};