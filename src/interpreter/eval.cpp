#include "eval.hpp"
#include "../compiler/lexer.hpp"
#include "../compiler/parser.hpp"
#include "../definitions/serializations.hpp"
#include "../runtime/builtins.hpp"
#include "../utils/printer.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>

std::shared_ptr<ValueBase>
eval_expression(EvalContext &ctx, const std::shared_ptr<Expression> &expr) {
  spdlog::debug("Evaluating expression");

  if (expr->ast_type == ASTType::FunctionCall) {
    spdlog::debug("Expression is a function call");
    auto call = std::dynamic_pointer_cast<CallExpression>(expr);
    std::vector<std::shared_ptr<ValueBase>> args;
    for (auto arg : call->arguments) {
      args.push_back(eval_expression(ctx, arg));
    }
    spdlog::debug("Arguments: {}", args.size());

    // To find the name, i guess we hae to evaluate the callee
    spdlog::debug("Evaluating callee: ");
    auto callee = eval_expression(ctx, call->callee);
    spdlog::debug("Callee type: {}", callee->type_name());
    spdlog::debug("Callee name: {}", std::string(callee->type_name()));

    if (std::string(callee->type_name()) != "string") {
      throw std::runtime_error("Callee is not a string");
    }

    auto str_val = std::dynamic_pointer_cast<StringValue>(callee);
    spdlog::debug("Callee string value: {}", str_val->value);

    if (builtin_functions.find(str_val->value) != builtin_functions.end()) {
      spdlog::debug("Builtin function found");
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
    spdlog::debug("Expression is a literal");
    auto lit = std::dynamic_pointer_cast<Literal>(expr);
    spdlog::debug("Literal value: {}", lit->value);

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
    case BaseType::Bool:
      inter_value = std::make_shared<BoolValue>(lit->value == "true");
      break;
    default:
      throw std::runtime_error("Unknown literal type");
    }
    // inter_value->print(std::cout);
    return inter_value;
  }

  if (expr->ast_type == ASTType::Identifier) {
    spdlog::debug("Expression is an identifier");
    auto ident = std::dynamic_pointer_cast<Identifier>(expr);
    spdlog::debug("Identifier: {}", ident->name);
    if (ctx.values.find(std::string(ident->name)) != ctx.values.end()) {
      return ctx.values[std::string(ident->name)];
    } else {
      return std::make_shared<StringValue>(std::string(ident->name));
    }
  }

  spdlog::warn("Unknown expression type");

  return nullptr;
}

void eval_statement(EvalContext &ctx, const std::shared_ptr<Statement> &stmt) {
  if (auto let = std::dynamic_pointer_cast<LetStatement>(stmt)) {

      std::shared_ptr<ValueBase> value = eval_expression(ctx, let->expression);
      spdlog::debug("evaluated let expression");

      auto ident = std::dynamic_pointer_cast<Identifier>(let->identifier);
      spdlog::debug("identifier: {}", ident->name);
      ctx.values[std::string(ident->name)] = value;
      spdlog::debug("d" );
      return;
    }

    if (auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
      spdlog::debug("evaluating expression statement");
      auto value = eval_expression(ctx, expr_stmt->expression);
      spdlog::debug("evaluated expression statement");
      return;
    }

    if (auto if_stmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
      spdlog::debug("evaluating if statement");
      auto condition = eval_expression(ctx, if_stmt->condition);
      spdlog::debug("evaluated if statement");
      if (condition->type_name() != "bool") {
        throw std::runtime_error("Condition is not a boolean");
      }
      auto bool_value = std::dynamic_pointer_cast<BoolValue>(condition);
      if (bool_value->value) {
        spdlog::debug("Condition is true, evaluating then branch");
        eval_statement(ctx, if_stmt->then_branch);
      } else if (if_stmt->else_branch) {
        spdlog::debug("Condition is false, evaluating else branch");
        eval_statement(ctx, if_stmt->else_branch);
      }
      return;
    }

    if (auto block_stmt = std::dynamic_pointer_cast<Block>(stmt)) {
      spdlog::debug("evaluating block statement");
      for (const auto &inner_stmt : block_stmt->statements) {
        eval_statement(ctx, inner_stmt);
      }
      spdlog::debug("evaluated block statement");
      return;
    }

    spdlog::warn("Unknown statement type at {}", stmt->span().start.to_string());
    std::exit(1);
}

int interpret(EvalContext &ctx) {
  for (auto stmt : ctx.program.statements) {
    eval_statement(ctx, stmt);
  }

  return 0;
}
