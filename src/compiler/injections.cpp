 /* Injections into the parse_tree, e.g. enum to string. We're making a function
templater here such that we for every singel enum, can inject a function that
converts the enum to a string into the AST before typechecking, which makes
typechecking easier and just ingrains the functioanltiy into the compiler more
instead of making it a codegen level features*/

#include "../definitions/ast.hpp"
#include "../definitions/serializations.hpp"
#include "../definitions/types.hpp"
#include "../utils/logging.hpp"
#include "../utils/printer.hpp"
#include "typecheck.hpp"
#include <unordered_map>

std::unordered_map<std::string, std::string> _interned_strings_injections;

// Forward declaration
Ref<FunctionDefinition> inject_enum_to_string(Ref<Enum> enum_struct);

// Recursively scan statements and inject necessary functions
void scan_and_inject_statements(std::vector<Ref<Statement>> &statements) {
  // We'll handle enum-to-string function injection during typechecking instead
  // This ensures they're added to the correct scope
}

// Helper to inject the built-in print function
void inject_builtin_print(std::vector<Ref<Statement>> &statements) {
  auto print_func_def = std::make_shared<FunctionDefinition>();
  print_func_def->identifier = std::make_shared<Identifier>();
  print_func_def->identifier->name = "print";
  print_func_def->identifier->span = Span{};
  print_func_def->return_type = std::make_shared<Type>();
  print_func_def->return_type->base_type = BaseType::Void;
  print_func_def->function = std::make_shared<Function>();
  print_func_def->function->name = "print";
  print_func_def->function->return_type = print_func_def->return_type;
  print_func_def->function->definition = print_func_def;
  print_func_def->function->scope = std::make_shared<Scope>();
  
  // Add an Any parameter to print function
  auto param = std::make_shared<Parameter>();
  param->identifier = std::make_shared<Identifier>();
  param->identifier->name = "value";
  param->identifier->span = Span{};
  param->type = std::make_shared<Type>();
  param->type->base_type = BaseType::Any;
  print_func_def->parameters.push_back(param);
  
  print_func_def->body = nullptr;
  statements.insert(statements.begin(), print_func_def);
  spdlog::debug("[injections] Injected built-in print function with Any parameter");
}

void perform_injections(Ref<Program> program) {
  spdlog::debug("[injections] Starting injection pass");
  if (!program || !program->body) {
    spdlog::error("[injections] Program or program body is null");
    return;
  }
  // Inject built-in print function at the start
  inject_builtin_print(program->body->statements);
  // Scan the program body and inject necessary functions
  scan_and_inject_statements(program->body->statements);
  spdlog::debug("[injections] Injection pass complete");
}

Ref<Call> inject_enum_to_string_call(Ref<Enum> enum_struct,
                                     Ref<Expression> enum_value, Span span) {
  spdlog::debug("[injections] Injecting enum to string call for enum: {}",
                enum_struct->name);
  auto call = std::make_shared<Call>();
  auto callee_ident = std::make_shared<Identifier>();

  // Create a proper string that owns its memory and intern it
  std::string enum_name_str(enum_struct->name);
  std::string func_name = enum_name_str + "_to_string";

  // Intern the function name to ensure it persists - use map to avoid
  // reallocation issues
  auto &interned_func_name = _interned_strings_injections[func_name];
  if (interned_func_name.empty()) {
    interned_func_name = func_name;
  }

  callee_ident->name = interned_func_name;
  callee_ident->span = span;
  call->callee = callee_ident;
  call->arguments.push_back(enum_value);
  call->span = span;
  return call;
}

Ref<FunctionDefinition> inject_enum_to_string(Ref<Enum> enum_struct) {
  if (!enum_struct) {
    spdlog::error("[injections] Enum struct is null");
    return nullptr;
  }
  spdlog::debug("[injections] Injecting enum to string function for enum: {} "
                "with {} members",
                enum_struct->name, enum_struct->members.size());

  // Create a proper string that owns its memory and intern it
  std::string enum_name_str(enum_struct->name);
  std::string func_name = enum_name_str + "_to_string";

  // Intern the function name to ensure it persists - use map to avoid
  // reallocation issues
  auto &interned_func_name = _interned_strings_injections[func_name];
  if (interned_func_name.empty()) {
    interned_func_name = func_name;
  }

  auto func_def = std::make_shared<FunctionDefinition>();
  func_def->function = std::make_shared<Function>();
  func_def->function->definition = func_def;
  func_def->function->scope = std::make_shared<Scope>();

  // Set return type to String
  func_def->return_type = std::make_shared<Type>(Type{BaseType::String});

  // Create function identifier
  func_def->identifier = std::make_shared<Identifier>();
  func_def->identifier->name = interned_func_name;
  func_def->identifier->span = enum_struct->span;

  // Create parameter for the enum value
  auto param = std::make_shared<Parameter>();
  param->identifier = std::make_shared<Identifier>();
  param->identifier->name = "value";
  param->identifier->span = enum_struct->span;

  // Use the specific enum type
  param->type = std::make_shared<Type>(Type{BaseType::Enum});
  param->type->structure = enum_struct;

  func_def->parameters.push_back(param);

  // Create function body with if-else chain
  func_def->body = std::make_shared<Block>();
  func_def->body->scope = std::make_shared<Scope>();
  func_def->body->span = enum_struct->span;

  // Create if-else statements for each enum member
  for (auto &member : enum_struct->members) {
    auto member_name = member.first;
    auto member_value = member.second;

    auto if_stmt = std::make_shared<If>();

    // Create the condition (BinaryOp: value == member_name)
    auto condition = std::make_shared<BinaryOp>();

    auto left_ident = std::make_shared<Identifier>();
    left_ident->name = "value";
    left_ident->span = enum_struct->span;
    condition->left = left_ident;

    auto right_ident = std::make_shared<Identifier>();
    right_ident->name = member_name;
    right_ident->span = enum_struct->span;

    auto dot = std::make_shared<Dot>();
    auto lhs = std::make_shared<Identifier>();
    lhs->name = enum_struct->name;
    lhs->span = enum_struct->span;

    dot->left = lhs;
    dot->right = right_ident;
    dot->span = enum_struct->span;

    condition->right = dot;

    condition->op = BinaryOpType::Equals;
    condition->span = enum_struct->span;
    if_stmt->condition = condition;

    // Create the then branch (Block with Return)
    auto then_block = std::make_shared<Block>();
    then_block->scope = std::make_shared<Scope>();
    then_block->span = enum_struct->span;

    auto return_stmt = std::make_shared<Return>();
    auto literal = std::make_shared<Literal>();

    // Create string literal for the member name - use map for stable storage
    std::string member_str = std::string(member_name);
    auto &interned_member_str = _interned_strings_injections[member_str];
    if (interned_member_str.empty()) {
      interned_member_str = member_str;
    }
    literal->value = interned_member_str;

    literal->span = enum_struct->span;
    literal->type = std::make_shared<Type>(Type{BaseType::String});
    return_stmt->expression = literal;
    return_stmt->span = enum_struct->span;

    then_block->statements.push_back(return_stmt);
    if_stmt->then_branch = then_block;
    if_stmt->span = enum_struct->span;

    func_def->body->statements.push_back(if_stmt);
  }

  // Set the span for the function definition
  func_def->span = enum_struct->span;

  spdlog::debug("[injections] Created enum-to-string function: {}",
                interned_func_name);
  utils::ast::print_ast(func_def);

  return func_def;
}

Ref<FunctionDefinition> inject_enum_to_string_in_scope(Ref<TypecheckContext> ctx, Ref<Enum> enum_struct) {
  auto injected_func = inject_enum_to_string(enum_struct);
  if (injected_func) {
    // Register the injected function in the current scope
    auto func_name = injected_func->identifier->name;
    spdlog::debug("[injections] Injecting enum-to-string function: {} in scope", func_name);
    
    // Create function type
    auto func_type = std::make_shared<Function>();
    func_type->name = func_name;
    func_type->return_type = injected_func->return_type;
    func_type->span = injected_func->span;

    // Add parameters to function type
    for (auto &param : injected_func->parameters) {
      auto param_var = std::make_shared<Variable>();
      param_var->name = param->identifier->name;
      param_var->type = param->type;
      func_type->parameters.push_back(param_var);
    }

    // Create type wrapper
    auto type = std::make_shared<Type>();
    type->base_type = BaseType::Function;
    type->structure = func_type;
    type->span = injected_func->span;

    // Register symbol in current scope
    auto func_symbol = std::make_shared<Symbol>();
    func_symbol->name = func_name;
    func_symbol->type = type;
    func_symbol->symbol_type = SymbolType::Function;
    func_symbol->span = injected_func->span;
    ctx->current_scope()->symbols[func_name] = func_symbol;
    
    spdlog::debug("[injections] Registered injected function: {} in current scope", func_name);
  }
  return injected_func;
}