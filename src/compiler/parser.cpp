#include "parser.hpp"
#include <format>
#include <iostream>
#include <spdlog/spdlog.h>

std::shared_ptr<Expression> parse_atom(ParserContext &ctx) {
  const Token &tok = ctx.tokens[ctx.current];

  switch (tok.type) {
  case TokenType::Int:
  case TokenType::Float:
  case TokenType::String: {
    auto lit = std::make_shared<Literal>();
    lit->type = Type{token_to_literal_type(tok.type)};
    lit->value = tok.value;
    lit->span() = tok.span;
    ctx.consume();
    return lit;
  }
  case TokenType::Identifier: {
    // Function Call
    if (ctx.peek(1).type == TokenType::LParens) {
      auto call_expr = std::make_shared<CallExpression>();
      auto ident = std::make_shared<Identifier>();
      ident->name = tok.value;
      ident->span() = tok.span;
      call_expr->callee = ident;

      ctx.consume();
      ctx.consume();

      while (ctx.current < ctx.tokens.size() &&
             ctx.current_token().type != TokenType::RParens) {
        auto arg = parse_expression(ctx);
        if (!arg) {
          spdlog::error(
              "Expected Expression at {} but couldn't find one",
              std::string(ctx.current_token().span.start.to_string()));
          std::exit(1);
        }
        call_expr->arguments.push_back(arg);
        ctx.consume_if(TokenType::Comma);
      }

      ctx.consume_assert(TokenType::RParens, "Missing ')' in Function Call");
      call_expr->span() = tok.span;
      return call_expr;
    }

    auto ident = std::make_shared<Identifier>();
    ident->name = tok.value;
    ident->span() = tok.span;
    ctx.consume();
    return ident;
  }
  case TokenType::True:
  case TokenType::False: {
    auto bool_lit = std::make_shared<Literal>();
    bool_lit->type = Type{BaseType::Bool};
    bool_lit->value = tok.type == TokenType::True ? "true" : "false";
    bool_lit->span() = tok.span;
    ctx.consume();
    return bool_lit;
  }
  default:
    break;
  }

  return nullptr;
}

std::shared_ptr<Type> parse_type(ParserContext &ctx) {
  return nullptr;
}

std::shared_ptr<Identifier> parse_identifier(ParserContext &ctx) {
  const Token &tok = ctx.tokens[ctx.current];
  if (tok.type != TokenType::Identifier) {
    std::cerr << std::format("Expected Identifier at {} but got {}\n",
                             tok.span.start.to_string(),
                             magic_enum::enum_name(tok.type));
    std::exit(1);
  }
  auto ident = std::make_shared<Identifier>();
  ident->name = tok.value;
  ident->span() = tok.span;
  ctx.consume();
  return ident;
}

std::shared_ptr<Expression> parse_expression(ParserContext &ctx) {
  auto left = parse_atom(ctx);
  if (!left)
    return nullptr;

  // Shunting Yard Algorithm for Binary Operators
  std::vector<std::shared_ptr<Expression>> output;
  std::vector<BinaryOpType> ops;
  output.push_back(left);

  while (ctx.current < ctx.tokens.size()) {
    const Token &tok = ctx.current_token();
    auto binop = token_to_binop(tok.type);
    if (!binop.has_value()) {
      break; // No more binary operators
    }
    ctx.consume();
    while (!ops.empty() && binary_op_precedence(ops.back()) >=
                               binary_op_precedence(binop.value())) {
      auto right = output.back();
      output.pop_back();
      auto left_expr = output.back();
      output.pop_back();

      auto bin_expr = std::make_shared<BinaryOp>();
      bin_expr->left = left_expr;
      bin_expr->right = right;
      bin_expr->op = ops.back();
      bin_expr->span() = Span(left_expr->span().start, right->span().end);
      output.push_back(bin_expr);
      ops.pop_back();
    }

    auto right = parse_atom(ctx);
    if (!right) {
      spdlog::error("Expected right operand for binary operator at {}",
                    tok.span.start.to_string());
      std::exit(1);
    }

    output.push_back(right);
    ops.push_back(binop.value());
  }

  // Process remaining operators
  while (!ops.empty()) {
    auto right = output.back();
    output.pop_back();
    auto left_expr = output.back();
    output.pop_back();
    auto bin_expr = std::make_shared<BinaryOp>();
    bin_expr->left = left_expr;
    bin_expr->right = right;
    bin_expr->op = ops.back();
    bin_expr->span() = Span(left_expr->span().start, right->span().end);
    output.push_back(bin_expr);
    ops.pop_back();
  }

  return output.size() == 1 ? output[0] : nullptr;
}

std::shared_ptr<Statement> parse_statement(ParserContext &ctx) {
  const Token &tok = ctx.tokens[ctx.current];
  Span statement_start = tok.span;

  if (tok.type == TokenType::Import) {
    spdlog::debug("Parsing Import statement");
    auto import_stmt = std::make_shared<ImportStatement>();
    ctx.consume();
    ctx.consume_assert(TokenType::LessThan, "Missing '<' in Import statement");
    auto module_path = parse_atom(ctx);
    if (!module_path) {
      spdlog::error("Expected module path in Import statement at {}",
                    tok.span.start.to_string());
      std::exit(1);
    }
    import_stmt->module_path = std::dynamic_pointer_cast<Literal>(module_path);
    spdlog::debug("Module path: {}", import_stmt->module_path->value);
    std::cout << ctx.current_token().span.start.to_string() << std::endl;
    import_stmt->span() = Span(tok.span.start, module_path->span().end);

    // Use CTX Module Context to add the module to the program
    ctx.module_context->add_module(import_stmt->module_path->value, ctx.current_file_path);

    return import_stmt;
  }

  if (tok.type == TokenType::Let) {
    auto let_stmt = std::make_shared<LetStatement>();
    ctx.consume();

    const Token &ident_tok = ctx.current_token();
    if (ident_tok.type != TokenType::Identifier) {
      spdlog::error("Expected Identifier at {} but got {}",
                    ident_tok.span.start.to_string(),
                    magic_enum::enum_name(ident_tok.type));
      std::exit(1);
    }

    auto ident_expr = std::make_shared<Identifier>();
    ident_expr->name = ident_tok.value;
    ident_expr->span() = ident_tok.span;
    let_stmt->identifier = ident_expr;
    ctx.consume();

    ctx.consume_assert(TokenType::Equals, "Missing '=' in Let statement");

    const Span &equals_span = ctx.current_token().span;
    auto expr = parse_expression(ctx);
    if (!expr) {
      spdlog::error("Expected Expression at {} but couldn't find one",
                    equals_span.start.to_string());
      std::exit(1);
    }
    let_stmt->expression = expr;
    let_stmt->span() = {statement_start.start, expr->span().end};

    // Symbols
    ctx.program->symbols[ident_expr->name] =
        std::make_shared<Symbol>(ident_expr->name, nullptr, ident_tok.span);

    return let_stmt;
  }

  if (tok.type == TokenType::LCurly) {
    auto block_stmt = std::make_shared<Block>();
    ctx.consume();

    while (ctx.current < ctx.tokens.size() &&
           ctx.current_token().type != TokenType::RCurly) {
      auto stmt = parse_statement(ctx);
      if (stmt) {
        block_stmt->statements.push_back(stmt);
      } else {
        break; // error handling or end
      }
    }

    ctx.consume_assert(TokenType::RCurly, "Missing '}' in Block statement");
    block_stmt->span() =
        Span(statement_start.start, ctx.previous_token_span().end);
    return block_stmt;
  }

  if (tok.type == TokenType::If) {
    auto if_stmt = std::make_shared<IfStatement>();
    ctx.consume();

    auto condition = parse_expression(ctx);
    if (!condition) {
      spdlog::error("Expected condition expression at {} but couldn't find one",
                    ctx.current_token().span.start.to_string());
      std::exit(1);
    }
    if_stmt->condition = condition;

    auto then_branch = parse_statement(ctx);
    if (!then_branch) {
      spdlog::error(
          "Expected then branch statement at {} but couldn't find one",
          ctx.current_token().span.start.to_string());
      std::exit(1);
    }
    if_stmt->then_branch = then_branch;

    if (ctx.current < ctx.tokens.size() &&
        ctx.current_token().type == TokenType::Else) {
      ctx.consume();
      auto else_branch = parse_statement(ctx);
      if (!else_branch) {
        spdlog::error(
            "Expected else branch statement at {} but couldn't find one",
            ctx.current_token().span.start.to_string());
        std::exit(1);
      }
      if_stmt->else_branch = else_branch;
    }

    if_stmt->span() =
        Span(statement_start.start, ctx.previous_token_span().end);
    return if_stmt;
  }

  if (tok.type == TokenType::While) {
    auto while_stmt = std::make_shared<WhileLoop>();
    ctx.consume();

    auto condition = parse_expression(ctx);
    if (!condition) {
      spdlog::error("Expected condition expression at {} but couldn't find one",
                    ctx.current_token().span.start.to_string());
      std::exit(1);
    }
    while_stmt->condition = condition;

    auto body = parse_statement(ctx);
    if (!body) {
      spdlog::error("Expected body statement at {} but couldn't find one",
                    ctx.current_token().span.start.to_string());
      std::exit(1);
    }
    while_stmt->body = body;

    while_stmt->span() =
        Span(statement_start.start, ctx.previous_token_span().end);
    return while_stmt;
  }

  // Maybe its a dangling expression
  auto expr = parse_expression(ctx);
  if (expr) {
    auto expr_stmt = std::make_shared<ExpressionStatement>();
    expr_stmt->expression = expr;
    expr_stmt->span() = expr->span();
    return expr_stmt;
  }

  return nullptr;
}

std::shared_ptr<Program> parse(const std::vector<Token> &tokens) {
  auto program = std::make_shared<Program>();
  ParserContext ctx{program, tokens, std::make_shared<ModuleContext>()};
  // Try to get the file path from the first token, if available
  if (!tokens.empty()) {
    ctx.current_file_path = std::string(tokens[0].span.start.file_name);
  }

  while (ctx.current < tokens.size()) {
    auto stmt = parse_statement(ctx);
    if (stmt) {
      program->statements.push_back(stmt);
    } else {
      spdlog::error("Couldn't parse statement at {}",
                    ctx.current_token().span.start.to_string());
      std::exit(1);
    }
  }

  return program;
}

void ParserContext::consume_assert(TokenType type, const std::string &message) {
  if (current_token().type != type) {
    spdlog::error("{}, got {} instead", message,
                  magic_enum::enum_name(current_token().type));
    std::exit(1);
  }
  consume();
}
