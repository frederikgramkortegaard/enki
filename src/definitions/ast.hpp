#pragma once
#include "position.hpp"
#include "tokens.hpp"
#include <string_view>
#include <spdlog/spdlog.h>

struct ModuleContext; // Forward declaration for module context
struct Block;         // Forward declaration for Block

#include "types.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

template <typename T> using Ref = std::shared_ptr<T>;

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
  FunctionDefinition,
  Extern,
  Import,
  ExpressionStatement,
  Assignment,
  Block,
  If,
  While,
  Return,
  EnumDefinition,
  StructDefinition,
  StructInstantiation,
  Dot,
  Dereference,
  AddressOf,
  Unknown
};

struct ASTNode {
  Span span;
  virtual ~ASTNode() = default;
  virtual ASTType get_type() const = 0;
  virtual bool is_global_decl() const {
    // NOTE: VarDecl isn't a global decl for now, this is a hack for cpp codegen
    auto res = get_type() == ASTType::FunctionDefinition ||
           get_type() == ASTType::Extern ||
           get_type() == ASTType::EnumDefinition ||
           get_type() == ASTType::StructDefinition;
    spdlog::debug("[ASTNode] is_global_decl: {} for type {}",
                res, magic_enum::enum_name(get_type()));
    return res;
  }
};

struct Expression : ASTNode {
  Ref<Type> etype; // Type of the expression, set during type checking
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
  ASTType get_type() const override { return ASTType::Call; }
};

struct Assignment : Statement {
  Ref<Expression> assignee;
  Ref<Expression> expression;
  ASTType get_type() const override { return ASTType::Assignment; }
};

struct StructInstantiation : Expression {
  Ref<Identifier> identifier;
  Ref<Struct> struct_type;
  std::vector<Ref<Expression>> arguments;
  ASTType get_type() const override { return ASTType::StructInstantiation; }
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

struct Dereference : Expression {
  Ref<Expression> expression;
  ASTType get_type() const override { return ASTType::Dereference; }
};

struct AddressOf : Expression {
  Ref<Expression> expression;
  ASTType get_type() const override { return ASTType::AddressOf; }
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
  Ref<Type> type;
  std::string_view value;
  ASTType get_type() const override { return ASTType::Literal; }
};

struct Return : Statement {
  Ref<Expression> expression;
  Span span;
  Ref<Type> type;
  Ref<Function> function;
  ASTType get_type() const override { return ASTType::Return; }
};

// Honestly, I'm not sure its worth it to create e..g StructDefinition,
// EnumDefinition etc, we can just use the AST node it starts at as a reference
// inside ther Structures (e.g. Function, Struct, from types.hpp) but for now,
// we're doing it.
struct StructDefinition : Statement {
  Ref<Identifier> identifier;
  std::vector<Ref<Variable>> fields;
  Ref<Struct> struct_type;
  ASTType get_type() const override { return ASTType::StructDefinition; }
};

struct Program {
  Span span;
  Ref<Block> body; // Global block containing all statements
  Ref<Scope> scope = std::make_shared<Scope>(); // Global Scope
  std::shared_ptr<std::string> source_buffer;   // Holds the source buffer
  Ref<ModuleContext> module_context; // Holds the module context for lifetime
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
  Ref<Function> function;

  ASTType get_type() const override { return ASTType::FunctionDefinition; }
};

struct Dot : Expression {
  Ref<Expression> left;
  Ref<Expression> right;
  ASTType get_type() const override { return ASTType::Dot; }
};

struct EnumDefinition : Statement {
  Ref<Identifier> identifier;
  std::vector<Ref<Variable>> members;
  Ref<Type> enum_type;
  Ref<FunctionDefinition> to_string_function; // Optional, for enum-to-string

  ASTType get_type() const override { return ASTType::EnumDefinition; }
};

inline BaseType token_to_literal_type(TokenType type) {
  switch (type) {
  case TokenType::Int:
    return BaseType::Int;
  case TokenType::Float:
    return BaseType::Float;
  case TokenType::String:
    return BaseType::String;
  case TokenType::Char:
    return BaseType::Char;
  default:
    throw std::invalid_argument("TokenType is not a literal");
  }
}

struct ExpressionStatement : Statement {
  Ref<Expression> expression;
  ASTType get_type() const override { return ASTType::ExpressionStatement; }
};