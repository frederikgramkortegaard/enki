#include "parser.hpp"
#include <format>
#include <iostream>

std::shared_ptr<Expression> parse_expression(ParserContext &ctx) {
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
          std::cerr << std::format(
              "Expected Expression at {} but couldn't find one\n",
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

std::shared_ptr<Statement> parse_statement(ParserContext &ctx) {
  const Token &tok = ctx.tokens[ctx.current];
  Span statement_start = tok.span;

  if (tok.type == TokenType::Let) {
    auto let_stmt = std::make_shared<LetStatement>();
    ctx.consume();

    const Token &ident_tok = ctx.current_token();
    if (ident_tok.type != TokenType::Identifier) {
      std::cerr << std::format("Expected Identifier at {} but got {}\n",
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
      std::cerr << std::format(
          "Expected Expression at {} but couldn't find one\n",
          equals_span.start.to_string());
      std::exit(1);
    }

    let_stmt->expression = expr;
    let_stmt->span() = Span(statement_start.start, expr->span().end);

    // Symbols
    ctx.program->symbols[ident_expr->name] =
        Symbol{ident_expr->name, nullptr, ident_tok.span};

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
    block_stmt->span() = Span(statement_start.start, ctx.previous_token_span().end);
    return block_stmt;
  }

  if (tok.type == TokenType::If) {
    auto if_stmt = std::make_shared<IfStatement>();
    ctx.consume();

    auto condition = parse_expression(ctx);
    if (!condition) {
      std::cerr << std::format(
          "Expected condition expression at {} but couldn't find one\n",
          ctx.current_token().span.start.to_string());
      std::exit(1);
    }
    if_stmt->condition = condition;

    auto then_branch = parse_statement(ctx);
    if (!then_branch) {
      std::cerr << std::format(
          "Expected then branch statement at {} but couldn't find one\n",
          ctx.current_token().span.start.to_string());
      std::exit(1);
    }
    if_stmt->then_branch = then_branch;

    if (ctx.current < ctx.tokens.size() &&
        ctx.current_token().type == TokenType::Else) {
      ctx.consume();
      auto else_branch = parse_statement(ctx);
      if (!else_branch) {
        std::cerr << std::format(
            "Expected else branch statement at {} but couldn't find one\n",
            ctx.current_token().span.start.to_string());
        std::exit(1);
      }
      if_stmt->else_branch = else_branch;
    }

    if_stmt->span() = Span(statement_start.start, ctx.previous_token_span().end);
    return if_stmt;
  }

  // Maybe its a dangling expression
  auto expr = parse_expression(ctx);
  if (expr) {
    return std::make_shared<ExpressionStatement>(expr);
  }

  return nullptr;
}

std::shared_ptr<Program> parse(std::vector<Token> tokens) {
  auto ctx = ParserContext{std::make_shared<Program>(), tokens, 0};

  // @TODO : In the future we add blocks/scopes
  while (ctx.current < ctx.tokens.size()) {
    auto stmt = parse_statement(ctx);
    if (stmt) {
      ctx.program->statements.push_back(stmt);
    } else {
      break; // error handling or end
    }
  }

  return ctx.program;
};
