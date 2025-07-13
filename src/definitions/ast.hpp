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
  Import,
};



struct Statement : Spanned {
  ASTType ast_type;
  virtual ~Statement() = default;
};

struct Expression : Spanned {
  ASTType expr_type;
  virtual ~Expression() = default;
};

struct Identifier : Expression {
  std::string_view name;
  Identifier() { expr_type = ASTType::Identifier; }
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
  CallExpression() { expr_type = ASTType::FunctionCall; }
};

enum class BinaryOpType {
  Add,
  Subtract,
  Multiply,
  Divide,
  Modulo,
  Equals,
  NotEquals,
  LessThan,
  GreaterThan,
  LessThanOrEqual,
  GreaterThanOrEqual,
};

int binary_op_precedence(BinaryOpType op);
std::optional<BinaryOpType> token_to_binop(TokenType type);

struct BinaryOp : Expression {
  std::shared_ptr<Expression> left;
  std::shared_ptr<Expression> right;
  BinaryOpType op;
  BinaryOp() { expr_type = ASTType::BinaryOp; }
};

struct LetStatement : Statement {
  std::shared_ptr<Expression> identifier;
  std::shared_ptr<Expression> expression;
};

struct IfStatement : Statement {
  std::shared_ptr<Expression> condition;
  std::shared_ptr<Statement> then_branch;
  std::shared_ptr<Statement> else_branch;
};

struct WhileLoop : Statement {
  std::shared_ptr<Expression> condition;
  std::shared_ptr<Statement> body;
};

struct Block : Statement {
  std::vector<std::shared_ptr<Statement>> statements;
};

struct Literal : Expression {
  Type type;
  std::string value;
  Literal() { expr_type = ASTType::Literal; }
};

struct Program : Spanned {
  std::vector<std::shared_ptr<Statement>> statements;
  std::unordered_map<std::string_view, std::shared_ptr<Symbol>> symbols;
};

struct ImportStatement : Statement {
  std::shared_ptr<Literal> module_path;
  ImportStatement() { ast_type = ASTType::Import; }
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