#include "lexer.hpp"
#include "../definitions/tokens.hpp"
#include <format>
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <vector>

TokenType get_tokentype_for_keyword_or_ident(const std::string_view &str) {
  if (str == "let") {
    return TokenType::Let;
  }
  return TokenType::Identifier;
}

std::vector<Token> lex(const std::string_view &source,
                       const std::string_view &file_name) {
  std::vector<Token> tokens;

  int cursor = 0;
  int row = 0;
  int col = 0;
  auto create_token = [&](TokenType type, Location start) {
    std::string_view value_view = source.substr(start.pos, cursor - start.pos);
    tokens.push_back(Token{
        type, value_view, {start, Location{row, col, cursor, file_name}}});
    std::cout << std::format("Created token of type {} with value '{}'\n",
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
    case ')':
    case ',':
    case '.':
    case '|':
    case ':':
    case ';':
    case '=': {
      increment();
      create_token(from_char(source[start.pos]), start);
      continue;
    }

    // Single-line comments
    case '/': {
      if (cursor + 1 < source.size() && source[cursor + 1] == '/') {
        while (cursor < source.size() && source[cursor] != '\n') {
          increment();
        }
      }
      break;
    }

      // String literal
    case '"': {
      increment(); // skip opening quote
      while (cursor < source.size() && source[cursor] != '"') {
        if (source[cursor] == '\\' && cursor + 1 < source.size()) {
          increment(2);
        } else {
          increment();
        }
      }

      if (cursor >= source.size()) {
        std::cerr << "Unterminated string literal at " << row << ":" << col
                  << "\n";
      } else {
        start.pos += 1;
        start.col += 1;
        create_token(TokenType::String, start);
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
    }
    }
  }
  return tokens;
}