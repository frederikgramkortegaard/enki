#pragma once
#include "../definitions/ast.hpp"
#include "../definitions/tokens.hpp"
#include <string_view>
#include <vector>
#include <iostream>

std::shared_ptr<Program> parse(std::vector<Token> tokens);

struct ParserContext {
  std::shared_ptr<Program> program;
  const std::vector<Token> &tokens;
  size_t current = 0;

  bool eof() const { return current >= tokens.size(); }

  const Token &peek(size_t offset = 0) const {
    if (current + offset >= tokens.size())
      throw std::out_of_range("Peek past end of token stream");
    return tokens[current + offset];
  }

  const Token &current_token() const { return peek(); }

  const Token &consume() {
    if (eof())
      throw std::runtime_error("Unexpected end of input while consuming");
    std::cout << std::format(
        "Consuming token {}: {} at {}",
        current, magic_enum::enum_name(tokens[current].type),
        tokens[current].span.start.to_string()) << std::endl;
    return tokens[current++];
  }

  Span previous_token_span() const {
    if (current == 0)
      throw std::runtime_error("No previous token to get span from");
    return tokens[current - 1].span;
  }

  std::optional<Token> consume_if(TokenType expected) {
    if (!eof() && peek().type == expected) {
      return consume();
    }
    return std::nullopt;
  }

  const Token &consume_assert(TokenType expected,
                              std::string_view context = "") {
    if (eof())
      throw std::runtime_error(
          std::format("Expected token '{}' in {}, but reached end of input",
                      magic_enum::enum_name(expected), context));
    const Token &tok = peek();
    if (tok.type != expected) {
      throw std::runtime_error(
          std::format("Expected token '{}' in {}, but got '{}' at {}",
                      magic_enum::enum_name(expected), context,
                      magic_enum::enum_name(tok.type), tok.span.to_string()));
    }
    return consume();
  }
};
