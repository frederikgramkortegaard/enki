#include "lexer.hpp"
#include "../definitions/tokens.hpp"
#include <format>
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <vector>

TokenType get_tokentype_for_keyword_or_ident(const std::string_view &str) {
  if (str == "let") {
    return TokenType::Let;
  } else if (str == "extern") {
    return TokenType::Extern;
  } else if (str == "import") {
    return TokenType::Import;
  } else if (str == "from") {
    return TokenType::From;
  }
  if (str == "if") {
    return TokenType::If;
  }
  if (str == "else") {
    return TokenType::Else;
  }
  if (str == "true") {
    return TokenType::True;
  }
  if (str == "false") {
    return TokenType::False;
  }
  if (str == "while") {
    return TokenType::While;
  }
  if (str == "define") {
    return TokenType::Define;
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
    spdlog::debug("Created token of type {} with value '{}'",
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
    case '(': simple_token(TokenType::LParens); continue;
    case ')': simple_token(TokenType::RParens); continue;
    case '[': simple_token(TokenType::LSquare); continue;
    case ']': simple_token(TokenType::RSquare); continue;
    case '{': simple_token(TokenType::LCurly); continue;
    case '}': simple_token(TokenType::RCurly); continue;
    case ',': simple_token(TokenType::Comma); continue;
    case '.': simple_token(TokenType::Dot); continue;
    case '|': simple_token(TokenType::Pipe); continue;
    case ':': simple_token(TokenType::Colon); continue;
    case ';': simple_token(TokenType::Semicolon); continue;
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
    case '+': simple_token(TokenType::Plus); continue;
      case '-': {
      if (cursor + 1 < source.size() && source[cursor + 1] == '>') {
        simple_token(TokenType::Arrow, 2);
      } else {
        simple_token(TokenType::Minus);
      }
      continue;
    }
    case '*': simple_token(TokenType::Asterisk); continue;
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
      spdlog::debug("Found string literal at {}:{}", row, col);
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

      // If we reach here, it means we have an unknown character
      std::cerr << "Unknown character '" << source[cursor]
                << "' at " << row << ":" << col << "\n";
      increment();
      continue;
    }
    }
  }
  return tokens;
}