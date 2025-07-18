#include "typecheck.hpp"
#include "../definitions/ast.hpp"
#include "../definitions/types.hpp"
#include "../utils/logging.hpp"
#include <spdlog/spdlog.h>

void typecheck_statement(Ref<TypecheckContext> ctx, Ref<Statement> stmt);
Ref<Type> typecheck_expression(Ref<TypecheckContext> ctx, Ref<Expression> expr);
void typecheck_function_definition(Ref<TypecheckContext> ctx,
                                   Ref<FunctionDefinition> func_def);

// Helper function to determine the result type of a binary operation
Ref<Type> get_binary_op_result_type(BinaryOpType op, Ref<Type> left_type, Ref<Type> right_type) {
  auto result_type = std::make_shared<Type>();
  
  switch (op) {
    // Arithmetic operations: return the type of the operands (must be numeric)
    case BinaryOpType::Add:
    case BinaryOpType::Subtract:
    case BinaryOpType::Multiply:
    case BinaryOpType::Divide:
    case BinaryOpType::Modulo:
      if (left_type->base_type == BaseType::Float || right_type->base_type == BaseType::Float) {
        result_type->base_type = BaseType::Float;
      } else if (left_type->base_type == BaseType::Int && right_type->base_type == BaseType::Int) {
        result_type->base_type = BaseType::Int;
      } else {
        // This should not happen if type checking is done properly
        result_type->base_type = BaseType::Int; // fallback
      }
      break;
      
    // Comparison operations: always return boolean
    case BinaryOpType::Equals:
    case BinaryOpType::NotEquals:
    case BinaryOpType::LessThan:
    case BinaryOpType::GreaterThan:
    case BinaryOpType::LessThanOrEqual:
    case BinaryOpType::GreaterThanOrEqual:
      result_type->base_type = BaseType::Bool;
      break;
      
    default:
      // This should not happen
      result_type->base_type = BaseType::Int;
      break;
  }
  
  return result_type;
}

// Helper function to check if a binary operation is valid for the given types
bool is_valid_binary_op(BinaryOpType op, Ref<Type> left_type, Ref<Type> right_type) {
  switch (op) {
    // Arithmetic operations: require numeric types (int or float, NOT char)
    case BinaryOpType::Add:
    case BinaryOpType::Subtract:
    case BinaryOpType::Multiply:
    case BinaryOpType::Divide:
    case BinaryOpType::Modulo:
      return (left_type->base_type == BaseType::Int || left_type->base_type == BaseType::Float) &&
             (right_type->base_type == BaseType::Int || right_type->base_type == BaseType::Float);
      
    // Comparison operations: require compatible types
    case BinaryOpType::Equals:
    case BinaryOpType::NotEquals:
      // Can compare any types for equality
      return true;
      
    case BinaryOpType::LessThan:
    case BinaryOpType::GreaterThan:
    case BinaryOpType::LessThanOrEqual:
    case BinaryOpType::GreaterThanOrEqual:
      // Can only compare numeric types (int or float, NOT char)
      return (left_type->base_type == BaseType::Int || left_type->base_type == BaseType::Float) &&
             (right_type->base_type == BaseType::Int || right_type->base_type == BaseType::Float);
      
    default:
      return false;
  }
}

// Helper function for scope chain traversal
Ref<Symbol> find_symbol_in_scope_chain(const Ref<Scope> &scope,
                                       const std::string_view &name) {
  auto current_scope = scope;
  int depth = 0;
  while (current_scope) {
    spdlog::debug("Searching for '{}' in scope at depth {}", name, depth);
    auto it = current_scope->symbols.find(std::string(name));
    if (it != current_scope->symbols.end() && it->second) {
      spdlog::debug("Found symbol '{}' in scope at depth {}", name, depth);
      return it->second;
    }
    current_scope = current_scope->parent;
    depth++;
  }
  spdlog::debug("Symbol '{}' not found in any scope", name);
  return nullptr;
}

Ref<Type> typecheck_identifier(Ref<TypecheckContext> ctx, Ref<Identifier> id) {
  spdlog::debug("[typechecker] typecheck_identifier: id = {}", id->name);
  auto symbol = find_symbol_in_scope_chain(ctx->current_scope(), id->name);
  if (!symbol) {
    LOG_ERROR_EXIT("[typechecker] Symbol not found: " + std::string(id->name), id->span, *ctx->program->source_buffer);
  }
  return symbol->type;
}

Ref<Type> typecheck_function_call(Ref<TypecheckContext> ctx, Ref<Call> call) {
  spdlog::debug(
      "[typechecker] typecheck_function_call: call type = {}",
      magic_enum::enum_name(call ? call->get_type() : ASTType::Unknown));

  auto function_name = std::static_pointer_cast<Identifier>(call->callee)->name;
  spdlog::debug("Looking for function: '{}' in current scope", function_name);

  // find function in symbols
  auto function_symbol =
      find_symbol_in_scope_chain(ctx->current_scope(), function_name);
  if (!function_symbol) {
    LOG_ERROR_EXIT(
        "[typechecker] Function not found: " +
        std::string(std::static_pointer_cast<Identifier>(call->callee)->name),
        call->span, *ctx->program->source_buffer);
  }
  if (function_symbol->symbol_type != SymbolType::Function) {
    LOG_ERROR_EXIT(
        "[typechecker] Symbol is not a function: " +
        std::string(std::static_pointer_cast<Identifier>(call->callee)->name),
        call->span, *ctx->program->source_buffer);
  }

  // For built-in functions like print, we accept any arguments for now
  if (function_symbol->name == "print") {
    // Typecheck all arguments (but don't enforce specific types for print)
    for (auto &arg : call->arguments) {
      typecheck_expression(ctx, arg);
    }
    return function_symbol->type; // Returns void
  }

  // TODO: For user-defined functions, check argument count and types
  // This will need to be implemented when we have proper function parameter
  // tracking

  return function_symbol->type;
}

Ref<Type> typecheck_binary_op(Ref<TypecheckContext> ctx, Ref<BinaryOp> bin_op) {
  spdlog::debug("[typechecker] typecheck_binary_op: op = {}",
                magic_enum::enum_name(bin_op->op));
  auto left_type = typecheck_expression(ctx, bin_op->left);
  auto right_type = typecheck_expression(ctx, bin_op->right);

  // Check if the binary operation is valid for the given types
  if (!is_valid_binary_op(bin_op->op, left_type, right_type)) {
    LOG_ERROR_EXIT(
        "[typechecker] Invalid binary operation: " +
        std::string(magic_enum::enum_name(bin_op->op)) + " between " +
        std::string(magic_enum::enum_name(left_type->base_type)) + " and " +
        std::string(magic_enum::enum_name(right_type->base_type)),
        bin_op->span, *ctx->program->source_buffer);
  }

  // Get the correct result type for this operation
  auto result_type = get_binary_op_result_type(bin_op->op, left_type, right_type);
  
  spdlog::debug("[typechecker] binary_op result type: {}",
                magic_enum::enum_name(result_type->base_type));
  
  return result_type;
}

Ref<Type> typecheck_var_decl(Ref<TypecheckContext> ctx, Ref<VarDecl> var_decl) {
  spdlog::debug("[typechecker] typecheck_var_decl: var_decl type = {}",
                magic_enum::enum_name(var_decl ? var_decl->get_type()
                                               : ASTType::Unknown));
  auto type = typecheck_expression(ctx, var_decl->expression);
  auto var_symbol = std::make_shared<Symbol>();
  var_symbol->name = var_decl->identifier->name;
  var_symbol->type = type;
  var_symbol->symbol_type = SymbolType::Variable;
  var_symbol->span = var_decl->span;
  ctx->current_scope()->symbols[var_decl->identifier->name] =
      std::static_pointer_cast<Symbol>(var_symbol);
  return type;
}

Ref<Type> typecheck_literal(Ref<TypecheckContext> ctx, Ref<Literal> lit) {
  return lit->type;
}

Ref<Type> typecheck_parameter(Ref<TypecheckContext> ctx, Ref<Parameter> param) {
  spdlog::debug(
      "typecheck_parameter: param type = {}",
      magic_enum::enum_name(param ? param->get_type() : ASTType::Unknown));
  return param->type;
}

Ref<Type> typecheck_expression(Ref<TypecheckContext> ctx,
                               Ref<Expression> expr) {
  spdlog::debug(
      "[typechecker] typecheck_expression: expr type = {}",
      magic_enum::enum_name(expr ? expr->get_type() : ASTType::Unknown));
  switch (expr->get_type()) {
  case ASTType::Identifier:
    return typecheck_identifier(ctx,
                                std::static_pointer_cast<Identifier>(expr));
  case ASTType::Literal:
    return typecheck_literal(ctx, std::static_pointer_cast<Literal>(expr));
  case ASTType::BinaryOp:
    return typecheck_binary_op(ctx, std::static_pointer_cast<BinaryOp>(expr));
  case ASTType::FunctionCall:
    return typecheck_function_call(ctx, std::static_pointer_cast<Call>(expr));
  default:
    LOG_ERROR_EXIT(
        "[typechecker] Unknown expression type: " +
        std::string(magic_enum::enum_name(expr->get_type())),
        expr->span, *ctx->program->source_buffer);
  }

  return nullptr;
}

void typecheck_block(Ref<TypecheckContext> ctx, Ref<Block> block) {
  spdlog::debug(
      "[typechecker] typecheck_block: block type = {}",
      magic_enum::enum_name(block ? block->get_type() : ASTType::Unknown));
  ctx->push_scope(block->scope);
  for (auto &stmt : block->statements) {
    typecheck_statement(ctx, std::static_pointer_cast<Statement>(stmt));
  }
  ctx->pop_scope();
}

void typecheck_return(Ref<TypecheckContext> ctx, Ref<Return> ret) {
  spdlog::debug("[typechecker] typecheck_return: processing return statement");

  auto current_func = ctx->current_function();
  if (!current_func) {
    LOG_ERROR_EXIT("[typechecker] Return statement outside of function", ret->span, *ctx->program->source_buffer);
  }
  // If function returns void, return must not have an expression
  if (current_func->return_type->base_type == BaseType::Void) {
    if (ret->expression != nullptr) {
      LOG_ERROR_EXIT("[typechecker] Cannot return a value from a void function", ret->span, *ctx->program->source_buffer);
    }
    ret->type = current_func->return_type;
    ret->function = current_func;
    spdlog::debug("[typechecker] typecheck_return: void function, no return value");
    return;
  }
  // Otherwise, must typecheck the expression and match types
  if (ret->expression == nullptr) {
    LOG_ERROR_EXIT("[typechecker] Missing return expression in non-void function", ret->span, *ctx->program->source_buffer);
  }
  auto return_type = typecheck_expression(ctx, ret->expression);
  if (return_type->base_type != current_func->return_type->base_type) {
    LOG_ERROR_EXIT(
        "[typechecker] Return type mismatch: " +
        std::string(magic_enum::enum_name(return_type->base_type)) + " != " +
        std::string(
            magic_enum::enum_name(current_func->return_type->base_type)),
        ret->span, *ctx->program->source_buffer);
  }
  ret->type = return_type;
  ret->function = current_func;
  spdlog::debug("[typechecker] typecheck_return: returning value of type {}",
                magic_enum::enum_name(return_type->base_type));
}

void typecheck_assignment(Ref<TypecheckContext> ctx,
                          Ref<Assignment> assignment) {
  spdlog::debug("[typechecker] typecheck_assignment: assignment type = {}",
                magic_enum::enum_name(assignment ? assignment->get_type()
                                                 : ASTType::Unknown));
  auto assignee_type = typecheck_expression(ctx, assignment->assignee);
  auto expression_type = typecheck_expression(ctx, assignment->expression);
  if (assignee_type->base_type != expression_type->base_type) {
    LOG_ERROR_EXIT(
        "[typechecker] Assignment type mismatch: " +
        std::string(magic_enum::enum_name(assignee_type->base_type)) + " != " +
        std::string(magic_enum::enum_name(expression_type->base_type)),
        assignment->span, *ctx->program->source_buffer);
  }

  //@TODO : This wont work the second we have arr indexes etc
  auto assignee_symbol = find_symbol_in_scope_chain(
      ctx->current_scope(),
      std::static_pointer_cast<Identifier>(assignment->assignee)->name);
  if (!assignee_symbol) {
    LOG_ERROR_EXIT(
        "[typechecker] Assignee symbol not found: " +
        std::string(
            std::static_pointer_cast<Identifier>(assignment->assignee)->name),
        assignment->span, *ctx->program->source_buffer);
  }
  assignee_symbol->type = expression_type;
  assignee_symbol->span = assignment->span;
  // Note: We're updating the symbol in the scope where it was found, not
  // necessarily the current scope This is correct behavior for nested scopes
}

void typecheck_if(Ref<TypecheckContext> ctx, Ref<If> if_stmt) {
  spdlog::debug(
      "[typechecker] typecheck_if: if_stmt type = {}",
      magic_enum::enum_name(if_stmt ? if_stmt->get_type() : ASTType::Unknown));

  // Typecheck the condition - must be boolean
  auto condition_type = typecheck_expression(ctx, if_stmt->condition);
  if (condition_type->base_type != BaseType::Bool) {
    LOG_ERROR_EXIT(
        "[typechecker] If condition must be boolean, got: " +
        std::string(magic_enum::enum_name(condition_type->base_type)),
        if_stmt->condition->span, *ctx->program->source_buffer);
  }

  // Typecheck the then branch
  typecheck_statement(ctx, if_stmt->then_branch);

  // Typecheck the else branch if it exists
  if (if_stmt->else_branch) {
    typecheck_statement(ctx, if_stmt->else_branch);
  }
}

void typecheck_while(Ref<TypecheckContext> ctx, Ref<While> while_stmt) {
  spdlog::debug("[typechecker] typecheck_while: while_stmt type = {}",
                magic_enum::enum_name(while_stmt ? while_stmt->get_type()
                                                 : ASTType::Unknown));

  // Typecheck the condition - must be boolean
  auto condition_type = typecheck_expression(ctx, while_stmt->condition);
  if (condition_type->base_type != BaseType::Bool) {
    LOG_ERROR_EXIT(
        "[typechecker] While condition must be boolean, got: " +
        std::string(magic_enum::enum_name(condition_type->base_type)),
        while_stmt->condition->span, *ctx->program->source_buffer);
  }

  // Typecheck the body
  typecheck_statement(ctx, while_stmt->body);
}

void typecheck_import(Ref<TypecheckContext> ctx, Ref<Import> import_stmt) {
  spdlog::debug("[typechecker] typecheck_import: import_stmt type = {}",
                magic_enum::enum_name(import_stmt ? import_stmt->get_type()
                                                  : ASTType::Unknown));
  // For now, just validate the module path is a string literal
  if (import_stmt->module_path->type->base_type != BaseType::String) {
    LOG_ERROR_EXIT("[typechecker] Import module path must be a string literal", import_stmt->span, *ctx->program->source_buffer);
  }
  // TODO: Actually resolve and import the module
}

void typecheck_extern(Ref<TypecheckContext> ctx, Ref<Extern> extern_stmt) {
  spdlog::debug("[typechecker] typecheck_extern: extern_stmt type = {}",
                magic_enum::enum_name(extern_stmt ? extern_stmt->get_type()
                                                  : ASTType::Unknown));

  // Register the extern function in the current scope
  auto func_symbol = std::make_shared<Symbol>();
  func_symbol->name = extern_stmt->identifier->name;
  func_symbol->type = extern_stmt->return_type;
  func_symbol->symbol_type = SymbolType::Function;
  func_symbol->span = extern_stmt->span;

  ctx->current_scope()->symbols[extern_stmt->identifier->name] = func_symbol;
}

void typecheck_statement(Ref<TypecheckContext> ctx, Ref<Statement> stmt) {
  spdlog::debug(
      "[typechecker] typecheck_statement: stmt type = {}",
      magic_enum::enum_name(stmt ? stmt->get_type() : ASTType::Unknown));
  switch (stmt->get_type()) {
  case ASTType::Block:
    typecheck_block(ctx, std::static_pointer_cast<Block>(stmt));
    break;
  case ASTType::ExpressionStatement:
    typecheck_expression(
        ctx, std::static_pointer_cast<ExpressionStatement>(stmt)->expression);
    break;
  case ASTType::VarDecl:
    typecheck_var_decl(ctx, std::static_pointer_cast<VarDecl>(stmt));
    break;
  case ASTType::FunctionDefinition:
    typecheck_function_definition(
        ctx, std::static_pointer_cast<FunctionDefinition>(stmt));
    break;
  case ASTType::Return:
    typecheck_return(ctx, std::static_pointer_cast<Return>(stmt));
    break;
  case ASTType::Assignment:
    typecheck_assignment(ctx, std::static_pointer_cast<Assignment>(stmt));
    break;
  case ASTType::If:
    typecheck_if(ctx, std::static_pointer_cast<If>(stmt));
    break;
  case ASTType::While:
    typecheck_while(ctx, std::static_pointer_cast<While>(stmt));
    break;
  case ASTType::Import:
    typecheck_import(ctx, std::static_pointer_cast<Import>(stmt));
    break;
  case ASTType::Extern:
    typecheck_extern(ctx, std::static_pointer_cast<Extern>(stmt));
    break;
  default:
    LOG_ERROR_EXIT(
        "[typechecker] Unknown statement type: " +
        std::string(magic_enum::enum_name(stmt->get_type())),
        stmt->span, *ctx->program->source_buffer);
  }
  return;
}

namespace {
int get_scope_depth(const Ref<Scope> &scope) {
  int depth = 0;
  auto curr = scope;
  while (curr && curr->parent) {
    ++depth;
    curr = curr->parent;
  }
  return depth;
}

void log_scope_symbols(const Ref<Scope> &scope, const std::string &context) {
  if (!scope) {
    spdlog::debug("[typecheck]   {}: Scope is null", context);
    return;
  }
  spdlog::debug("[typecheck]   {}: Scope depth = {}", context,
                get_scope_depth(scope));
  if (scope->symbols.empty()) {
    spdlog::debug("[typecheck]   {}: No symbols in scope", context);
    return;
  }
  for (const auto &[name, symbol] : scope->symbols) {
    if (symbol) {
      spdlog::debug(
          "[typecheck]   {}: Symbol '{}' (type: {}, kind: {})", context, name,
          symbol->type ? magic_enum::enum_name(symbol->type->base_type)
                       : "<null type>",
          magic_enum::enum_name(symbol->symbol_type));
    } else {
      spdlog::debug("[typecheck]   {}: Symbol '{}' is null", context, name);
    }
  }
}
} // namespace

void typecheck_function_definition(Ref<TypecheckContext> ctx,
                                   Ref<FunctionDefinition> func_def) {
  auto func_name =
      func_def && func_def->identifier ? func_def->identifier->name : "<null>";
  spdlog::debug("[typecheck] Entering function definition: {}", func_name);
  if (func_def && func_def->parameters.size() > 0) {
    for (const auto &param : func_def->parameters) {
      auto pname = param->identifier ? param->identifier->name : "<null>";
      spdlog::debug("[typecheck]   Parameter: {}", pname);
    }
  } else {
    spdlog::debug("[typecheck]   No parameters");
  }

  auto function = std::make_shared<Function>();
  ctx->current_scope()->functions[func_name] = function;
  function->scope = func_def->body->scope;
  function->name = func_name;
  function->span = func_def->span;
  function->return_type = func_def->return_type;
  function->definition = func_def;

  // Ensure the function scope has the current scope as its parent
  function->scope->parent = ctx->current_scope();

  ctx->push_function(function);
  ctx->push_scope(function->scope);
  spdlog::debug("[typecheck]   Entered function scope (depth = {})",
                get_scope_depth(function->scope));
  log_scope_symbols(ctx->current_scope(), "On scope entry");

  for (auto &param : func_def->parameters) {
    auto param_type = typecheck_parameter(ctx, param);
    auto param_variable = std::make_shared<Variable>();
    param_variable->name = param->identifier->name;
    param_variable->type = param_type;

    auto param_symbol = std::make_shared<Symbol>();
    param_symbol->name = param->identifier->name;
    param_symbol->type = param_variable->type;
    param_symbol->symbol_type = SymbolType::Variable;
    param_symbol->span = param->span;

    ctx->current_scope()->symbols[param->identifier->name] =
        std::static_pointer_cast<Symbol>(param_symbol);
    function->parameters.push_back(param_variable);
    spdlog::debug("[typecheck]     Added parameter symbol: {} (type: {})",
                  param_symbol->name,
                  param_symbol->type
                      ? magic_enum::enum_name(param_symbol->type->base_type)
                      : "<null type>");
    log_scope_symbols(ctx->current_scope(), "After adding parameter");
  }

  for (auto &stmt : func_def->body->statements) {
    typecheck_statement(ctx, std::static_pointer_cast<Statement>(stmt));
  }

  ctx->pop_function();
  ctx->pop_scope();
  spdlog::debug(
      "[typecheck]   Exited function scope for: {} (depth = {})", func_name,
      ctx->current_scope() ? get_scope_depth(ctx->current_scope()) : 0);
  if (ctx->current_scope())
    log_scope_symbols(ctx->current_scope(), "After exiting function scope");

  auto func_symbol = std::make_shared<Symbol>();
  func_symbol->name = func_def->identifier->name;
  func_symbol->type = function->return_type;
  func_symbol->symbol_type = SymbolType::Function;
  func_symbol->span = func_def->span;
  ctx->current_scope()->symbols[func_def->identifier->name] =
      std::static_pointer_cast<Symbol>(func_symbol);
  spdlog::debug("[typecheck] Registered function symbol: {} (type: {})",
                func_symbol->name,
                func_symbol->type
                    ? magic_enum::enum_name(func_symbol->type->base_type)
                    : "<null type>");
  log_scope_symbols(ctx->current_scope(), "After registering function symbol");
  spdlog::debug("[typecheck] Finished function definition: {}", func_name);
}

void typecheck(Ref<Program> program) {
  spdlog::debug("[typechecker] program at {}", fmt::ptr(program.get()));

  auto ctx = std::make_shared<TypecheckContext>(program);

  // Register built-in functions
  auto print_symbol = std::make_shared<Symbol>();
  print_symbol->name = "print";
  print_symbol->type = std::make_shared<Type>();
  print_symbol->type->base_type = BaseType::Void;
  print_symbol->symbol_type = SymbolType::Function;
  ctx->global_scope->symbols["print"] = print_symbol;

  spdlog::debug("Registered print built-in in global scope");
  log_scope_symbols(ctx->global_scope, "Global scope");

  for (auto &stmt : program->statements) {
      spdlog::debug("[typechecker] Typechecking statement, current_scope = {}",
                fmt::ptr(ctx->current_scope().get()));
    typecheck_statement(ctx, std::static_pointer_cast<Statement>(stmt));
  }
}