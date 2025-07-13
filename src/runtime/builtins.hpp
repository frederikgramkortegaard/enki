#pragma once
#include "../interpreter/eval.hpp" // for Value
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct BuiltinFunction {
  std::string name;
  int min_args;
  int max_args;
  std::function<Value(const std::vector<Value> &)> impl;
};

// Table of builtins by name
extern std::unordered_map<std::string, BuiltinFunction> builtin_functions;

// Registration function (call at startup)
void register_builtins();
