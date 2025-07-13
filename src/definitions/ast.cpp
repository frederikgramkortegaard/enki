#include "ast.hpp"

int binary_op_precedence(BinaryOpType op) {
  switch (op) {
  case BinaryOpType::Multiply:
  case BinaryOpType::Divide:
  case BinaryOpType::Modulo:
    return 10;
  case BinaryOpType::Add:
  case BinaryOpType::Subtract:
    return 20;
  case BinaryOpType::LessThan:
  case BinaryOpType::GreaterThan:
  case BinaryOpType::LessThanOrEqual:
  case BinaryOpType::GreaterThanOrEqual:
    return 30;
  case BinaryOpType::Equals:
  case BinaryOpType::NotEquals:
    return 40;
  default:
    return -1; // Invalid operator
  }
}

std::optional<BinaryOpType> token_to_binop(TokenType type) {
  switch (type) {
  case TokenType::Plus:
    return BinaryOpType::Add;
  case TokenType::Minus:
    return BinaryOpType::Subtract;
  case TokenType::Asterisk:
    return BinaryOpType::Multiply;
  case TokenType::Slash:
    return BinaryOpType::Divide;
  case TokenType::LessThan:
    return BinaryOpType::LessThan;
  case TokenType::GreaterThan:
    return BinaryOpType::GreaterThan;
  case TokenType::LessThanEquals:
    return BinaryOpType::LessThanOrEqual;
  case TokenType::GreaterThanEquals:
    return BinaryOpType::GreaterThanOrEqual;
  case TokenType::EqualsEquals:
    return BinaryOpType::Equals;
  case TokenType::NotEquals:
    return BinaryOpType::NotEquals;
  default:
    return {}; // No matching binary operator
  }
}