#pragma once

#include "../definitions/ast.hpp"
#include "../definitions/types.hpp"
#include "typecheck.hpp"

// Main injection function to be called before typechecking
void perform_injections(Ref<Program> program);

// Individual injection functions
Ref<Call> inject_enum_to_string_call(Ref<Enum> enum_struct,
                                     Ref<Expression> enum_value, Span span);
Ref<FunctionDefinition> inject_enum_to_string(Ref<Enum> enum_struct);
void inject_enum_to_string_in_scope(Ref<TypecheckContext> ctx, Ref<Enum> enum_struct);