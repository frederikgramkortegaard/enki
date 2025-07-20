#include "lexer.hpp"
#include "../definitions/tokens.hpp"
#include "../utils/logging.hpp"
#include <format>
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <vector>

std::unordered_map<std::string_view, TokenType> keyword_to_token_type = {
    {"let", TokenType::Let},         {"extern", TokenType::Extern},
    {"import", TokenType::Import},   {"from", TokenType::From},
    {"if", TokenType::If},           {"else", TokenType::Else},
    {"true", TokenType::True},       {"false", TokenType::False},
    {"while", TokenType::While},     {"return", TokenType::Return},
    {"define", TokenType::Define},   {"int", TokenType::IntType},
    {"float", TokenType::FloatType}, {"string", TokenType::StringType},
    {"bool", TokenType::BoolType},   {"void", TokenType::VoidType},
    {"char", TokenType::CharType},   {"enum", TokenType::EnumType}};

TokenType get_tokentype_for_keyword_or_ident(const std::string_view &str) {
  if (keyword_to_token_type.find(str) != keyword_to_token_type.end()) {
    return keyword_to_token_type.at(str);
  }
  return TokenType::Identifier;
}

std::vector<Token> lex(const std::string_view &source,
                       const std::string_view &file_name) {
  spdlog::debug("[lexer] Starting with source: '{}'", source);
  std::vector<Token> tokens;

  int cursor = 0;
  int row = 0;
  int col = 0;
  auto create_token = [&](TokenType type, Location start) {
    std::string_view value_view = source.substr(start.pos, cursor - start.pos);
    tokens.push_back(Token{
        type, value_view, {start, Location{row, col, cursor, file_name}}});
    spdlog::debug("[lexer] Created token of type {} with value '{}'",
                  magic_enum::enum_name(type), value_view);
  };

  // Increment the cursor and the column
  auto increment = [&](int amount = 1) {
    for (int i = 0; i < amount; i++) {
      switch (source[cursor]) {
      case '\n':
        row++;
        col = 0;
        break;
      default:
        col++;
        break;
      }

      cursor++;
    }
  };

  auto simple_token = [&](TokenType type, size_t length = 1) {
    Location start = {row, col, cursor, file_name};
    increment(length);
    create_token(type, start);
  };

  auto peek = [&](int offset) {
    if (cursor + offset < source.size()) {
      return source[cursor + offset];
    }
    return '\0'; // End of string
  };

  // start of lexing
  while (cursor < source.size()) {

    Location start = {row, col, cursor, file_name};
    // Skip whitespace

    switch (source[cursor]) {
    // Whitespace
    case '\n':
    case '\t':
    case '\v':
    case '\r':
    case ' ':
    case '\0':
      increment();
      break;

    // Operators / Syntax
    case '(':
      simple_token(TokenType::LParens);
      continue;
    case ')':
      simple_token(TokenType::RParens);
      continue;
    case '[':
      simple_token(TokenType::LSquare);
      continue;
    case ']':
      simple_token(TokenType::RSquare);
      continue;
    case '{':
      simple_token(TokenType::LCurly);
      continue;
    case '}':
      simple_token(TokenType::RCurly);
      continue;
    case ',':
      simple_token(TokenType::Comma);
      continue;
    case '.':
      simple_token(TokenType::Dot);
      continue;
    case '|':
      simple_token(TokenType::Pipe);
      continue;
    case ':':
      simple_token(TokenType::Colon);
      continue;
    case ';':
      simple_token(TokenType::Semicolon);
      continue;
    case '=': {
      if (peek(1) == '=') {
        simple_token(TokenType::EqualsEquals, 2);
      } else {
        simple_token(TokenType::Equals);
      }
      continue;
    }
    case '!': {
      if (peek(1) == '=') {
        simple_token(TokenType::NotEquals, 2);
      } else {
        simple_token(TokenType::Exclamation);
      }
      continue;
    }
    case '+':
      simple_token(TokenType::Plus);
      continue;
    case '-': {
      if (cursor + 1 < source.size() && source[cursor + 1] == '>') {
        simple_token(TokenType::Arrow, 2);
      } else {
        simple_token(TokenType::Minus);
      }
      continue;
    }
    case '>':
      if (peek(1) == '=') {
        simple_token(TokenType::GreaterThanEquals, 2);
      } else {
        simple_token(TokenType::GreaterThan);
      }
      continue;
    case '*':
      simple_token(TokenType::Asterisk);
      continue;
    case '&':
      simple_token(TokenType::Ampersand);
      continue;
    case '<': {
      if (peek(1) == '=') {
        simple_token(TokenType::LessThanEquals, 2);
      } else {
        simple_token(TokenType::LessThan);
      }
      continue;
    }

    // Single-line comments
    case '/': {
      if (peek(1) == '/') {
        while (cursor < source.size() && source[cursor] != '\n') {
          increment();
        }
      } else {
        simple_token(TokenType::Slash);
      }
      continue;
    }

      // String literal
    case '"': {
      spdlog::debug("[lexer] Found string literal at {}:{}", row, col);
      increment(); // skip opening quote
      while (cursor < source.size() && source[cursor] != '"') {
        if (source[cursor] == '\\' && cursor + 1 < source.size()) {
          increment(2);
        } else {
          increment();
        }
      }

      if (cursor >= source.size()) {
        Location error_loc = {row, col, cursor, file_name};
        Span error_span = {error_loc, error_loc};
        LOG_ERROR_EXIT("[lexer] Unterminated string literal at " +
                           std::to_string(row) + ":" + std::to_string(col),
                       error_span);
      } else {
        start.pos += 1;
        start.col += 1;
        create_token(TokenType::String, start);
        increment(); // skip closing quote
      }
      continue;
    }

      // Character literal
    case '\'': {
      spdlog::debug("[lexer] Found character literal at {}:{}", row, col);
      increment(); // skip opening quote

      if (cursor >= source.size()) {
        Location error_loc = {row, col, cursor, file_name};
        Span error_span = {error_loc, error_loc};
        LOG_ERROR_EXIT("[lexer] Unterminated character literal at " +
                           std::to_string(row) + ":" + std::to_string(col),
                       error_span);
      }

      // Handle escaped characters
      if (source[cursor] == '\\' && cursor + 1 < source.size()) {
        increment(2); // skip backslash and escaped character
      } else {
        increment(); // skip the character
      }

      if (cursor >= source.size() || source[cursor] != '\'') {
        Location error_loc = {row, col, cursor, file_name};
        Span error_span = {error_loc, error_loc};
        LOG_ERROR_EXIT("[lexer] Unterminated character literal at " +
                           std::to_string(row) + ":" + std::to_string(col),
                       error_span);
      } else {
        start.pos += 1;
        start.col += 1;
        create_token(TokenType::Char, start);
        increment(); // skip closing quote
      }
      continue;
    }
    default: {

      // Number literal
      auto previous = cursor;
      while (cursor < source.size() && isdigit(source[cursor])) {
        increment();
      }

      // If a number is found
      if (previous != cursor) {

        // If its a float, continue to find the rest after the radix
        if (cursor < source.size() && source[cursor] == '.') {
          increment();
          while (cursor < source.size() && isdigit(source[cursor])) {
            increment();
          }
          create_token(TokenType::Float, start);
          continue;

          // otherwise just create the int
        } else {
          create_token(TokenType::Int, start);
          continue;
        }
      }

      // Identifiers and Keywords
      if (cursor < source.size() && isalpha(source[cursor])) {
        while (cursor < source.size() &&
               (isalnum(source[cursor]) || source[cursor] == '_')) {
          increment();
        }

        create_token(get_tokentype_for_keyword_or_ident(
                         source.substr(start.pos, cursor - start.pos)),
                     start);
        continue;
      }

      // If we reach here, it means we have an unknown character
      Location error_loc = {row, col, cursor, file_name};
      Span error_span = {error_loc, error_loc};
      LOG_ERROR_EXIT("[lexer] Unknown character '" +
                         std::string(1, source[cursor]) + "' at " +
                         std::to_string(row) + ":" + std::to_string(col),
                     error_span);
      increment();
      continue;
    }
    }
  }

  // Add EOF token at the end
  Location eof_start = {row, col, cursor, file_name};
  tokens.push_back(Token{TokenType::Eof, "", {eof_start, eof_start}});

  return tokens;
}