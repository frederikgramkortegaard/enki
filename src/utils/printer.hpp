#pragma once
#include "../definitions/ast.hpp"
#include "../definitions/serializations.hpp"
#include "../definitions/types.hpp"
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string>

namespace utils {
namespace ast {

inline void print_indent(int depth) {
  for (int i = 0; i < depth; ++i)
    std::cout << "  ";
}

// Forward declarations
void print_scope(const Ref<Scope> &scope, int depth);
void print_symbol(const Ref<Symbol> &symbol, int depth);
void print_ast(const Ref<Expression> &expr, int depth = 0, int max_depth = -1);
void print_ast(const Ref<Statement> &stmt, int depth = 0, int max_depth = -1);
void print_ast(const Program &program, int depth = 0, int max_depth = -1);
void print_ast(const Ref<Type> &type, int depth = 0);

// Print Type
inline void print_ast(const Ref<Type> &type, int depth) {
  if (!type) {
    print_indent(depth);
    std::cout << "<null Type>" << std::endl;
    return;
  }
  print_indent(depth);
  std::cout << "Type: base_type=" << magic_enum::enum_name(type->base_type);

  // Handle enum and struct members
  if (type->base_type == BaseType::Enum) {
    try {
      const auto &enum_data = std::get<Ref<Enum>>(type->structure);
      std::cout << " (enum: " << enum_data->name << ")" << std::endl;
      print_indent(depth + 1);
      std::cout << "members:" << std::endl;
      for (const auto &[name, member] : enum_data->members) {
        print_indent(depth + 2);
        std::cout << "Member: " << name << std::endl;
      }
    } catch (const std::bad_variant_access &) {
      std::cout << " (enum data not available)" << std::endl;
    }
  } else if (type->base_type == BaseType::Struct) {
    try {
      const auto &struct_data = std::get<Ref<Struct>>(type->structure);
      std::cout << " (struct: " << struct_data->name << ")" << std::endl;
      print_indent(depth + 1);
      std::cout << "fields:" << std::endl;
      for (const auto &field : struct_data->fields) {
        print_indent(depth + 2);
        std::cout << "Field: " << field->name << std::endl;
      }
    } catch (const std::bad_variant_access &) {
      std::cout << " (struct data not available)" << std::endl;
    }
  } else if (type->base_type == BaseType::Function) {
    try {
      const auto &func_data = std::get<Ref<Function>>(type->structure);
      std::cout << " (function: " << func_data->name << ")" << std::endl;
    } catch (const std::bad_variant_access &) {
      std::cout << " (function data not available)" << std::endl;
    }
  } else {
    std::cout << std::endl;
  }
}

// Print Enum
inline void print_ast(const Ref<Enum> &enum_type, int depth = 0) {
  if (!enum_type) {
    print_indent(depth);
    std::cout << "<null Enum>" << std::endl;
    return;
  }
  print_indent(depth);
  std::cout << "Enum: " << enum_type->name << std::endl;
  print_indent(depth + 1);
  std::cout << "members:" << std::endl;
  for (const auto &[name, member] : enum_type->members) {
    print_indent(depth + 2);
    std::cout << "Member: " << name << std::endl;
  }
}

// Print Struct
inline void print_ast(const Ref<Struct> &struct_type, int depth = 0) {
  if (!struct_type) {
    print_indent(depth);
    std::cout << "<null Struct>" << std::endl;
    return;
  }
  print_indent(depth);
  std::cout << "Struct: " << struct_type->name << std::endl;
  print_indent(depth + 1);
  std::cout << "fields:" << std::endl;
  for (const auto &field : struct_type->fields) {
    print_indent(depth + 2);
    std::cout << "Field: " << field->name << std::endl;
  }
}

// Print Expression (polymorphic)
inline void print_ast(const Ref<Expression> &expr, int depth, int max_depth) {
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
  // Call
  if (auto call = std::dynamic_pointer_cast<Call>(expr)) {
    print_indent(depth);
    std::cout << "Call:" << std::endl;
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
  // BinaryOp
  if (auto bin_op = std::dynamic_pointer_cast<BinaryOp>(expr)) {
    print_indent(depth);
    std::cout << "BinaryOp: " << magic_enum::enum_name(bin_op->op) << std::endl;
    print_indent(depth + 1);
    std::cout << "left:" << std::endl;
    print_ast(bin_op->left, depth + 2, max_depth);
    print_indent(depth + 1);
    std::cout << "right:" << std::endl;
    print_ast(bin_op->right, depth + 2, max_depth);
    return;
  }
  // Dot
  if (auto dot_expr = std::dynamic_pointer_cast<Dot>(expr)) {
    print_indent(depth);
    std::cout << "Dot:" << std::endl;
    print_indent(depth + 1);
    std::cout << "left:" << std::endl;
    print_ast(dot_expr->left, depth + 2, max_depth);
    print_indent(depth + 1);
    std::cout << "right:" << std::endl;
    print_ast(dot_expr->right, depth + 2, max_depth);
    return;
  }
  // StructInstantiation
  if (auto struct_inst = std::dynamic_pointer_cast<StructInstantiation>(expr)) {
    print_indent(depth);
    std::cout << "StructInstantiation:" << std::endl;
    print_indent(depth + 1);
    std::cout << "struct_type:" << std::endl;
    print_ast(struct_inst->struct_type, depth + 2);
    print_indent(depth + 1);
    std::cout << "arguments:" << std::endl;
    for (const auto &arg : struct_inst->arguments) {
      print_ast(arg, depth + 2, max_depth);
    }
    return;
  }
  // Unknown type
  print_indent(depth);
  std::cout << magic_enum::enum_name(expr->get_type()) << std::endl;
  std::cout << "<Unknown Expression Type>" << std::endl;
}

// Print Statement (polymorphic)
inline void print_ast(const Ref<Statement> &stmt, int depth, int max_depth) {
  if (!stmt) {
    print_indent(depth);
    std::cout << "<null>" << std::endl;
    return;
  }
  if (max_depth >= 0 && depth > max_depth)
    return;
  // VarDecl
  if (auto let = std::dynamic_pointer_cast<VarDecl>(stmt)) {
    print_indent(depth);
    std::cout << "VarDecl:" << std::endl;
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
  // Extern
  if (auto ext = std::dynamic_pointer_cast<Extern>(stmt)) {
    print_indent(depth);
    std::cout << "Extern:" << std::endl;
    print_indent(depth + 1);
    std::cout << "identifier:" << std::endl;
    print_ast(ext->identifier, depth + 2, max_depth);
    print_indent(depth + 1);
    std::cout << "args:" << std::endl;
    for (const auto &arg : ext->args) {
      print_ast(arg, depth + 2);
    }
    print_indent(depth + 1);
    std::cout << "return_type:" << std::endl;
    print_ast(ext->return_type, depth + 2);
    print_indent(depth + 1);
    std::cout << "module_path: " << ext->module_path << std::endl;
    print_indent(depth + 1);
    std::cout << "span: ..." << std::endl;
    return;
  }
  // If
  if (auto if_stmt = std::dynamic_pointer_cast<If>(stmt)) {
    print_indent(depth);
    std::cout << "If:" << std::endl;
    print_indent(depth + 1);
    std::cout << "condition:" << std::endl;
    print_ast(if_stmt->condition, depth + 2, max_depth);
    print_indent(depth + 1);
    std::cout << "then_branch:" << std::endl;
    print_ast(if_stmt->then_branch, depth + 2, max_depth);
    if (if_stmt->else_branch) {
      print_indent(depth + 1);
      std::cout << "else_branch:" << std::endl;
      print_ast(if_stmt->else_branch, depth + 2, max_depth);
    }
    return;
  }
  // While
  if (auto while_stmt = std::dynamic_pointer_cast<While>(stmt)) {
    print_indent(depth);
    std::cout << "While:" << std::endl;
    print_indent(depth + 1);
    std::cout << "condition:" << std::endl;
    print_ast(while_stmt->condition, depth + 2, max_depth);
    print_indent(depth + 1);
    std::cout << "body:" << std::endl;
    print_ast(while_stmt->body, depth + 2, max_depth);
    return;
  }
  // Block
  if (auto block = std::dynamic_pointer_cast<Block>(stmt)) {
    print_indent(depth);
    std::cout << "Block:" << std::endl;
    if (block->scope) {
      print_scope(block->scope, depth + 1);
    }
    for (const auto &inner_stmt : block->statements) {
      print_ast(inner_stmt, depth + 1, max_depth);
    }
    return;
  }
  // Import
  if (auto import_stmt = std::dynamic_pointer_cast<Import>(stmt)) {
    print_indent(depth);
    std::cout << "Import:" << std::endl;
    print_indent(depth + 1);
    std::cout << "module_path:" << std::endl;
    print_ast(import_stmt->module_path, depth + 2, max_depth);
    return;
  }
  // FunctionDefinition
  if (auto func_def = std::dynamic_pointer_cast<FunctionDefinition>(stmt)) {
    print_indent(depth);
    std::cout << "FunctionDefinition:" << std::endl;
    print_indent(depth + 1);
    std::cout << "define" << std::endl;
    print_indent(depth + 1);
    std::cout << "identifier:" << std::endl;
    print_ast(func_def->identifier, depth + 2, max_depth);
    print_indent(depth + 1);
    std::cout << "args:" << std::endl;
    for (const auto &arg : func_def->parameters) {
      print_ast(arg, depth + 2);
    }
    print_indent(depth + 1);
    std::cout << "return_type:" << std::endl;
    print_ast(func_def->return_type, depth + 2);
    print_indent(depth + 1);
    std::cout << "body:" << std::endl;
    print_ast(func_def->body, depth + 2, max_depth);
    return;
  }
  // EnumDefinition
  if (auto enum_def = std::dynamic_pointer_cast<EnumDefinition>(stmt)) {
    print_indent(depth);
    std::cout << "EnumDefinition:" << std::endl;
    print_indent(depth + 1);
    std::cout << "identifier:" << std::endl;
    print_ast(enum_def->identifier, depth + 2, max_depth);
    print_indent(depth + 1);
    std::cout << "members:" << std::endl;
    for (const auto &member : enum_def->members) {
      print_indent(depth + 2);
      std::cout << "Member: " << member->name << std::endl;
    }
    print_indent(depth + 1);
    std::cout << "enum_type:" << std::endl;
    print_ast(enum_def->enum_type, depth + 2);
    return;
  }
  // StructDefinition
  if (auto struct_def = std::dynamic_pointer_cast<StructDefinition>(stmt)) {
    print_indent(depth);
    std::cout << "StructDefinition:" << std::endl;
    print_indent(depth + 1);
    std::cout << "identifier:" << std::endl;
    print_ast(struct_def->identifier, depth + 2, max_depth);
    print_indent(depth + 1);
    std::cout << "fields:" << std::endl;
    for (const auto &field : struct_def->fields) {
      print_indent(depth + 2);
      std::cout << "Field: " << field->name << std::endl;
    }
    print_indent(depth + 1);
    std::cout << "struct_type:" << std::endl;
    print_ast(struct_def->struct_type, depth + 2);
    return;
  }
  // Assignment
  if (auto assign = std::dynamic_pointer_cast<Assignment>(stmt)) {
    print_indent(depth);
    std::cout << "Assignment:" << std::endl;
    print_indent(depth + 1);
    std::cout << "assignee:" << std::endl;
    print_ast(assign->assignee, depth + 2, max_depth);
    print_indent(depth + 1);
    std::cout << "expression:" << std::endl;
    print_ast(assign->expression, depth + 2, max_depth);
    return;
  }
  // Return
  if (auto ret_stmt = std::dynamic_pointer_cast<Return>(stmt)) {
    print_indent(depth);
    std::cout << "Return" << std::endl;
    if (ret_stmt->expression) {
      print_ast(ret_stmt->expression, depth + 1, max_depth);
    }
    return;
  }
  // Unknown type
  print_indent(depth);
  std::cout << magic_enum::enum_name(stmt->get_type()) << std::endl;
  std::cout << "<Unknown Statement Type>" << std::endl;
}

// Print Program
inline void print_ast(const Program &program, int depth, int max_depth) {
  print_indent(depth);
  std::cout << "Program" << std::endl;
  if (program.scope) {
    print_scope(program.scope, depth + 1);
  }
  if (program.body) {
    print_ast(program.body, depth + 1, max_depth);
  }
}

// Overload to print a shared_ptr<Program>
inline void print_ast(const Ref<Program> &program, int depth = 0,
                      int max_depth = -1) {
  if (program)
    print_ast(*program, depth, max_depth);
  else
    std::cout << "<null Program>" << std::endl;
}

// Utility to print a Symbol
inline void print_symbol(const Ref<Symbol> &symbol, int depth) {
  if (!symbol) {
    print_indent(depth);
    std::cout << "Symbol: <null>" << std::endl;
    return;
  }
  print_indent(depth);
  std::cout << "Symbol: " << symbol->name << " ("
            << magic_enum::enum_name(symbol->symbol_type) << ")";
  if (symbol->type) {
    std::cout << ", type: ";
    print_ast(symbol->type, 0);
  } else {
    std::cout << ", type: <null>";
  }
  std::cout << std::endl;
}
// Utility to print a Scope
inline void print_scope(const Ref<Scope> &scope, int depth) {
  if (!scope) {
    print_indent(depth);
    std::cout << "Scope: <null>" << std::endl;
    return;
  }
  print_indent(depth);
  std::cout << "Scope:" << std::endl;
  for (const auto &[name, symbol] : scope->symbols) {
    if (symbol)
      print_symbol(symbol, depth + 1);
    else {
      print_indent(depth + 1);
      std::cout << "Symbol: <null>" << std::endl;
    }
  }
  for (const auto &child : scope->children) {
    if (child)
      print_scope(child, depth + 1);
    else {
      print_indent(depth + 1);
      std::cout << "Scope: <null>" << std::endl;
    }
  }
}

} // namespace ast
} // namespace utils