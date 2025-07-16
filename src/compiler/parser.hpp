#pragma once
#include "../definitions/ast.hpp"
#include "../definitions/tokens.hpp"
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>
#include <iostream>
#include "modules.hpp"


struct ModuleContext;
Ref<Program> parse(const std::vector<Token> &tokens);

struct ParserContext {
  Ref<Program> program;
  const std::vector<Token> &tokens;
  Ref<ModuleContext> module_context;
  std::string current_file_path;
  Ref<Scope> current_scope;

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
    spdlog::debug("Consuming token {}: {} at {}", current,
                  magic_enum::enum_name(tokens[current].type),
                  tokens[current].span.start.to_string());
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

  void consume_assert(TokenType type, const std::string &message);
};

Ref<Expression> parse_atom(ParserContext &ctx);
Ref<Expression> parse_expression(ParserContext &ctx);
Ref<Statement> parse_statement(ParserContext &ctx);
