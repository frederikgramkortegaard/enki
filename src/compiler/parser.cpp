#include "parser.hpp"
#include "../utils/logging.hpp"
#include <format>
#include <iostream>
#include <spdlog/spdlog.h>

bool is_assignable(Ref<Expression> expr) {
  return expr->get_type() == ASTType::Identifier;
}

Ref<Identifier> parse_identifier(ParserContext &ctx);
Ref<Expression> parse_atom(ParserContext &ctx);

Ref<Expression> parse_prefix_op(ParserContext &ctx) {
  spdlog::debug("[parser] Entering parse_prefix_op at token {}",
                magic_enum::enum_name(ctx.current_token().type));
  const Token &tok = ctx.current_token();
  switch (tok.type) {
    case TokenType::Ampersand: {
      spdlog::debug("[parser] Found address-of operator");
      ctx.consume();
      auto addr_expr = std::make_shared<AddressOf>();
      addr_expr->expression = parse_prefix_op(ctx);
      if (!addr_expr->expression) {
        LOG_ERROR_EXIT("[parser] Expected expression after '&'",
                       tok.span, *ctx.program->source_buffer);
      }
      addr_expr->span = tok.span;
      return addr_expr;
    }

    case TokenType::Asterisk: {
      spdlog::debug("[parser] Found dereference operator");
      ctx.consume();
      auto deref_expr = std::make_shared<Dereference>();
      deref_expr->expression = parse_prefix_op(ctx);
      if (!deref_expr->expression) {
        LOG_ERROR_EXIT("[parser] Expected expression after '*'",
                       tok.span, *ctx.program->source_buffer);
      }
      deref_expr->span = tok.span;
      return deref_expr;
    }

    default:
      return parse_atom(ctx);
  }
}

Ref<Expression> parse_atom(ParserContext &ctx) {
  spdlog::debug("[parser] Entering parse_atom at token {}",
                magic_enum::enum_name(ctx.current_token().type));
  const Token &tok = ctx.tokens[ctx.current];

  switch (tok.type) {
  case TokenType::Int:
  case TokenType::Float:
  case TokenType::String:
  case TokenType::Char: {
    auto lit = std::make_shared<Literal>();
    lit->type = std::make_shared<Type>(Type{token_to_literal_type(tok.type)});
    lit->value = tok.value;
    lit->span = tok.span;
    ctx.consume();
    return lit;
  }
  case TokenType::StructType: {
    // Struct Instantiation
    spdlog::debug("[parser] Found struct instantiation");
  
      ctx.consume();
      auto ident = parse_identifier(ctx);
      if (!ident) {
        LOG_ERROR_EXIT("[parser] Expected identifier after 'struct' keyword in struct instantiation",
                       tok.span, *ctx.program->source_buffer);
      }

      ctx.consume_assert(TokenType::LCurly, "Missing '{' in Struct Instantiation");


      auto struct_inst = std::make_shared<StructInstantiation>();
      struct_inst->identifier = ident;

      while (ctx.current < ctx.tokens.size() && ctx.current_token().type != TokenType::RCurly) {
        spdlog::debug("[parser] Parsing struct argument");
        auto arg = parse_expression(ctx);
        if (!arg) {
          LOG_ERROR_EXIT("[parser] Expected expression as struct argument but found '" +
                         std::string(ctx.current_token().value) + "' (" +
                         std::string(magic_enum::enum_name(ctx.current_token().type)) +
                         ")",
                         ctx.current_token().span, *ctx.program->source_buffer);
        }
        struct_inst->arguments.push_back(arg);
        ctx.consume_if(TokenType::Comma);
      }
      ctx.consume_assert(TokenType::RCurly, "Missing '}' in Struct Instantiation");
      struct_inst->span = Span(struct_inst->identifier->span.start, ctx.previous_token_span().end);
      return struct_inst;


  }
  case TokenType::Identifier: {
    spdlog::debug("[parser] Found identifier: {}", tok.value);

    // Try to parse as function call first
    auto call_expr = parse_call(ctx);
    if (call_expr) {
      return call_expr;
    }

    auto ident = std::make_shared<Identifier>();
    ident->name = tok.value;
    ident->span = tok.span;
    ctx.consume();

    if (ctx.current < ctx.tokens.size() &&
        ctx.current_token().type == TokenType::Dot) {
      ctx.consume();
      auto dot_expr = std::make_shared<Dot>();
      dot_expr->left = ident;
      dot_expr->right = parse_expression(
          ctx); // Parse as full expression, not just identifier
      if (!dot_expr->right) {
        LOG_ERROR_EXIT(
            "[parser] Expected expression after '.' in dot expression",
            ctx.current_token().span, *ctx.program->source_buffer);
      }
      dot_expr->span = Span(ident->span.start, dot_expr->right->span.end);
      utils::ast::print_ast(dot_expr);
      return dot_expr;
    }

    return ident;
  }
  case TokenType::True:
  case TokenType::False: {
    auto bool_lit = std::make_shared<Literal>();
    bool_lit->type = std::make_shared<Type>(Type{BaseType::Bool});
    bool_lit->value = tok.type == TokenType::True ? "true" : "false";
    bool_lit->span = tok.span;
    ctx.consume();
    return bool_lit;
  }
  default:
    break;
  }

  return nullptr;
}
Ref<Type> parse_type(ParserContext &ctx) {
  spdlog::debug("[parser] Entering parse_type at token {}",
                magic_enum::enum_name(ctx.current_token().type));

  auto type = std::make_shared<Type>();
  type->span = ctx.current_token().span;

  switch (ctx.current_token().type) {
  case TokenType::IntType:
    type->base_type = BaseType::Int;
    break;
  case TokenType::FloatType:
    type->base_type = BaseType::Float;
    break;
  case TokenType::StringType:
    type->base_type = BaseType::String;
    break;
  case TokenType::BoolType:
    type->base_type = BaseType::Bool;
    break;
  case TokenType::VoidType:
    type->base_type = BaseType::Void;
    break;
  case TokenType::CharType:
    type->base_type = BaseType::Char;
    break;

  // This probably should only be used in very specific cases, like as the Parameter type of an extern function e.g. sizeof(type)
  case TokenType::TypeType:
    type->base_type = BaseType::Type;
    break;

  //@NOTE : This could be an enum or a struct, but we don't know yet before the
  // typechecker
  case TokenType::Identifier:
    type->base_type = BaseType::Unknown;
    type->name = ctx.current_token().value; // Store the type name
    break;

  case TokenType::Ampersand: {
    type->base_type = BaseType::Pointer;
    ctx.consume();
    type->structure = parse_type(ctx);
    // Don't want to break out and over-consume the next token
    return type;
  }

  default:
    LOG_ERROR_EXIT(
        "[parser] Expected type keyword but got " +
            std::string(magic_enum::enum_name(ctx.current_token().type)),
        ctx.current_token().span, *ctx.program->source_buffer);
  }

  ctx.consume();
  return type;
}

Ref<Variable> parse_struct_field(ParserContext &ctx) {
  spdlog::debug("[parser] Entering parse_struct_field at token {}",
                magic_enum::enum_name(ctx.current_token().type));

  auto field = std::make_shared<Variable>();
  auto ident = parse_identifier(ctx);
  if (!ident) {
    LOG_ERROR_EXIT("[parser] Expected identifier after 'struct' keyword",
                   ctx.current_token().span, *ctx.program->source_buffer);
  }
  field->name = ident->name;
  field->span = ident->span;
  ctx.consume_assert(TokenType::Colon, "Missing ':' in Struct Field");
  std::cout << "Parsing type for struct field " << field->name << std::endl;
  std::cout << "Current token: " << ctx.current_token().value << std::endl;
  field->type = parse_type(ctx);
  
  /* @NOTE : here we'd normally check if the type is none, but we can't because consider the situation
  
  struct Point {
    x:
    y: int
  }

  when we try to parse the type for x, the current token is y, so parse_type will actually return a type (BaseType::Unknown) becuase
  it considers that y potentially could be the name of e.g. another struct or enum etc. So what will happen now,
  is that we will leave it until it becomes a problem, becasue the problem doesnt parse becuase parse_type consumes tokens anyways,
  so it the application wont run anyways
  */
  field->span = Span(ident->span.start, field->type->span.end);
  return field;
}

Ref<StructDefinition> parse_struct(ParserContext &ctx) {
  spdlog::debug("[parser] Entering parse_struct at token {}",
                magic_enum::enum_name(ctx.current_token().type));


  ctx.consume_assert(TokenType::StructType, "Missing 'struct' keyword - this should never happen");
  auto struct_def = std::make_shared<StructDefinition>();
  struct_def->identifier = parse_identifier(ctx);
  if (!struct_def->identifier) {
    LOG_ERROR_EXIT("[parser] Expected identifier after 'struct' keyword",
                   ctx.current_token().span, *ctx.program->source_buffer);
  }
  ctx.consume_assert(TokenType::LCurly, "Missing '{' in Struct Definition");


  // Parse struct fields
  while (ctx.current < ctx.tokens.size() &&
         ctx.current_token().type != TokenType::RCurly) {
    auto field = parse_struct_field(ctx);
    if (!field) {
      break;
    }
    struct_def->fields.push_back(field);
    if (ctx.current_token().type == TokenType::Comma) {
      ctx.consume();
    }
    ctx.consume_if(TokenType::Comma);
  }

  ctx.consume_assert(TokenType::RCurly, "Missing '}' in Struct Definition");

  struct_def->span =
      Span(struct_def->identifier->span.start, ctx.previous_token_span().end);
  utils::ast::print_ast(struct_def);
  return struct_def;
}
Ref<Variable> parse_enum_member(ParserContext &ctx) {

  spdlog::debug("[parser] Entering parse_enum_member at token {}",
                magic_enum::enum_name(ctx.current_token().type));

  auto var = std::make_shared<Variable>();
  var->name = ctx.current_token().value;
  auto ident = parse_identifier(ctx);
  if (!ident) {
    LOG_ERROR_EXIT("[parser] Expected identifier after 'enum' keyword",
                   ctx.current_token().span, *ctx.program->source_buffer);
  }
  var->span = ident->span;
  return var;
}

Ref<EnumDefinition> parse_enum(ParserContext &ctx) {
  spdlog::debug("[parser] Entering parse_enum at token {}",
                magic_enum::enum_name(ctx.current_token().type));

  auto enum_def = std::make_shared<EnumDefinition>();
  ctx.consume_assert(TokenType::EnumType, "Missing 'enum' keyword");

  // Parse the enum identifier
  enum_def->identifier = parse_identifier(ctx);
  if (!enum_def->identifier) {
    LOG_ERROR_EXIT("[parser] Expected identifier after 'enum' keyword",
                   ctx.current_token().span, *ctx.program->source_buffer);
  }

  ctx.consume_assert(TokenType::LCurly, "Missing '{' in Enum Definition");

  // Create the enum type
  auto enum_type = std::make_shared<Type>();
  enum_type->base_type = BaseType::Enum;
  enum_def->enum_type = enum_type;

  // Create the Enum struct for the variant (There really isn't a lot of reason
  // to even have this EnumDefinition AST node, since we can just use the Enum
  // struct directly, maybe &ref the Token/AST but it's fine to have both I want
  // it.)
  auto enum_struct = std::make_shared<Enum>();
  enum_struct->name = enum_def->identifier->name;
  enum_struct->span = enum_def->span;
  enum_type->structure = enum_struct;

  // Parse enum members
  while (ctx.current < ctx.tokens.size() &&
         ctx.current_token().type != TokenType::RCurly) {
    auto member = parse_enum_member(ctx);
    if (!member) {
      break;
    }

    member->type = enum_type;
    enum_def->members.push_back(member);
    enum_struct->members[member->name] = member;

    ctx.consume_if(TokenType::Comma);
  }

  ctx.consume_assert(TokenType::RCurly, "Missing '}' in Enum Definition");

  // Set the span
  enum_def->span =
      Span(enum_def->identifier->span.start, ctx.previous_token_span().end);
  return enum_def;
}

Ref<Identifier> parse_identifier(ParserContext &ctx) {
  spdlog::debug("[parser] Entering parse_identifier at token {}",
                magic_enum::enum_name(ctx.current_token().type));
  const Token &tok = ctx.tokens[ctx.current];
  if (tok.type != TokenType::Identifier) {
    LOG_ERROR_EXIT("[parser] Expected Identifier but got " +
                       std::string(magic_enum::enum_name(tok.type)),
                   tok.span, *ctx.program->source_buffer);
  }
  auto ident = std::make_shared<Identifier>();
  ident->name = tok.value;
  ident->span = tok.span;
  ctx.consume();
  return ident;
}

Ref<Expression> parse_call(ParserContext &ctx) {
  spdlog::debug("[parser] Entering parse_call at token {}",
                magic_enum::enum_name(ctx.current_token().type));
  
  // Function Call
  if (!ctx.eof() && ctx.peek(1).type == TokenType::LParens) {
    auto call_expr = std::make_shared<Call>();
    auto ident = std::make_shared<Identifier>();
    ident->name = ctx.current_token().value;
    ident->span = ctx.current_token().span;
    call_expr->callee = ident;

    ctx.consume(); // consume identifier
    ctx.consume(); // consume LParens

    while (ctx.current < ctx.tokens.size() &&
           ctx.current_token().type != TokenType::RParens) {
      auto arg = parse_expression(ctx);
      if (!arg) {
        LOG_ERROR_EXIT(
            "[parser] Expected expression as function argument but found '" +
                std::string(ctx.current_token().value) + "' (" +
                std::string(magic_enum::enum_name(ctx.current_token().type)) +
                ")",
            ctx.current_token().span, *ctx.program->source_buffer);
      }
      call_expr->arguments.push_back(arg);
      ctx.consume_if(TokenType::Comma);
    }

    if (ctx.eof()) {
      LOG_ERROR_EXIT(
          "[parser] Missing closing parenthesis ')' in function call",
          call_expr->callee->span, *ctx.program->source_buffer);
    }

    if (ctx.current_token().type != TokenType::RParens) {
      LOG_ERROR_EXIT(
          "[parser] Missing closing parenthesis ')' in function call, found "
          "'" +
              std::string(ctx.current_token().value) + "' (" +
              std::string(magic_enum::enum_name(ctx.current_token().type)) +
              ") instead",
          ctx.current_token().span, *ctx.program->source_buffer);
    }

    ctx.consume(); // consume the RParens
    call_expr->span = ident->span;
    return call_expr;
  }
  
  return nullptr;
}

Ref<Expression> parse_expression(ParserContext &ctx) {
  spdlog::debug("[parser] Entering parse_expression at token {}",
                magic_enum::enum_name(ctx.current_token().type));
  auto left = parse_prefix_op(ctx);
  if (!left)
    return nullptr;

  // Shunting Yard Algorithm for Binary Operators
  std::vector<Ref<Expression>> output;
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
      bin_expr->span = Span(left_expr->span.start, right->span.end);
      output.push_back(bin_expr);
      ops.pop_back();
    }

    auto right = parse_prefix_op(ctx);
    if (!right) {
      LOG_ERROR_EXIT("[parser] Expected right operand for binary operator",
                     tok.span, *ctx.program->source_buffer);
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
    bin_expr->span = Span(left_expr->span.start, right->span.end);
    output.push_back(bin_expr);
    ops.pop_back();
  }

  return output.size() == 1 ? output[0] : nullptr;
}

Ref<VarDecl> parse_parameter(ParserContext &ctx) {
  spdlog::debug("[parser] Entering parse_parameter at token {}",
                magic_enum::enum_name(ctx.current_token().type));
  auto var_decl = std::make_shared<VarDecl>();
  var_decl->identifier = parse_identifier(ctx);
  ctx.consume_assert(TokenType::Colon, "Missing ':' in Variable Declaration");
  var_decl->type = parse_type(ctx);
  var_decl->span =
      Span(var_decl->identifier->span.start, var_decl->type->span.end);
  return var_decl;
}


Ref<Extern> parse_extern(ParserContext &ctx) {
  spdlog::debug("[parser] Entering parse_extern at token {}",
                magic_enum::enum_name(ctx.current_token().type));
  auto extern_stmt = std::make_shared<Extern>();
  // extern malloc(int) -> &void from "libc"

  ctx.consume_assert(TokenType::Extern, "Missing 'extern' keyword");
  extern_stmt->identifier = parse_identifier(ctx);
  if (!extern_stmt->identifier) {
    LOG_ERROR_EXIT("[parser] Expected identifier after 'extern' keyword",
                   ctx.current_token().span, *ctx.program->source_buffer);
  }
  ctx.consume_assert(TokenType::LParens, "Missing '(' in Extern Declaration");

  while (ctx.current < ctx.tokens.size() &&
         ctx.current_token().type != TokenType::RParens) {
    auto type = parse_type(ctx);
    extern_stmt->args.push_back(type);
    ctx.consume_if(TokenType::Comma);
  }
  ctx.consume_assert(TokenType::RParens, "Missing ')' in Extern Declaration");

  ctx.consume_assert(TokenType::Arrow, "Missing return type declaration arrow '->' in Extern Declaration");
  extern_stmt->return_type = parse_type(ctx);
  if (!extern_stmt->return_type) {
    LOG_ERROR_EXIT("[parser] Expected return type in Extern Declaration",
                   ctx.current_token().span, *ctx.program->source_buffer);
  }
  ctx.consume_assert(TokenType::From, "Missing 'from' keyword in Extern Declaration");
  auto mpath = parse_atom(ctx);
  if (!mpath) {
    LOG_ERROR_EXIT("[parser] Expected module path in Extern Declaration",
                   ctx.current_token().span, *ctx.program->source_buffer);
  }
  if (mpath->get_type() != ASTType::Literal || std::dynamic_pointer_cast<Literal>(mpath)->type->base_type != BaseType::String) {
    std::cout << "mpath->get_type() = " << magic_enum::enum_name(mpath->get_type()) << std::endl;
    LOG_ERROR_EXIT("[parser] Expected string literal for module path in Extern Declaration",
                   mpath->span, *ctx.program->source_buffer);
  }
  extern_stmt->module_path = std::dynamic_pointer_cast<Literal>(mpath)->value;

  extern_stmt->span = Span(extern_stmt->identifier->span.start, ctx.previous_token_span().end);
  return extern_stmt;
}

// @NOTE DOES NOT CONSUME {}
Ref<Block> parse_block(ParserContext &ctx) {
  spdlog::debug("[parser] Entering parse_block at token {} with value '{}'",
                magic_enum::enum_name(ctx.current_token().type),
                ctx.current_token().value);
  auto block = std::make_shared<Block>();
  // Create a new scope for this block
  block->scope = std::make_shared<Scope>();
  block->scope->parent = ctx.current_scope;
  ctx.current_scope->children.push_back(block->scope);

  // Enter the new scope
  ctx.current_scope = block->scope;

  while (ctx.current < ctx.tokens.size() &&
         ctx.current_token().type != TokenType::RCurly) {
    spdlog::debug(
        "[parser] parse_block: parsing statement at token {} with value '{}'",
        magic_enum::enum_name(ctx.current_token().type),
        ctx.current_token().value);
    auto stmt = parse_statement(ctx);
    if (stmt) {
      spdlog::debug("[parser] parse_block: added statement of type {}",
                    magic_enum::enum_name(stmt->get_type()));
      block->statements.push_back(stmt);
    } else {
      spdlog::debug("[parser] parse_block: got null statement, considering this end of block");
      break; // error handling or end
    }
  }

  spdlog::debug(
      "[parser] parse_block: finished, current token is {} with value '{}'",
      magic_enum::enum_name(ctx.current_token().type),
      ctx.current_token().value);
  // Restore previous scope
  ctx.current_scope = block->scope->parent;
  return block;
}

Ref<Statement> parse_statement(ParserContext &ctx) {
  if (ctx.eof() || ctx.current_token().type == TokenType::Eof) {
    return nullptr;
  }
  spdlog::debug("[parser] Entering parse_statement at token {} with value '{}'",
                magic_enum::enum_name(ctx.current_token().type),
                ctx.current_token().value);
  const Token &tok = ctx.tokens[ctx.current];
  Span statement_start = tok.span;

  
  if (tok.type == TokenType::StructType) {
    auto struct_def = parse_struct(ctx);
    return struct_def;
  }

  if (tok.type == TokenType::Extern) {
    auto extern_stmt = parse_extern(ctx);
    return extern_stmt;
  } 

  if (tok.type == TokenType::EnumType) {
    auto enum_def = parse_enum(ctx);
    return enum_def;
  }

  if (tok.type == TokenType::Define) {
    //@TODO : make this into a helper function instead
    spdlog::debug("[parser] Entering function definition parsing at token {} "
                  "with value '{}'",
                  magic_enum::enum_name(ctx.current_token().type),
                  ctx.current_token().value);

    auto function_def = std::make_shared<FunctionDefinition>();
    function_def->function = std::make_shared<Function>();
    function_def->function->definition = function_def;

    ctx.consume();
    function_def->identifier = parse_identifier(ctx);
    ctx.consume_assert(TokenType::LParens,
                       "Missing '(' in Function Definition");

    while (ctx.current < ctx.tokens.size() &&
           ctx.current_token().type != TokenType::RParens) {
      auto param = parse_parameter(ctx);
      function_def->parameters.push_back(param);
      ctx.consume_if(TokenType::Comma);
    }

    ctx.consume_assert(TokenType::RParens,
                       "Missing ')' in Function Definition");
    ctx.consume_assert(TokenType::Arrow, "Missing '->' in Function Definition");
    function_def->return_type = parse_type(ctx);
    ctx.consume_assert(TokenType::LCurly, "Missing '{' in Function Definition");
    spdlog::debug("[parser] Function definition: about to parse body, current "
                  "token is {} with value '{}'",
                  magic_enum::enum_name(ctx.current_token().type),
                  ctx.current_token().value);
    function_def->body = parse_block(ctx);
    spdlog::debug("[parser] Function definition: body parsed, current token is "
                  "{} with value '{}'",
                  magic_enum::enum_name(ctx.current_token().type),
                  ctx.current_token().value);
    ctx.consume_assert(TokenType::RCurly, "Missing '}' in Function Definition");
    spdlog::debug("[parser] Function definition: consumed closing brace, "
                  "current token is {} with value '{}'",
                  magic_enum::enum_name(ctx.current_token().type),
                  ctx.current_token().value);
    function_def->span = Span(tok.span.start, ctx.previous_token_span().end);
    ctx.consume_if(TokenType::Semicolon);
    spdlog::debug("[parser] Function definition: finished, current token is {} "
                  "with value '{}'",
                  magic_enum::enum_name(ctx.current_token().type),
                  ctx.current_token().value);
    return function_def;
  }

  if (tok.type == TokenType::Import) {
    spdlog::debug("[parser] Entering parse_statement at token {}",
                  magic_enum::enum_name(ctx.current_token().type));
    auto import_stmt = std::make_shared<Import>();
    ctx.consume();
    ctx.consume_assert(TokenType::LessThan, "Missing '<' in Import statement");
    auto module_path = parse_atom(ctx);
    if (!module_path) {
      LOG_ERROR_EXIT("[parser] Expected module path in Import statement",
                     tok.span, *ctx.program->source_buffer);
    }
    import_stmt->module_path = std::dynamic_pointer_cast<Literal>(module_path);
    spdlog::debug("[parser] Module path: {}", import_stmt->module_path->value);
    import_stmt->span = Span(tok.span.start, module_path->span.end);

    // Use CTX Module Context to add the module to the program
    ctx.module_context->add_module(std::string(import_stmt->module_path->value),
                                   ctx.current_file_path);

    ctx.consume_assert(TokenType::GreaterThan,
                       "Missing '>' in Import statement");
    return import_stmt;
  }

  if (tok.type == TokenType::Return) {
    auto return_stmt = std::make_shared<Return>();
    ctx.consume();
    return_stmt->expression = parse_expression(ctx);
    if (return_stmt->expression) {
      return_stmt->span =
          Span(tok.span.start, return_stmt->expression->span.end);
    } else {
      return_stmt->span = tok.span;
    }
    ctx.consume_if(TokenType::Semicolon);
    return return_stmt;
  }

  if (tok.type == TokenType::Let) {
    auto var_decl = std::make_shared<VarDecl>();
    ctx.consume();

    const Token &ident_tok = ctx.current_token();
    if (ident_tok.type != TokenType::Identifier) {
      LOG_ERROR_EXIT("[parser] Expected Identifier but got " +
                         std::string(magic_enum::enum_name(ident_tok.type)),
                     ident_tok.span, *ctx.program->source_buffer);
    }

    auto ident_expr = std::make_shared<Identifier>();
    ident_expr->name = ident_tok.value;
    ident_expr->span = ident_tok.span;
    var_decl->identifier = ident_expr;
    ctx.consume();

    ctx.consume_assert(TokenType::Equals, "Missing '=' in Let statement");

    auto expr = parse_expression(ctx);
    if (!expr) {
      LOG_ERROR_EXIT(
          "[parser] Expected expression after '=' but found '" +
              std::string(ctx.current_token().value) + "' (" +
              std::string(magic_enum::enum_name(ctx.current_token().type)) +
              ")",
          ctx.current_token().span, *ctx.program->source_buffer);
    }
    var_decl->expression = expr;
    var_decl->span = {statement_start.start, expr->span.end};

    return var_decl;
  }

  if (tok.type == TokenType::LCurly) {
    auto block_stmt = std::make_shared<Block>();
    ctx.consume();

    // Create a new scope for this block
    block_stmt->scope = std::make_shared<Scope>();
    block_stmt->scope->parent = ctx.current_scope;
    ctx.current_scope->children.push_back(block_stmt->scope);

    // Enter the new scope
    ctx.current_scope = block_stmt->scope;

    while (ctx.current < ctx.tokens.size() &&
           ctx.current_token().type != TokenType::RCurly) {
      auto stmt = parse_statement(ctx);
      if (stmt) {
        block_stmt->statements.push_back(stmt);
      } else {
        break; // error handling or end
      }
    }

    // Consume the closing brace
    if (ctx.current < ctx.tokens.size() &&
        ctx.current_token().type == TokenType::RCurly) {
      ctx.consume();
    }

    block_stmt->span =
        Span(statement_start.start, ctx.previous_token_span().end);

    // Restore previous scope
    ctx.current_scope = block_stmt->scope->parent;
    return block_stmt;
  }

  if (tok.type == TokenType::If) {
    auto if_node = std::make_shared<If>();
    ctx.consume();

    auto condition = parse_expression(ctx);
    if (!condition) {
      LOG_ERROR_EXIT(
          "[parser] Expected condition expression but found '" +
              std::string(ctx.current_token().value) + "' (" +
              std::string(magic_enum::enum_name(ctx.current_token().type)) +
              ")",
          ctx.current_token().span, *ctx.program->source_buffer);
    }
    if_node->condition = condition;

    auto then_branch = parse_block(ctx);
    if (!then_branch) {
      LOG_ERROR_EXIT(
          "[parser] Expected then branch block but found '" +
              std::string(ctx.current_token().value) + "' (" +
              std::string(magic_enum::enum_name(ctx.current_token().type)) +
              ")",
          ctx.current_token().span, *ctx.program->source_buffer);
    }
    if_node->then_branch = then_branch;

    if (ctx.current < ctx.tokens.size() &&
        ctx.current_token().type == TokenType::Else) {
      ctx.consume();
      auto else_branch = parse_block(ctx);
      if (!else_branch) {
        LOG_ERROR_EXIT(
            "[parser] Expected else branch block but found '" +
                std::string(ctx.current_token().value) + "' (" +
                std::string(magic_enum::enum_name(ctx.current_token().type)) +
                ")",
            ctx.current_token().span, *ctx.program->source_buffer);
      }
      if_node->else_branch = else_branch;
    }

    if_node->span = Span(statement_start.start, ctx.previous_token_span().end);
    return if_node;
  }

  if (tok.type == TokenType::While) {
    auto while_stmt = std::make_shared<While>();
    ctx.consume();

    auto condition = parse_expression(ctx);
    if (!condition) {
      LOG_ERROR_EXIT(
          "[parser] Expected condition expression but found '" +
              std::string(ctx.current_token().value) + "' (" +
              std::string(magic_enum::enum_name(ctx.current_token().type)) +
              ")",
          ctx.current_token().span, *ctx.program->source_buffer);
    }
    while_stmt->condition = condition;

    // Check for opening brace before parsing block
    if (ctx.current_token().type != TokenType::LCurly) {
      LOG_ERROR_EXIT(
          "[parser] Expected '{' for while loop body but found '" +
              std::string(ctx.current_token().value) + "' (" +
              std::string(magic_enum::enum_name(ctx.current_token().type)) +
              ")",
          ctx.current_token().span, *ctx.program->source_buffer);
    }

    auto body = parse_block(ctx);
    if (!body) {
      LOG_ERROR_EXIT(
          "[parser] Expected body block but found '" +
              std::string(ctx.current_token().value) + "' (" +
              std::string(magic_enum::enum_name(ctx.current_token().type)) +
              ")",
          ctx.current_token().span, *ctx.program->source_buffer);
    }
    while_stmt->body = body;

    while_stmt->span =
        Span(statement_start.start, ctx.previous_token_span().end);
    return while_stmt;
  }

  // Maybe its a function call expression statement or assignment
  auto expr = parse_expression(ctx);
  if (expr) {
    // Check if this is an assignment
    if (ctx.current_token().type == TokenType::Equals && is_assignable(expr)) {
      auto assignment = std::make_shared<Assignment>();
      assignment->assignee = expr;
      ctx.consume_assert(TokenType::Equals, "Missing '=' in Assignment");
      assignment->expression = parse_expression(ctx);
      assignment->span =
          Span(expr->span.start, assignment->expression->span.end);
      return assignment;
    }

    // Only allow function calls as expression statements
    if (expr->get_type() == ASTType::Call) {
      auto expr_stmt = std::make_shared<ExpressionStatement>();
      expr_stmt->expression = expr;
      expr_stmt->span = expr->span;
      return expr_stmt;
    } else {
      // Dangling expressions that are not function calls are syntax errors
      LOG_ERROR_EXIT("[parser] Dangling expression is not allowed. Only "
                     "function calls can be used as statements. "
                     "Did you mean to assign this to a variable or use it in a "
                     "different context?",
                     expr->span, *ctx.program->source_buffer);
    }
  }

  return nullptr;
}

Ref<Program> parse(const std::vector<Token> &tokens,
                   std::shared_ptr<std::string> source_buffer,
                   Ref<ModuleContext> module_context) {
  spdlog::debug("[parser] Starting with {} tokens", tokens.size());
  for (size_t i = 0; i < tokens.size(); i++) {
    spdlog::debug("[parser] Token {}: {} = '{}'", i,
                  magic_enum::enum_name(tokens[i].type), tokens[i].value);
  }
  auto program = std::make_shared<Program>();
  program->source_buffer = source_buffer;
  program->module_context = module_context;
  ParserContext ctx{program, tokens, module_context};
  ctx.current_scope = program->scope;

  // Try to get the file path from the first token, if available
  if (!tokens.empty()) {
    ctx.current_file_path = std::string(tokens[0].span.start.file_name);
  }

  // Create a global block to contain all statements
  auto global_block = std::make_shared<Block>();
  global_block->scope = program->scope;
  global_block->span = program->span;

  while (!ctx.eof() && ctx.current_token().type != TokenType::Eof) {
    auto stmt = parse_statement(ctx);
    if (stmt) {
      global_block->statements.push_back(stmt);
    }
    else {
    spdlog::debug("[parser] parse: current token is {} with value '{}'",
                  magic_enum::enum_name(ctx.current_token().type),
                  ctx.current_token().value);
    spdlog::debug("[parser] parse: got null statement, considering this end of file");
    
    // To move on, from dangling stuff
    if (ctx.current_token().type != TokenType::Eof) {
      LOG_ERROR_EXIT("[parser] Expected EOF but found '" +
                     std::string(ctx.current_token().value) + "' (" +
                     std::string(magic_enum::enum_name(ctx.current_token().type)) +
                     ")",
                     ctx.current_token().span, *ctx.program->source_buffer);  
    }
    }
  }

  program->body = global_block;

  return program;
}

void ParserContext::consume_assert(TokenType type, const std::string &message) {
  spdlog::debug("[parser] consume_assert: expecting {}, got {} with value '{}'",
                magic_enum::enum_name(type),
                magic_enum::enum_name(current_token().type),
                current_token().value);
  if (current_token().type != type) {
    LOG_ERROR_EXIT(
        message + ", got " +
            std::string(magic_enum::enum_name(current_token().type)) +
            " instead",
        current_token().span, *program->source_buffer);
  }
  consume();
}
