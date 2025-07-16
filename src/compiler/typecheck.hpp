#pragma once
#include "../definitions/ast.hpp"
#include "../definitions/tokens.hpp"
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>
#include <iostream>
#include "modules.hpp"

void typecheck(Ref<Program> program);

struct TypecheckContext {
  Ref<Program> program;
  const std::vector<Token> &tokens;
  Ref<ModuleContext> module_context;
  std::string current_file_path;

  size_t current = 0;
};

