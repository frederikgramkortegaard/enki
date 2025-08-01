#pragma once
#include "../definitions/ast.hpp"
#include "../definitions/tokens.hpp"
#include "../definitions/types.hpp"
#include "modules.hpp"
#include <iostream>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>

struct TypecheckContext {
  Ref<Program> program;
  std::vector<Ref<Function>> function_stack;
  std::vector<Ref<Scope>> scope_stack;
  Ref<Scope> global_scope;
  Ref<Block> current_block; // Track the current block being processed

  size_t current = 0;

  TypecheckContext(Ref<Program> program) : program(program) {
    global_scope = program->scope;
    scope_stack.push_back(global_scope);
    spdlog::debug("[typechecker] TypecheckContext created, global_scope = {}",
                  fmt::ptr(global_scope.get()));
  }

  // Helper methods for function stack management
  Ref<Function> current_function() const {
    return function_stack.empty() ? nullptr : function_stack.back();
  }

  void push_function(Ref<Function> func) { function_stack.push_back(func); }

  void pop_function() {
    if (!function_stack.empty()) {
      function_stack.pop_back();
    }
  }

  // Helper methods for scope stack management
  Ref<Scope> current_scope() const {
    return scope_stack.empty() ? nullptr : scope_stack.back();
  }

  void push_scope(Ref<Scope> scope) { scope_stack.push_back(scope); }

  void pop_scope() {
    if (!scope_stack.empty()) {
      scope_stack.pop_back();
    }
  }
};

void typecheck(Ref<Program> program);

// Function declarations for typechecking
void typecheck_if(Ref<TypecheckContext> ctx, Ref<If> if_stmt);
void typecheck_while(Ref<TypecheckContext> ctx, Ref<While> while_stmt);
void typecheck_import(Ref<TypecheckContext> ctx, Ref<Import> import_stmt);
void typecheck_extern(Ref<TypecheckContext> ctx, Ref<Extern> extern_stmt);

// Registration function declarations
void register_function_signature(Ref<TypecheckContext> ctx, Ref<FunctionDefinition> func_def);
void register_enum_signature(Ref<TypecheckContext> ctx, Ref<EnumDefinition> enum_def);
void register_struct_signature(Ref<TypecheckContext> ctx, Ref<StructDefinition> struct_def);
void perform_first_pass_registration(Ref<TypecheckContext> ctx, const std::vector<Ref<Statement>> &statements);
void perform_second_pass_typechecking(Ref<TypecheckContext> ctx, const std::vector<Ref<Statement>> &statements);
