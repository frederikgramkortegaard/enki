#pragma once
#include "../definitions/tokens.hpp"
#include <string_view>
#include <vector>

std::vector<Token> lex(const std::string_view &source,
                       const std::string_view &file_nam);
