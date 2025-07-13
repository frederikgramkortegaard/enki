
#pragma once
#include "span.hpp"
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <string>

#include "ast.hpp"
#include "symbols.hpp"
#include "tokens.hpp"
#include <format>
#include <ostream>     // for std::ostream
#include <string>      // for std::string
#include <string_view> // for std::string_view

enum class BaseType { Int, Float, String, Identifier };

struct Type {
  BaseType base_type;
  // TODO: add more types here
};
