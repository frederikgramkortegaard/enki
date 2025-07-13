#pragma once
#include "span.hpp"
#include "symbols.hpp"
#include "tokens.hpp"
#include <string_view>

#include "types.hpp"
#include <unordered_map>
#include <vector>

enum class ASTType {
  StringLiteral,
  FloatLiteral,
  IntLiteral,
  Identifier,
  VarDecl,
  Call,
  UnaryOp,
  BinaryOp,
  Literal,
  FunctionCall,
  FunctionDefinition,
  Extern,
};



struct Statement : Spanned {
  ASTType ast_type;
  virtual ~Statement() = default;
};

struct Expression : Spanned {
  ASTType ast_type;
  virtual ~Expression() = default;
};

struct Identifier : Expression {
  std::string_view name;
  Identifier() { ast_type = ASTType::Identifier; }
};



struct ExternStatement : Statement {
  std::shared_ptr<Identifier> identifier;
  std::vector<std::shared_ptr<Type>> args;
  std::shared_ptr<Type> return_type;
  std::string_view module_path;
  ExternStatement() { ast_type = ASTType::Extern; }
};

struct CallExpression : Expression {
  std::shared_ptr<Expression> callee;
  std::vector<std::shared_ptr<Expression>> arguments;
  CallExpression() { ast_type = ASTType::FunctionCall; }
};

struct LetStatement : Statement {
  std::shared_ptr<Expression> identifier;
  std::shared_ptr<Expression> expression;
};

struct Literal : Expression {
  Type type;
  std::string value;
  Literal() { ast_type = ASTType::Literal; }
};

struct Program : Spanned {
  std::vector<std::shared_ptr<Statement>> statements;
  std::unordered_map<std::string_view, Symbol> symbols;
};

inline BaseType token_to_literal_type(TokenType type) {
  switch (type) {
  case TokenType::Int:
    return BaseType::Int;
  case TokenType::Float:
    return BaseType::Float;
  case TokenType::String:
    return BaseType::String;
  default:
    throw std::invalid_argument("TokenType is not a literal");
  }
}

struct ExpressionStatement : Statement {
  std::shared_ptr<Expression> expression;
  ExpressionStatement() = default;
  ExpressionStatement(std::shared_ptr<Expression> expr)
      : expression(std::move(expr)) {}
};