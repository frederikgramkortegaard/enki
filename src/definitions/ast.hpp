#pragma once
#include "position.hpp"
#include "tokens.hpp"
#include <string_view>

#include "types.hpp"
#include <unordered_map>
#include <vector>
#include <memory>

template <typename T>
using Ref = std::shared_ptr<T>;

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
  ExpressionStatement,
  Assigment,
  Block,
  If,
  While,
  Return
};



struct ASTNode {
  Span span;
  virtual ~ASTNode() = default;
  virtual ASTType get_type() const = 0;
};

struct Expression : ASTNode {
  virtual ~Expression() = default;
  virtual ASTType get_type() const = 0;
};

struct Statement : ASTNode {
  virtual ~Statement() = default;
  virtual ASTType get_type() const = 0;
};

struct Identifier : Expression {
  std::string_view name;
  ASTType get_type() const override { return ASTType::Identifier; }
};

struct Extern : Statement {
  Ref<Identifier> identifier;
  std::vector<Ref<Type>> args;
  Ref<Type> return_type;
  std::string_view module_path;
  ASTType get_type() const override { return ASTType::Extern; }
};

struct Call : Expression {
  Ref<Expression> callee;
  std::vector<Ref<Expression>> arguments;
  ASTType get_type() const override { return ASTType::FunctionCall; }
};

struct Assigment : Statement {
  Ref<Expression> assignee;
  Ref<Expression> expression;
  ASTType get_type() const override { return ASTType::Assigment; }
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
  Ref<Expression> left;
  Ref<Expression> right;
  BinaryOpType op;
  ASTType get_type() const override { return ASTType::BinaryOp; }
};

struct VarDecl : Statement {
  Ref<Identifier> identifier;
  Ref<Type> type;
  Ref<Expression> expression;
  ASTType get_type() const override { return ASTType::VarDecl; }
};

struct If : Statement {
  Ref<Expression> condition;
  Ref<Statement> then_branch;
  Ref<Statement> else_branch;
  ASTType get_type() const override { return ASTType::If; }
};

struct While : Statement {
  Ref<Expression> condition;
  Ref<Statement> body;
  ASTType get_type() const override { return ASTType::While; }
};

struct Block : Statement {
  std::vector<Ref<Statement>> statements;
  Ref<Scope> scope;
  ASTType get_type() const override { return ASTType::Block; }
};

struct Literal : Expression {
  Type type;
  std::string value;
  ASTType get_type() const override { return ASTType::Literal; }
};

struct Return : Statement {
  Ref<Expression> expression;
  ASTType get_type() const override { return ASTType::Return; }
};

struct Program {
  Span span;
  std::vector<Ref<Statement>> statements;
  Ref<Scope> scope = std::make_shared<Scope>(); // Global Scope
  std::unordered_map<std::string, Ref<Function>> functions;

};

struct Import : Statement {
  Ref<Literal> module_path;
  ASTType get_type() const override { return ASTType::Import; }
};

using Parameter = VarDecl;

struct FunctionDefinition : Statement {
  Ref<Identifier> identifier;
  Ref<Type> return_type;
  std::vector<Ref<Parameter>> parameters;
  std::vector<Ref<Return>> returns;
  Ref<Block> body;

  ASTType get_type() const override { return ASTType::FunctionDefinition; }
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
  Ref<Expression> expression;
  ASTType get_type() const override { return ASTType::ExpressionStatement; }
};