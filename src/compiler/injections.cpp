/* Injections into the parse_tree, e.g. enum to string. We're making a function templater here such that we for every singel enum, can inject 
a function that converts the enum to a string into the AST before typechecking, which makes typechecking easier and just ingrains the functioanltiy into the compiler more instead of making it a codegen level features*/

#include "../definitions/ast.hpp"
#include "../definitions/types.hpp"
#include "../definitions/serializations.hpp"
#include "../utils/printer.hpp"
#include "../utils/logging.hpp"


Ref<Call> inject_enum_to_string_call(Ref<Enum> enum_struct, Ref<Expression> enum_value, Span span) {
  std::cout << "Injecting enum to string call" << std::endl;
  auto call = std::make_shared<Call>();
auto callee_ident = std::make_shared<Identifier>();
callee_ident->name = "enki_" + std::string(enum_struct->name) + "_to_string";
callee_ident->span = enum_struct->span;
call->callee = callee_ident;  
  call->arguments.push_back(enum_value);
  call->span = enum_value->span;
  call->span = span;
  return call;
}
 
Ref<FunctionDefinition> inject_enum_to_string(Ref<Enum> enum_struct) {
  auto func_def = std::make_shared<FunctionDefinition>();
  func_def->function = std::make_shared<Function>();
  func_def->function->definition = func_def;
  func_def->function->scope = std::make_shared<Scope>();
  // Note: ctx is not available in this context, this would need to be passed in
  

  func_def->return_type = std::make_shared<Type>(Type{BaseType::String});
  func_def->identifier = std::make_shared<Identifier>();
  func_def->identifier->name = "enki_" + std::string(enum_struct->name) + "_to_string";
  func_def->identifier->span = enum_struct->span;
    
  // It should take in one parameter, which should be the enum value, which would be an identifier, and return a string
  auto param = std::make_shared<Parameter>();
  param->identifier = std::make_shared<Identifier>();
  param->identifier->name = "value";
  param->identifier->span = enum_struct->span;
  param->type = std::make_shared<Type>(Type{BaseType::Identifier});
  func_def->parameters.push_back(param);

  for (auto &member : enum_struct->members) {
    auto member_name = member.first;
    auto member_value = member.second;
    
    auto if_stmt = std::make_shared<If>();
    
    // Create the condition (BinaryOp)
    auto condition = std::make_shared<BinaryOp>();
    
    auto left_ident = std::make_shared<Identifier>();
    left_ident->name = "value";
    left_ident->span = enum_struct->span;
    condition->left = left_ident;
    
    auto right_ident = std::make_shared<Identifier>();
    right_ident->name = member_name;
    right_ident->span = enum_struct->span;
    condition->right = right_ident;
    
    condition->op = BinaryOpType::Equals;
    condition->span = enum_struct->span;
    if_stmt->condition = condition;
    
    // Create the then branch (Block with Return)
    auto then_block = std::make_shared<Block>();
    then_block->scope = std::make_shared<Scope>();
    
    auto return_stmt = std::make_shared<Return>();
    auto literal = std::make_shared<Literal>();
    literal->value = "\"" + std::string(member_name) + "\"";
    literal->span = enum_struct->span;
    literal->type = std::make_shared<Type>(Type{BaseType::String});
    return_stmt->expression = literal;
    return_stmt->span = enum_struct->span;
    
    then_block->statements.push_back(return_stmt);
    if_stmt->then_branch = then_block;
    
    if_stmt->span = enum_struct->span;
    func_def->body->statements.push_back(if_stmt);
  }

  return func_def;
}