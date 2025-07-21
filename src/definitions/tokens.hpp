#pragma once
#include "position.hpp"
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <string>

#include <format>
#include <ostream>     // for std::ostream
#include <string>      // for std::string
#include <string_view> // for std::string_view
enum class TokenType {

  // Keywords
  Let,
  Extern,
  Import,
  If,
  Else,
  True,
  False,
  While,
  Return,
  Define, // for 'define' keyword

  // Type keywords
  IntType,
  FloatType,
  StringType,
  BoolType,
  VoidType,
  CharType,
  EnumType,
  StructType,
  TypeType,

  // Characters
  LParens,
  RParens,
  LCurly,
  RCurly,
  LSquare,
  RSquare,
  Comma,
  Dot,
  Pipe,
  Colon,
  Semicolon,
  Equals,
  Plus,
  Minus,
  Asterisk,
  Slash,
  LessThan,
  LessThanEquals,
  GreaterThan,
  GreaterThanEquals,
  EqualsEquals,
  NotEquals,
  Exclamation,
  Ampersand,

  // Literal types
  Identifier,
  String,
  Int,
  Float,
  Char,
  Enum,

  // Misc
  Arrow,
  From,

  // Meta
  Eof,
};

struct Token {
  TokenType type;
  std::string_view value;
  Span span;
};

inline TokenType from_char(char c) {
  switch (c) {
  case '(':
    return TokenType::LParens;
  case ')':
    return TokenType::RParens;
  case '{':
    return TokenType::LCurly;
  case '}':
    return TokenType::RCurly;
  case '[':
    return TokenType::LSquare;
  case ']':
    return TokenType::RSquare;
  case ',':
    return TokenType::Comma;
  case '.':
    return TokenType::Dot;
  case '|':
    return TokenType::Pipe;
  case ':':
    return TokenType::Colon;
  case ';':
    return TokenType::Semicolon;
  case '=':
    return TokenType::Equals;

  default:
    return TokenType::Eof; // Or TokenType::Identifier if you prefe
  }
}