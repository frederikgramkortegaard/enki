#include "typecheck.hpp"
#include "../definitions/ast.hpp"
#include "../definitions/types.hpp"
#include "../utils/logging.hpp"
#include "injections.hpp"
#include <spdlog/spdlog.h>
#include <variant>

void typecheck_statement(Ref<TypecheckContext> ctx, Ref<Statement> stmt);
Ref<Type> typecheck_expression(Ref<TypecheckContext> ctx, Ref<Expression> expr);
void typecheck_function_definition(Ref<TypecheckContext> ctx,
                                   Ref<FunctionDefinition> func_def);
void register_function_signature(Ref<TypecheckContext> ctx,
                                 Ref<FunctionDefinition> func_def);
void register_enum_signature(Ref<TypecheckContext> ctx,
                             Ref<EnumDefinition> enum_def);

// Helper function to determine the result type of a binary operation
Ref<Type> get_binary_op_result_type(BinaryOpType op, Ref<Type> left_type,
                                    Ref<Type> right_type) {
  auto result_type = std::make_shared<Type>();

  switch (op) {
  // Arithmetic operations: return the type of the operands (must be numeric)
  case BinaryOpType::Add:
  case BinaryOpType::Subtract:
  case BinaryOpType::Multiply:
  case BinaryOpType::Divide:
  case BinaryOpType::Modulo:
    if (left_type->base_type == BaseType::Float ||
        right_type->base_type == BaseType::Float) {
      result_type->base_type = BaseType::Float;
    } else if (left_type->base_type == BaseType::Int &&
               right_type->base_type == BaseType::Int) {
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
bool is_valid_binary_op(BinaryOpType op, Ref<Type> left_type,
                        Ref<Type> right_type) {
  switch (op) {
  // Arithmetic operations: require numeric types (int or float, NOT char)
  case BinaryOpType::Add:
  case BinaryOpType::Subtract:
  case BinaryOpType::Multiply:
  case BinaryOpType::Divide:
  case BinaryOpType::Modulo:
    // TODO: Allow pointer arithmetic
    return (left_type->base_type == BaseType::Int ||
            left_type->base_type == BaseType::Float) &&
           (right_type->base_type == BaseType::Int ||
            right_type->base_type == BaseType::Float);

  // Comparison operations: require compatible types
  case BinaryOpType::Equals:
  case BinaryOpType::NotEquals:
    // TODO: Allow pointer equality checks
    return types_are_equal(left_type, right_type);

  case BinaryOpType::LessThan:
  case BinaryOpType::GreaterThan:
  case BinaryOpType::LessThanOrEqual:
  case BinaryOpType::GreaterThanOrEqual:
    // TODO: Allow pointer comparison
    // Can only compare numeric types (int or float, NOT char)
    return (left_type->base_type == BaseType::Int ||
            left_type->base_type == BaseType::Float) &&
           (right_type->base_type == BaseType::Int ||
            right_type->base_type == BaseType::Float);

  default:
    return false;
  }
}

// Helper function to get the depth of a scope in the scope chain
int get_scope_depth(const Ref<Scope> &scope) {
  int depth = 0;
  auto current_scope = scope;
  while (current_scope && current_scope->parent) {
    ++depth;
    current_scope = current_scope->parent;
  }
  return depth;
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
    LOG_ERROR_EXIT("[typechecker] Symbol not found: " + std::string(id->name),
                   id->span, *ctx->program->source_buffer);
  }
  return symbol->type;
}

Ref<Type> typecheck_dereference(Ref<TypecheckContext> ctx,
                                Ref<Dereference> deref) {
  spdlog::debug("[typechecker] typecheck_dereference: deref expression");

  auto expr_type = typecheck_expression(ctx, deref->expression);
  if (expr_type->base_type != BaseType::Pointer) {
    LOG_ERROR_EXIT("[typechecker] Dereference operator '*' can only be applied "
                   "to pointer types, got: " +
                       expr_type->to_string(),
                   deref->span, *ctx->program->source_buffer);
  }

  // Dereferencing a pointer returns the type it points to
  return std::get<Ref<Type>>(expr_type->structure);
}

Ref<Type> typecheck_address_of(Ref<TypecheckContext> ctx,
                               Ref<AddressOf> addr_of) {
  spdlog::debug("[typechecker] typecheck_address_of: addr_of expression");

  auto expr_type = typecheck_expression(ctx, addr_of->expression);
  // TODO: Assert that we are only taking pointers of valid lvalues

  // Addressing a variable returns a pointer to its type
  // TODO: We might have multiple BaseType::Pointers which point to the same
  // underlying
  //       type, do we want to cache these so there is always one ground truth
  //       for _any_ type?
  return std::make_shared<Type>(Type{BaseType::Pointer, expr_type});
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
            std::string(
                std::static_pointer_cast<Identifier>(call->callee)->name),
        call->span, *ctx->program->source_buffer);
  }
  if (function_symbol->symbol_type != SymbolType::Function) {
    LOG_ERROR_EXIT(
        "[typechecker] Symbol is not a function: " +
            std::string(
                std::static_pointer_cast<Identifier>(call->callee)->name),
        call->span, *ctx->program->source_buffer);
  }

  // Typecheck function arguments
  if (function_symbol->type->base_type == BaseType::Function) {
    auto func_type = std::get<Ref<Function>>(function_symbol->type->structure);

    // Check argument count
    if (call->arguments.size() != func_type->parameters.size()) {
      LOG_ERROR_EXIT(
          "[typechecker] Function argument count mismatch: expected " +
              std::to_string(func_type->parameters.size()) + ", got " +
              std::to_string(call->arguments.size()),
          call->span, *ctx->program->source_buffer);
    }

    // Typecheck each argument against its corresponding parameter
    for (size_t i = 0; i < call->arguments.size(); ++i) {
      auto arg_type = typecheck_expression(ctx, call->arguments[i]);
      auto param_type = func_type->parameters[i]->type;
      
      if (!can_assign_type(param_type, arg_type)) {
        LOG_ERROR_EXIT("[typechecker] Type mismatch in argument " +
                           std::to_string(i + 1) + ": expected " +
                           param_type->to_string() + ", got " +
                           arg_type->to_string(),
                       call->arguments[i]->span, *ctx->program->source_buffer);
      }
    }

    spdlog::debug("[typecheck] Function return type: {}",
                  func_type->return_type->to_string());
    return func_type->return_type;
  }

  return function_symbol->type;
}

Ref<Type> typecheck_binary_op(Ref<TypecheckContext> ctx, Ref<BinaryOp> bin_op) {
  spdlog::debug("[typechecker] typecheck_binary_op: op = {}",
                magic_enum::enum_name(bin_op->op));
  auto left_type = typecheck_expression(ctx, bin_op->left);
  auto right_type = typecheck_expression(ctx, bin_op->right);

  // Check if the binary operation is valid for the given types
  if (!is_valid_binary_op(bin_op->op, left_type, right_type)) {
    LOG_ERROR_EXIT("[typechecker] Invalid binary operation: " +
                       std::string(magic_enum::enum_name(bin_op->op)) +
                       " between " + left_type->to_string() + " and " +
                       right_type->to_string(),
                   bin_op->span, *ctx->program->source_buffer);
  }

  // Get the correct result type for this operation
  auto result_type =
      get_binary_op_result_type(bin_op->op, left_type, right_type);

  spdlog::debug("[typechecker] binary_op result type: {}",
                result_type->to_string());

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
      "[typechecker] typecheck_parameter: param type = {}",
      magic_enum::enum_name(param ? param->get_type() : ASTType::Unknown));

  // If the parameter type is Unknown, try to resolve it as an enum type
  if (param->type->base_type == BaseType::Unknown) {
    // Look up the type name in the scope chain
    auto type_symbol =
        find_symbol_in_scope_chain(ctx->current_scope(), param->type->name);
    if (type_symbol && type_symbol->symbol_type == SymbolType::Enum) {
      spdlog::debug(
          "[typechecker] Resolved unknown parameter type '{}' to enum",
          param->type->to_string());
      return type_symbol->type;
    } else {
      spdlog::error(
          "[typechecker] Could not resolve unknown parameter type '{}'",
          param->type->to_string());
    }
  }

  return param->type;
}

Ref<Type> typecheck_dot_expression(Ref<TypecheckContext> ctx,
                                   Ref<Dot> dot_expr) {
  spdlog::debug("[typechecker] typecheck_dot_expression: dot_expr type = {}",
                magic_enum::enum_name(dot_expr ? dot_expr->get_type()
                                               : ASTType::Unknown));

  auto left_type = typecheck_expression(ctx, dot_expr->left);

  // Enum Member Access
  if (left_type->base_type == BaseType::Enum &&
      dot_expr->right->get_type() == ASTType::Identifier) {

    auto right = std::static_pointer_cast<Identifier>(dot_expr->right);
    auto right_name = right->name;
    auto enum_type = std::get<Ref<Enum>>(left_type->structure);
    auto member = enum_type->members.find(right_name);
    if (member == enum_type->members.end()) {
      LOG_ERROR_EXIT("[typechecker] Enum member not found: " +
                         std::string(right_name),
                     right->span, *ctx->program->source_buffer);
    }
    return member->second->type;
  }

  LOG_ERROR_EXIT(
      "[typechecker] Invalid dot expression: " + left_type->to_string() + " " +
          std::string(magic_enum::enum_name(dot_expr->right->get_type())),
      dot_expr->span, *ctx->program->source_buffer);
  return nullptr;
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
  case ASTType::Call:
    return typecheck_function_call(ctx, std::static_pointer_cast<Call>(expr));
  case ASTType::Dereference:
    return typecheck_dereference(ctx,
                                 std::static_pointer_cast<Dereference>(expr));
  case ASTType::AddressOf:
    return typecheck_address_of(ctx, std::static_pointer_cast<AddressOf>(expr));
  case ASTType::Dot:
    return typecheck_dot_expression(ctx, std::static_pointer_cast<Dot>(expr));
  default:
    LOG_ERROR_EXIT("[typechecker] Unknown expression type: " +
                       std::string(magic_enum::enum_name(expr->get_type())),
                   expr->span, *ctx->program->source_buffer);
  }

  return nullptr;
}

void typecheck_block(Ref<TypecheckContext> ctx, Ref<Block> block) {
  spdlog::debug(
      "[typechecker] typecheck_block: block type = {}",
      magic_enum::enum_name(block ? block->get_type() : ASTType::Unknown));

  // Set the current block for injection purposes
  ctx->current_block = block;

  ctx->push_scope(block->scope);

  // First pass: Register all enum definitions first, then function definitions
  spdlog::debug("[typechecker] Block first pass: Registering enums first");
  for (auto &stmt : block->statements) {
    auto stmt_type = stmt->get_type();
    if (stmt_type == ASTType::EnumDefinition) {
      spdlog::debug("[typechecker] Block first pass: Registering enum");
      register_enum_signature(ctx,
                              std::static_pointer_cast<EnumDefinition>(stmt));
    }
  }

  spdlog::debug("[typechecker] Block first pass: Registering functions");
  for (auto &stmt : block->statements) {
    auto stmt_type = stmt->get_type();
    if (stmt_type == ASTType::FunctionDefinition) {
      spdlog::debug("[typechecker] Block first pass: Registering function");
      register_function_signature(
          ctx, std::static_pointer_cast<FunctionDefinition>(stmt));
    }
  }

  // Second pass: Typecheck all statements
  spdlog::debug("[typechecker] Block second pass: Typechecking all statements");
  for (auto &stmt : block->statements) {
    spdlog::debug(
        "[typechecker] Second pass: Processing statement at address: {}",
        (void *)stmt.get());
    spdlog::debug("[typechecker] Second pass: Statement type: {}",
                  magic_enum::enum_name(stmt->get_type()));
    typecheck_statement(ctx, std::static_pointer_cast<Statement>(stmt));
  }

  ctx->pop_scope();
  ctx->current_block = nullptr; // Clear current block
}

void typecheck_return(Ref<TypecheckContext> ctx, Ref<Return> ret) {
  spdlog::debug("[typechecker] typecheck_return: processing return statement");

  auto current_func = ctx->current_function();
  if (!current_func) {
    LOG_ERROR_EXIT("[typechecker] Return statement outside of function",
                   ret->span, *ctx->program->source_buffer);
  }
  // If function returns void, return must not have an expression
  if (current_func->return_type->base_type == BaseType::Void) {
    if (ret->expression != nullptr) {
      LOG_ERROR_EXIT("[typechecker] Cannot return a value from a void function",
                     ret->span, *ctx->program->source_buffer);
    }
    ret->type = current_func->return_type;

    ret->function = current_func;
    spdlog::debug(
        "[typechecker] typecheck_return: void function, no return value");
    return;
  }
  // Otherwise, must typecheck the expression and match types
  if (ret->expression == nullptr) {
    LOG_ERROR_EXIT(
        "[typechecker] Missing return expression in non-void function",
        ret->span, *ctx->program->source_buffer);
  }
  auto return_type = typecheck_expression(ctx, ret->expression);

  if (!can_assign_type(current_func->return_type, return_type)) {
    LOG_ERROR_EXIT(
        "[typechecker] Return type mismatch: " + return_type->to_string() +
            " != " + current_func->return_type->to_string(),
        ret->span, *ctx->program->source_buffer);
  }
  ret->type = return_type;
  ret->function = current_func;
  spdlog::debug("[typechecker] typecheck_return: returning value of type {}",
                return_type->to_string());
}

void typecheck_assignment(Ref<TypecheckContext> ctx,
                          Ref<Assignment> assignment) {
  spdlog::debug("[typechecker] typecheck_assignment: assignment type = {}",
                magic_enum::enum_name(assignment ? assignment->get_type()
                                                 : ASTType::Unknown));
  auto assignee_type = typecheck_expression(ctx, assignment->assignee);
  auto expression_type = typecheck_expression(ctx, assignment->expression);

  if (!can_assign_type(assignee_type, expression_type)) {
    LOG_ERROR_EXIT("[typechecker] Assignment type mismatch: " +
                       assignee_type->to_string() +
                       " != " + expression_type->to_string(),
                   assignment->span, *ctx->program->source_buffer);
  }

  //@TODO : This wont work the second we have arr indexes etc
  auto assignee_symbol = find_symbol_in_scope_chain(
      ctx->current_scope(),
      std::static_pointer_cast<Identifier>(assignment->assignee)->name);
  if (!assignee_symbol) {
    LOG_ERROR_EXIT("[typechecker] Assignee symbol not found: " +
                       std::string(std::static_pointer_cast<Identifier>(
                                       assignment->assignee)
                                       ->name),
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
    LOG_ERROR_EXIT("[typechecker] If condition must be boolean, got: " +
                       condition_type->to_string(),
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
    LOG_ERROR_EXIT("[typechecker] While condition must be bool, got: " +
                       condition_type->to_string(),
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
    LOG_ERROR_EXIT("[typechecker] Import module path must be a string literal",
                   import_stmt->span, *ctx->program->source_buffer);
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

void typecheck_enum_definition(Ref<TypecheckContext> ctx,
                               Ref<EnumDefinition> enum_def) {
  spdlog::debug("[typechecker] typecheck_enum_definition: enum_def type = {}",
                magic_enum::enum_name(enum_def ? enum_def->get_type()
                                               : ASTType::Unknown));

  // The enum was already registered in the first pass, just verify it exists
  auto enum_name = enum_def->identifier->name;
  auto enum_symbol =
      find_symbol_in_scope_chain(ctx->current_scope(), enum_name);
  if (!enum_symbol || enum_symbol->symbol_type != SymbolType::Enum) {
    LOG_ERROR_EXIT("[typechecker] Enum not found in symbol table: " +
                       std::string(enum_name),
                   enum_def->span, *ctx->program->source_buffer);
  }

  // Register enum members in the current scope
  auto enum_type = std::get<Ref<Enum>>(enum_symbol->type->structure);
  for (const auto &[member_name, member_var] : enum_type->members) {
    auto member_symbol = std::make_shared<Symbol>();
    member_symbol->name = member_name;
    member_symbol->type = member_var->type;
    member_symbol->symbol_type = SymbolType::Variable;
    member_symbol->span = member_var->span;
    ctx->current_scope()->symbols[member_name] = member_symbol;
  }

  // Inject the enum-to-string function into the current scope
  inject_enum_to_string_in_scope(ctx, enum_type);
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
  case ASTType::EnumDefinition:
    typecheck_enum_definition(ctx,
                              std::static_pointer_cast<EnumDefinition>(stmt));
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
    LOG_ERROR_EXIT("[typechecker] Unknown statement type: " +
                       std::string(magic_enum::enum_name(stmt->get_type())),
                   stmt->span, *ctx->program->source_buffer);
  }
  return;
}

void typecheck_function_definition(Ref<TypecheckContext> ctx,
                                   Ref<FunctionDefinition> func_def) {
  auto func_name =
      func_def && func_def->identifier ? func_def->identifier->name : "<null>";
  spdlog::debug("[typecheck] Typechecking function definition: {}", func_name);
  if (func_def && func_def->parameters.size() > 0) {
    for (const auto &param : func_def->parameters) {
      auto pname = param->identifier ? param->identifier->name : "<null>";
      spdlog::debug("[typecheck]   Parameter: {}", pname);
    }
  } else {
    spdlog::debug("[typecheck]   No parameters");
  }

  // Get the function from the symbol table (registered in first pass)
  auto func_symbol =
      find_symbol_in_scope_chain(ctx->current_scope(), func_name);
  if (!func_symbol || func_symbol->symbol_type != SymbolType::Function) {
    LOG_ERROR_EXIT("[typechecker] Function not found in symbol table: " +
                       std::string(func_name),
                   func_def->span, *ctx->program->source_buffer);
  }

  // Get the function type from the symbol
  auto func_type = std::get<Ref<Function>>(func_symbol->type->structure);
  func_def->function = func_type;

  spdlog::debug("[typecheck] Function type return type: {}",
                func_type->return_type->to_string());

  // Set up function scope and metadata
  func_def->function->scope = func_def->body->scope;
  func_def->function->definition = func_def;

  // Ensure the function scope has the current scope as its parent
  func_def->function->scope->parent = ctx->current_scope();

  ctx->push_function(func_def->function);
  ctx->push_scope(func_def->function->scope);
  spdlog::debug("[typecheck]   Entered function scope (depth = {})",
                get_scope_depth(func_def->function->scope));

  // Add parameters to function scope
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
    spdlog::debug("[typecheck]     Added parameter symbol: {} (type: {})",
                  param_symbol->name,
                  param_symbol->type ? param_symbol->type->to_string()
                                     : "<null type>");

    // Also update the function type parameter to have the resolved type
    for (auto &func_param : func_def->function->parameters) {
      if (func_param->name == param->identifier->name) {
        func_param->type = param_type;
        break;
      }
    }
  }

  // Use typecheck_block to process the function body (handles nested functions
  // automatically)
  typecheck_block(ctx, func_def->body);

  ctx->pop_function();
  ctx->pop_scope();
  spdlog::debug(
      "[typecheck]   Exited function scope for: {} (depth = {})", func_name,
      ctx->current_scope() ? get_scope_depth(ctx->current_scope()) : 0);
  spdlog::debug("[typecheck] Function type return type after exit: {}",
                func_def->function->return_type->to_string());

  spdlog::debug("[typecheck] Finished function definition: {}", func_name);
}

// Registration functions for forward references
void register_function_signature(Ref<TypecheckContext> ctx,
                                 Ref<FunctionDefinition> func_def) {
  auto func_name = func_def->identifier->name;
  spdlog::debug("[typechecker] Registering function signature: {}", func_name);

  // Create function type
  auto func_type = std::make_shared<Function>();
  func_type->name = func_name;

  // Resolve return type if it's an identifier (e.g., "Color" -> Color enum
  // type)
  if (func_def->return_type->base_type == BaseType::Unknown) {
    auto return_type_symbol = find_symbol_in_scope_chain(
        ctx->current_scope(), func_def->return_type->name);
    if (return_type_symbol &&
        return_type_symbol->symbol_type == SymbolType::Enum) {
      func_type->return_type = return_type_symbol->type;
      spdlog::debug("[typechecker] Resolved unknown return type '{}' to enum",
                    func_def->return_type->name);
    } else {
      func_type->return_type = func_def->return_type;
      spdlog::error("[typechecker] Could not resolve unknown return type '{}'",
                    func_def->return_type->name);
    }
  } else {
    func_type->return_type = func_def->return_type;
  }

  func_type->span = func_def->span;

  // Add parameters to function type
  for (auto &param : func_def->parameters) {
    auto param_var = std::make_shared<Variable>();
    param_var->name = param->identifier->name;
    param_var->type = param->type;
    func_type->parameters.push_back(param_var);
  }

  // Create type wrapper
  auto type = std::make_shared<Type>();
  type->base_type = BaseType::Function; // Ensure correct base_type
  type->structure = func_type;
  type->span = func_def->span;

  // Register symbol
  auto func_symbol = std::make_shared<Symbol>();
  func_symbol->name = func_name;
  func_symbol->type = type;
  func_symbol->symbol_type = SymbolType::Function;
  func_symbol->span = func_def->span;
  ctx->current_scope()->symbols[func_name] = func_symbol;

  spdlog::debug(
      "[typechecker] Registered function signature: {} (return type: {})",
      func_name, func_type->return_type->to_string());
  spdlog::debug(
      "[typechecker] Function symbol '{}' type ptr: {} return type: {}",
      func_name, fmt::ptr(type.get()), func_type->return_type->to_string());
}

void register_enum_signature(Ref<TypecheckContext> ctx,
                             Ref<EnumDefinition> enum_def) {
  auto enum_name = enum_def->identifier->name;
  spdlog::debug("[typechecker] Registering enum signature: {}", enum_name);

  // Register the enum type
  auto enum_symbol = std::make_shared<Symbol>();
  enum_symbol->name = enum_name;
  enum_symbol->type = enum_def->enum_type;
  enum_symbol->symbol_type = SymbolType::Enum;
  enum_symbol->span = enum_def->span;
  ctx->current_scope()->symbols[enum_name] = enum_symbol;

  spdlog::debug("[typechecker] Registered enum signature: {}", enum_name);
}

void typecheck(Ref<Program> program) {
  spdlog::debug("[typechecker] program at {}", fmt::ptr(program.get()));

  auto ctx = std::make_shared<TypecheckContext>(program);

  spdlog::debug("[typechecker] Typechecking program body with {} statements",
                program->body->statements.size());
  typecheck_block(ctx, program->body);
}