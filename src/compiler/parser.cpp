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

std::shared_ptr<Statement> parse_statement(ParserContext &ctx) {
  const Token &tok = ctx.tokens[ctx.current];
  Span statement_start = tok.span;

  if (tok.type == TokenType::Extern) {
    auto extern_stmt = std::make_shared<ExternStatement>();
    ctx.consume();
    auto ident = parse_identifier(ctx);
    extern_stmt->identifier = ident;
    ctx.consume_assert(TokenType::LParens, "Missing '(' in Extern statement");
    while (ctx.current < ctx.tokens.size() &&
           ctx.current_token().type != TokenType::RParens) {
      auto arg = parse_type(ctx);
      extern_stmt->args.push_back(arg);
      ctx.consume_if(TokenType::Comma);
    }
    ctx.consume_assert(TokenType::RParens, "Missing ')' in Extern statement");
    ctx.consume_assert(TokenType::Arrow, "Missing '->' in Extern statement");
    extern_stmt->return_type = parse_type(ctx);
    ctx.consume_assert(TokenType::From, "Missing 'from' in Extern statement");
    extern_stmt->module_path = ctx.current_token().value;
    return extern_stmt;

    // @TODO Add to symtable
  }

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
