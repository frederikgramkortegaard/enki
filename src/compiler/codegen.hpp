#pragma once

#include <string>
#include "../definitions/ast.hpp"
#include "../definitions/types.hpp"

struct CodegenContext {
    std::string output;
};

std::string codegen(Ref<Program> program);