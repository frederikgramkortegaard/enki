#pragma once
#include "../definitions/ast.hpp"
#include "../definitions/tokens.hpp"
#include "../compiler/parser.hpp"
#include "../compiler/lexer.hpp"
#include "../utils/printer.hpp"
#include <string_view>
#include <vector>
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>
#include <filesystem>


std::shared_ptr<Program> parse(const std::vector<Token> &tokens);

struct ModuleContext {
  std::unordered_map<std::string, std::shared_ptr<Program>> modules;


  std::shared_ptr<Program> get_module(const std::string &name) {
    return modules[name];
  }


  std::shared_ptr<Program> add_module(const std::string &name, const std::string &importing_file = "") {
    // If already loaded, return it
    if (modules.find(name) != modules.end()) {
      return modules[name];
    }

    std::filesystem::path importee(name);
    // Only append .enki if not already present
    if (importee.extension() != ".enki") {
      importee += ".enki";
    }

    std::string resolved_path;
    if (!importing_file.empty()) {
      std::filesystem::path importer(importing_file);
      resolved_path = (importer.parent_path() / importee).string();
    } else {
      resolved_path = importee.string();
    }

    std::ifstream file(resolved_path);
    if (!file.is_open()) {
      spdlog::error("Failed to open file: {} (resolved from: {} in {})", resolved_path, name, importing_file);
      return nullptr;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();

    auto tokens = lex(code, resolved_path);
    auto program = parse(tokens);

    modules[name] = program;
    utils::ast::print_ast(*program, 0, 10);
    return program;
  }
};
