#pragma once
#include "../definitions/ast.hpp"
#include "../definitions/tokens.hpp"
#include <string_view>
#include <vector>



struct ModuleContext {
  std::unordered_map<std::string, std::shared_ptr<Program>> modules;

  std::shared_ptr<Program> get_module(const std::string &name) {
    return modules[name];
  }

  void add_module(const std::string &name, std::shared_ptr<Program> program) {
    modules[name] = program;
  }


std::shared_ptr<Symbol> get_symbol_from_module(const std::string &module_name, const std::string &symbol_name) {
  auto module = get_module(module_name);
  if (module == nullptr) {
    return nullptr;
  }
  return module->symbols[symbol_name];
}


  std::shared_ptr<Program> load_module(const std::string &name) {


    if (modules.find(name) != modules.end()) {
      return modules[name];
    }

    // Load the module from the file
    std::ifstream file(name);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();

    // Parse the module
    auto tokens = tokenize(code);
    auto program = parse(tokens);

    // Add the module to the context
    modules[name] = program;

    return program;
  }
};
