#include "eval.hpp"
#include "../compiler/lexer.hpp"
#include "../compiler/parser.hpp"
#include "../definitions/serializations.hpp"
#include "../runtime/builtins.hpp"
#include "../utils/printer.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

std::shared_ptr<ValueBase>
eval_expression(EvalContext &ctx, const std::shared_ptr<Expression> &expr) {
  std::cout << "Evaluating expression" << std::endl;

  if (expr->ast_type == ASTType::FunctionCall) {
    std::cout << "Expression is a function call" << std::endl;
    auto call = std::dynamic_pointer_cast<CallExpression>(expr);
    std::vector<std::shared_ptr<ValueBase>> args;
    for (auto arg : call->arguments) {
      args.push_back(eval_expression(ctx, arg));
    }
    std::cout << "Arguments: " << args.size() << std::endl;

    // To find the name, i guess we hae to evaluate the callee
    std::cout << "Evaluating callee: " << std::endl;
    auto callee = eval_expression(ctx, call->callee);
    std::cout << "Callee type: " << callee->type_name() << std::endl;
    std::cout << "Callee name: " << std::string(callee->type_name())
              << std::endl;

    if (std::string(callee->type_name()) != "string") {
      throw std::runtime_error("Callee is not a string");
    }

    auto str_val = std::dynamic_pointer_cast<StringValue>(callee);
    std::cout << "Callee string value: " << str_val->value << std::endl;

    if (builtin_functions.find(str_val->value) != builtin_functions.end()) {
      std::cout << "Builtin function found" << std::endl;
      auto builtin = builtin_functions[str_val->value];
      if (args.size() < builtin.min_args || args.size() > builtin.max_args)
        throw std::runtime_error(
            "Invalid number of arguments for builtin function: " +
            str_val->value);
      else {
        // Call the builtin function
        auto builtin_func = builtin_functions[str_val->value];
        return builtin_func.impl(args);
      }
    }

    return nullptr;
  }

  if (expr->ast_type == ASTType::Literal) {
    std::cout << "Expression is a literal" << std::endl;
    auto lit = std::dynamic_pointer_cast<Literal>(expr);
    std::cout << "Literal value: " << lit->value << std::endl;

    std::shared_ptr<ValueBase> inter_value;
    switch (lit->type.base_type) {
    case BaseType::Int:
      inter_value = std::make_shared<IntValue>(std::stoi(lit->value));
      break;
    case BaseType::Float:

      inter_value = std::make_shared<DoubleValue>(std::stod(lit->value));
      break;
    case BaseType::String:
      inter_value = std::make_shared<StringValue>(lit->value);
      break;
    default:
      throw std::runtime_error("Unknown literal type");
    }
    inter_value->print(std::cout);
    return inter_value;
  }

  if (expr->ast_type == ASTType::Identifier) {
    std::cout << "Expression is an identifier" << std::endl;
    auto ident = std::dynamic_pointer_cast<Identifier>(expr);
    std::cout << "Identifier: " << ident->name << std::endl;
    if (ctx.values.find(std::string(ident->name)) != ctx.values.end()) {
      return ctx.values[std::string(ident->name)];
    } else {
      return std::make_shared<StringValue>(std::string(ident->name));
    }
  }

  std::cout << "Unknown expression type" << std::endl;

  return nullptr;
}

int interpret(EvalContext &ctx) {

  for (auto stmt : ctx.program.statements) {
    if (auto let = std::dynamic_pointer_cast<LetStatement>(stmt)) {

      std::shared_ptr<ValueBase> value = eval_expression(ctx, let->expression);
      std::cout << "evaluated let expression" << std::endl;

      auto ident = std::dynamic_pointer_cast<Identifier>(let->identifier);
      std::cout << "identifier: " << ident->name << std::endl;
      ctx.values[std::string(ident->name)] = value;
      std::cout << "d" << std::endl;
    }

    if (auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
      std::cout << "evaluating expression statement" << std::endl;
      auto value = eval_expression(ctx, expr_stmt->expression);
      std::cout << "evaluated expression statement" << std::endl;
    }
  }

  return 0;
}
