#pragma once
#include "../definitions/ast.hpp"
#include "../definitions/serializations.hpp"
#include "../definitions/types.hpp"
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace utils {
namespace ast {

inline void print_indent(int depth) {
  for (int i = 0; i < depth; ++i)
    std::cout << "  ";
}

// Forward declarations
void print_ast(const std::shared_ptr<Expression> &expr, int depth = 0,
               int max_depth = -1);
void print_ast(const std::shared_ptr<Statement> &stmt, int depth = 0,
               int max_depth = -1);
void print_ast(const Program &program, int depth = 0, int max_depth = -1);
void print_ast(const std::shared_ptr<Type> &type, int depth = 0);

// Print Type
inline void print_ast(const std::shared_ptr<Type> &type, int depth) {
  if (!type) {
    print_indent(depth);
    std::cout << "<null Type>" << std::endl;
    return;
  }
  print_indent(depth);
  std::cout << "Type: base_type=" << magic_enum::enum_name(type->base_type)
            << std::endl;
}

// Print Expression (polymorphic)
inline void print_ast(const std::shared_ptr<Expression> &expr, int depth,
                      int max_depth) {
  if (!expr) {
    print_indent(depth);
    std::cout << "<null>" << std::endl;
    return;
  }
  if (max_depth >= 0 && depth > max_depth)
    return;
  // Identifier
  if (auto ident = std::dynamic_pointer_cast<Identifier>(expr)) {
    print_indent(depth);
    std::cout << "Identifier: " << ident->name << std::endl;
    return;
  }
  // Literal
  if (auto lit = std::dynamic_pointer_cast<Literal>(expr)) {
    print_indent(depth);
    std::cout << "Literal: " << lit->value << std::endl;
    return;
  }
  // CallExpression
  if (auto call = std::dynamic_pointer_cast<CallExpression>(expr)) {
    print_indent(depth);
    std::cout << "CallExpression:" << std::endl;
    print_indent(depth + 1);
    std::cout << "callee:" << std::endl;
    print_ast(call->callee, depth + 2, max_depth);
    print_indent(depth + 1);
    std::cout << "arguments:" << std::endl;
    for (const auto &arg : call->arguments) {
      print_ast(arg, depth + 2, max_depth);
    }
    return;
  }
  // Unknown type
  print_indent(depth);
  std::cout << "<Unknown Expression Type>" << std::endl;
}

// Print Statement (polymorphic)
inline void print_ast(const std::shared_ptr<Statement> &stmt, int depth,
                      int max_depth) {
  if (!stmt) {
    print_indent(depth);
    std::cout << "<null>" << std::endl;
    return;
  }
  if (max_depth >= 0 && depth > max_depth)
    return;
  // LetStatement
  if (auto let = std::dynamic_pointer_cast<LetStatement>(stmt)) {
    print_indent(depth);
    std::cout << "LetStatement:" << std::endl;
    print_indent(depth + 1);
    std::cout << "identifier:" << std::endl;
    print_ast(let->identifier, depth + 2, max_depth);
    print_indent(depth + 1);
    std::cout << "expression:" << std::endl;
    print_ast(let->expression, depth + 2, max_depth);
    return;
  }
  // ExpressionStatement
  if (auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
    print_indent(depth);
    std::cout << "ExpressionStatement:" << std::endl;
    print_indent(depth + 1);
    std::cout << "expression:" << std::endl;
    print_ast(expr_stmt->expression, depth + 2, max_depth);
    return;
  }
  // Unknown type
  print_indent(depth);
  std::cout << "<Unknown Statement Type>" << std::endl;
}

// Print Program
inline void print_ast(const Program &program, int depth, int max_depth) {
  print_indent(depth);
  std::cout << "Program" << std::endl;
  for (const auto &stmt : program.statements) {
    print_ast(stmt, depth + 1, max_depth);
  }
  // Print symbols (if any)
  if (!program.symbols.empty()) {
    print_indent(depth + 1);
    std::cout << "Symbols:" << std::endl;
    for (const auto &[name, sym] : program.symbols) {
      print_indent(depth + 2);
      std::cout << "Symbol: " << name << std::endl;
      print_indent(depth + 3);
      std::cout << "type:" << std::endl;
      print_ast(sym.type, depth + 4);
      print_indent(depth + 3);
      std::cout << "span: ..." << std::endl;
      // Optionally print value, etc.
    }
  }
}

} // namespace ast
} // namespace utils