#include "codegen.hpp"
#include "../utils/logging.hpp"
#include <spdlog/spdlog.h>

static void unimplemented(CodegenContext &ctx, ASTNode *node) {
  spdlog::error("[codegen] Unimplemented node kind: {}",
                magic_enum::enum_name(node->get_type()));
  spdlog::info("[codegen] current output: \n{}", ctx.output);
  std::exit(1);
}

static void gen_block(CodegenContext &ctx, Ref<Block> block);
static void gen_ast(CodegenContext &ctx, Ref<ASTNode> node);

static std::string type_with_name(Ref<Type> type, const std::string &name) {
  assert(type != nullptr && "Type is null");
  spdlog::debug("[codegen] Generating code for type: {}", type->to_string());
  // Here we would generate code for the type, if needed
  switch (type->base_type) {
  case BaseType::Int:
    return "int " + name;
  case BaseType::Float:
    return "float " + name;
  case BaseType::String:
    return "std::string " + name;
  case BaseType::Bool:
    return "bool " + name;
  case BaseType::Void:
    return "void " + name;
  case BaseType::Char:
    return "char " + name;
  case BaseType::Type:
    return "frederik_remove_basetype_type_pls " + name;
  case BaseType::Enum:
    return std::string(std::get<Ref<Enum>>(type->structure)->name) + " " + name;
  case BaseType::Struct:
    return std::string(std::get<Ref<Struct>>(type->structure)->name) + " " +
           name;
  case BaseType::Pointer:
    return type_with_name(std::get<Ref<Type>>(type->structure), "*" + name);

  case BaseType::Unknown:
  case BaseType::Any:
    spdlog::error("[codegen] Found an Any type in code generation, probably "
                  "missed by typechecker. Location: {}",
                  type->span.start.to_string());
    exit(1);
  default:
    spdlog::warn("[codegen] Unhandled base type: {}",
                 magic_enum::enum_name(type->base_type));
    exit(1);
  }
}

static void gen_enum_definition(CodegenContext &ctx,
                                Ref<EnumDefinition> enum_def) {
  spdlog::debug("[codegen] Generating code for enum definition: {}",
                enum_def->identifier->name);
  ctx.output +=
      "enum class " + std::string(enum_def->identifier->name) + " {\n";
  for (const auto &member : enum_def->members) {
    ctx.output += "  " + std::string(member->name) + ",\n";
  }
  ctx.output += "};\n";
}

static void gen_struct_definition(CodegenContext &ctx,
                                  Ref<StructDefinition> struct_def) {
  spdlog::debug("[codegen] Generating code for struct definition: {}",
                struct_def->identifier->name);
  ctx.output += "struct " + std::string(struct_def->identifier->name) + " {\n";
  for (const auto &field : struct_def->fields) {
    ctx.output +=
        "  " + type_with_name(field->type, std::string(field->name)) + ";\n";
  }
  ctx.output += "};\n";
}

static void gen_if_statement(CodegenContext &ctx, Ref<If> ifstmt) {
  spdlog::debug("[codegen] Generating code for if statement");
  ctx.output += "if (";
  gen_ast(ctx, ifstmt->condition);
  ctx.output += ")";
  gen_ast(ctx, ifstmt->then_branch);
  if (ifstmt->else_branch) {
    ctx.output += " else ";
    gen_ast(ctx, ifstmt->else_branch);
  }
}

static void gen_while_statement(CodegenContext &ctx, Ref<While> while_stmt) {
  spdlog::debug("[codegen] Generating code for while statement");
  ctx.output += "while (";
  gen_ast(ctx, while_stmt->condition);
  ctx.output += ")";
  gen_ast(ctx, while_stmt->body);
}

static void gen_var_decl(CodegenContext &ctx, Ref<VarDecl> var_decl) {
  spdlog::debug("[codegen] Generating code for variable declaration: {}",
                var_decl->identifier->name);
  ctx.output +=
      type_with_name(var_decl->type, std::string(var_decl->identifier->name));
  if (var_decl->expression) {
    ctx.output += " = ";
    gen_ast(ctx, var_decl->expression);
  }
  ctx.output += ";\n";
}

static void gen_call(CodegenContext &ctx, Ref<Call> call) {
  spdlog::debug("[codegen] Generating code for function call");

  if (auto ident = std::dynamic_pointer_cast<Identifier>(call->callee);
      ident && ident->name == "print") {
    ctx.output += "std::cout << ";
    for (const auto &arg : call->arguments) {
      gen_ast(ctx, arg);
      if (&arg != &call->arguments.back()) {
        ctx.output += " << ";
      }
    }
    ctx.output += " << std::endl;\n";
    return;
  }

  gen_ast(ctx, call->callee);
  ctx.output += "(";
  for (const auto &arg : call->arguments) {
    gen_ast(ctx, arg);
    if (&arg != &call->arguments.back()) {
      ctx.output += ", ";
    }
  }
  ctx.output += ")";
}

static void gen_struct_instantiation(CodegenContext &ctx, Ref<StructInstantiation> struct_inst) {
  spdlog::debug("[codegen] Generating code for struct instantiation");
  ctx.output += struct_inst->identifier->name;
  ctx.output += "{";
  for (size_t i = 0; i < struct_inst->arguments.size(); ++i) {
    gen_ast(ctx, struct_inst->arguments[i]);
    if (i < struct_inst->arguments.size() - 1) {
      ctx.output += ", ";
    }
  }
  ctx.output += "}";
}

static void gen_binary_op(CodegenContext &ctx, Ref<BinaryOp> binop) {
  spdlog::debug("[codegen] Generating code for binary operation: {}",
                magic_enum::enum_name(binop->op));
  ctx.output += "(";
  gen_ast(ctx, binop->left);
  switch (binop->op) {
  case BinaryOpType::Add:
    ctx.output += " + ";
    break;
  case BinaryOpType::Subtract:
    ctx.output += " - ";
    break;
  case BinaryOpType::Multiply:
    ctx.output += " * ";
    break;
  case BinaryOpType::Divide:
    ctx.output += " / ";
    break;
  case BinaryOpType::Modulo:
    ctx.output += " % ";
    break;
  case BinaryOpType::Equals:
    ctx.output += " == ";
    break;
  default:
    spdlog::error("[codegen] Unhandled binary operation: {}",
                  magic_enum::enum_name(binop->op));
    exit(1);
  }
  gen_ast(ctx, binop->right);
  ctx.output += ")";
}

static void gen_function_definition(CodegenContext &ctx,
                                    Ref<FunctionDefinition> func_def) {
  spdlog::debug("[codegen] Generating code for function: {}",
                func_def->identifier->name);
  if (!func_def->body)
    return;

  ctx.output += type_with_name(func_def->return_type,
                               std::string(func_def->identifier->name));
  ctx.output += "(";
  for (const auto &param : func_def->parameters) {
    ctx.output +=
        type_with_name(param->type, std::string(param->identifier->name));
    if (&param != &func_def->parameters.back()) {
      ctx.output += ", ";
    }
  }
  ctx.output += ")";

  // Generate code for the function body
  gen_ast(ctx, func_def->body);
}

static void gen_ast(CodegenContext &ctx, Ref<ASTNode> stmt) {
  spdlog::debug("[codegen] Generating code for statement of type: {}",
                magic_enum::enum_name(stmt->get_type()));
  switch (stmt->get_type()) {
  case ASTType::FunctionDefinition:
    gen_function_definition(ctx,
                            std::static_pointer_cast<FunctionDefinition>(stmt));
    break;
  case ASTType::EnumDefinition:
    gen_enum_definition(ctx, std::static_pointer_cast<EnumDefinition>(stmt));
    gen_function_definition(
        ctx,
        std::static_pointer_cast<EnumDefinition>(stmt)->to_string_function);
    break;
  case ASTType::Extern:
    // noop, externs don't codegen
    break;
  case ASTType::StructDefinition:
    gen_struct_definition(ctx,
                          std::static_pointer_cast<StructDefinition>(stmt));
    break;
  case ASTType::VarDecl:
    gen_var_decl(ctx, std::static_pointer_cast<VarDecl>(stmt));
    break;
  case ASTType::If:
    gen_if_statement(ctx, std::static_pointer_cast<If>(stmt));
    break;
  case ASTType::While:
    gen_while_statement(ctx, std::static_pointer_cast<While>(stmt));
    break;
  case ASTType::Block:
    gen_block(ctx, std::static_pointer_cast<Block>(stmt));
    break;
  case ASTType::BinaryOp:
    gen_binary_op(ctx, std::static_pointer_cast<BinaryOp>(stmt));
    break;
  case ASTType::Identifier:
    ctx.output += std::string(std::static_pointer_cast<Identifier>(stmt)->name);
    break;
  case ASTType::Literal: {
    auto literal = std::static_pointer_cast<Literal>(stmt);
    if (literal->type->base_type == BaseType::String) {
      ctx.output += "\"" + std::string(literal->value) + "\"";
    } else {
      ctx.output += std::string(literal->value);
    }
    break;
  }
  case ASTType::ExpressionStatement:
    gen_ast(ctx,
            std::static_pointer_cast<ExpressionStatement>(stmt)->expression);
    ctx.output += ";\n";
    break;
  case ASTType::StructInstantiation:
    gen_struct_instantiation(ctx,
            std::static_pointer_cast<StructInstantiation>(stmt));
    break;
  case ASTType::Return:
    ctx.output += "return ";
    gen_ast(ctx, std::static_pointer_cast<Return>(stmt)->expression);
    ctx.output += ";\n";
    break;
  case ASTType::Dereference:
    ctx.output += "(*(";
    gen_ast(ctx, std::static_pointer_cast<Dereference>(stmt)->expression);
    ctx.output += "))\n";
    break;
  case ASTType::AddressOf:
    ctx.output += "(&(";
    gen_ast(ctx, std::static_pointer_cast<AddressOf>(stmt)->expression);
    ctx.output += "))\n";
    break;
  case ASTType::Assignment:
    gen_ast(ctx, std::static_pointer_cast<Assignment>(stmt)->assignee);
    ctx.output += " = ";
    gen_ast(ctx, std::static_pointer_cast<Assignment>(stmt)->expression);
    ctx.output += ";\n";
    break;
  case ASTType::Call:
    gen_call(ctx, std::static_pointer_cast<Call>(stmt));
    break;
  case ASTType::Dot: {
    auto dot = std::static_pointer_cast<Dot>(stmt);

    gen_ast(ctx, dot->left);
    // TODO when to use :: vs dot . enum?
    if (dot->left->etype->base_type == BaseType::Enum) {
      ctx.output += "::";
    } else {
      ctx.output += ".";
    }
    gen_ast(ctx, dot->right);
    break;
  }
  default:
    unimplemented(ctx, stmt.get());
    break;
  }
}

static void gen_block(CodegenContext &ctx, Ref<Block> block) {
  spdlog::debug("[codegen] Generating code for block with {} statements",
                block->statements.size());
  ctx.output += "{\n";
  for (const auto &stmt : block->statements) {
    gen_ast(ctx, stmt);
  }
  ctx.output += "}\n";
}

std::string codegen(Ref<Program> program) {
  spdlog::debug("[codegen] Starting code generation for program");
  CodegenContext ctx;

  ctx.output += "#include <iostream>\n";
  ctx.output += "#include <string>\n";
  ctx.output += "#include <stdlib.h>\n";

  for (const auto &stmt : program->body->statements) {
      gen_ast(ctx, stmt);
  }
  spdlog::debug("[codegen] Code generation completed");
  return ctx.output;
}