
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "compiler/codegen.hpp"
#include "compiler/injections.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/typecheck.hpp"
#include "definitions/serializations.hpp"
#include "utils/logging.hpp"

// External declaration of the global visualization flag
extern bool g_visualization_mode;

// Utility to print a string_view as hex for debugging
void print_string_view_hex(const std::string_view &sv, const char *label) {
  std::cout << label << ": length=" << sv.size() << ", data=";
  for (unsigned char c : sv) {
    if (std::isprint(c))
      std::cout << c;
    else
      std::cout << "\\x" << std::hex << std::setw(2) << std::setfill('0')
                << (int)c << std::dec;
  }
  std::cout << std::endl;
}

// Recursively print all string_view fields in the AST
void debug_print_ast_string_views(const Ref<Program> &program) {
  for (const auto &stmt : program->body->statements) {
    if (!stmt)
      continue;
    if (auto func = std::dynamic_pointer_cast<FunctionDefinition>(stmt)) {
      if (func->identifier)
        print_string_view_hex(func->identifier->name, "Function identifier");
      for (const auto &param : func->parameters) {
        if (param && param->identifier)
          print_string_view_hex(param->identifier->name,
                                "Parameter identifier");
      }
    }
    if (auto var = std::dynamic_pointer_cast<VarDecl>(stmt)) {
      if (var->identifier)
        print_string_view_hex(var->identifier->name, "VarDecl identifier");
    }
    if (auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
      if (auto ident =
              std::dynamic_pointer_cast<Identifier>(expr_stmt->expression)) {
        print_string_view_hex(ident->name, "ExprStmt identifier");
      }
    }
    if (auto import = std::dynamic_pointer_cast<Import>(stmt)) {
      if (import->module_path)
        print_string_view_hex(import->module_path->value, "Import module_path");
    }
  }
  // Print scope symbol names
  if (program->scope) {
    for (const auto &[name, symbol] : program->scope->symbols) {
      if (symbol)
        print_string_view_hex(symbol->name, "Scope symbol name");
    }
  }
}

Ref<Program> compile(const std::string &source, const std::string &filename,
                     Ref<ModuleContext> module_context) {
  auto buffer_ptr = std::make_shared<std::string>(source);
  std::vector<Token> tokens = lex(*buffer_ptr, filename);
  auto program = parse(tokens, buffer_ptr, module_context);
  perform_injections(program); // New injection pass
  typecheck(program);
  return program;
}

void print_global_usage(const char *prog_name) {
  fmt::println("Usage: {} <command> [options] [arguments]", prog_name);
  fmt::println("Commands:");
  fmt::println("  compile: Compile a enki source file to AST JSON");
  fmt::println("  serde: Test AST serialization/deserialization");
  fmt::println("");
  fmt::println(
      "Run '{} <command> -h' for more information on a specific command",
      prog_name);
}

void print_compile_usage(const char *prog_name) {
  fmt::println("Usage: {} compile [options] <input-file>", prog_name);
  fmt::println("Options:");
  fmt::println("  -o <file>: Output file for compiled AST");
  fmt::println("  -a: Output AST as JSON");
  fmt::println(
      "  --vis: Output minimal AST for visualization (no spans/locations)");
  fmt::println("  -h: Show this help message");
}

void print_serde_usage(const char *prog_name) {
  fmt::println("Usage: {} serde [options] <input-file>", prog_name);
  fmt::println("Options:");
  fmt::println("  -h: Show this help message");
}

std::string default_output_path(const std::string &input_filename,
                                std::string_view extension) {
  constexpr std::string_view build_dir = "./build/";
  std::filesystem::create_directories(build_dir);

  std::string output_filename = input_filename;
  output_filename =
      output_filename.substr(output_filename.find_last_of("/") + 1);
  size_t dot_pos = output_filename.find_last_of('.');
  if (dot_pos != std::string::npos) {
    output_filename = output_filename.substr(0, dot_pos);
  }
  output_filename += extension;
  return build_dir.data() + output_filename;
}

int compile_command(int argc, char *argv[]) {
  optind = 1; // Reset getopt
  std::string output_filename;
  bool visualization_mode = false;
  int opt;

  // Handle --vis flag first
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "--vis") {
      visualization_mode = true;
      // Remove the flag from argv
      for (int j = i; j < argc - 1; j++) {
        argv[j] = argv[j + 1];
      }
      argc--;
      i--; // Adjust index since we removed an element
      break;
    }
  }

  // Reset optind after modifying argc/argv
  optind = 1;
  bool output_ast_json = false;

  while ((opt = getopt(argc, argv, "o:ha")) != -1) {
    switch (opt) {
    case 'o':
      output_filename = optarg;
      break;
    case 'h':
      print_compile_usage(argv[0]);
      return 0;
    case 'a':
      output_ast_json = true;
      break;
    default: /* '?' */
      print_compile_usage(argv[0]);
      return 1;
    }
  }

  std::string input_filename;
  if (optind < argc) {
    input_filename = argv[optind];
  }

  if (input_filename.empty()) {
    spdlog::error("An input file is required.");
    print_compile_usage(argv[0]);
    return 1;
  }

  // Check for filename conflicts
  if (!output_filename.empty() && output_filename == input_filename) {
    spdlog::error("Error: Do not use the same file for both input and output.");
    return 1;
  }

  Ref<Program> program;
  Ref<ModuleContext> module_context = std::make_shared<ModuleContext>();

  // If input file, lex, parse, and print to output file
  std::ifstream file(input_filename);
  if (!file.is_open()) {
    spdlog::error("Could not open file: {}", input_filename);
    return 1;
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();
  program = compile(source, input_filename, module_context);

  if (output_filename.empty()) {
    output_filename =
        default_output_path(input_filename, output_ast_json ? ".ast.json" : "");
    spdlog::info("No output file specified, using: {}", output_filename);
  }

  // Set visualization mode if requested
  g_visualization_mode = visualization_mode;

  if (output_ast_json) {
    std::ofstream output(output_filename);
    if (!output.is_open()) {
      spdlog::error("Could not open output file: {}", output_filename);
      return 1;
    }
    debug_print_ast_string_views(program);
    nlohmann::json j = *program;
    output << j.dump(2) << std::endl;
    spdlog::info("Wrote AST to {}", output_filename);
  } else {
    auto temp_cpp_file = output_filename + ".cpp";
    std::ofstream cpp_output(temp_cpp_file);
    if (!cpp_output.is_open()) {
      spdlog::error("Could not open temporary C++ output file: {}",
                    temp_cpp_file);
      return 1;
    }
    cpp_output << codegen(program);
    cpp_output.close();
    spdlog::info("Wrote CPP code to {}", temp_cpp_file);

    // compile the generated C++ code
    std::string compile_cmd = "g++ -std=c++17 -o " +
                             output_filename + " " + temp_cpp_file;
    spdlog::info("Compiling generated C++ code with command: {}", compile_cmd);
    int compile_result = system(compile_cmd.c_str());
    if (compile_result != 0) {
      spdlog::error("Failed to compile generated C++ code");
      return 1;
    }
  }

  return 0;
}

int serde_command(int argc, char *argv[]) {
  optind = 1; // Reset getopt
  int opt;
  while ((opt = getopt(argc, argv, "h")) != -1) {
    switch (opt) {
    case 'h':
      print_compile_usage(argv[0]);
      return 0;
    default: /* '?' */
      print_compile_usage(argv[0]);
      return 1;
    }
  }

  std::string input_filename;
  if (optind < argc) {
    input_filename = argv[optind];
  }

  if (input_filename.empty()) {
    spdlog::error("An input file is required.");
    print_compile_usage(argv[0]);
    return 1;
  }

  Ref<Program> program;
  Ref<ModuleContext> module_context = std::make_shared<ModuleContext>();

  {
    // If input file, lex, parse, and print to output file
    std::ifstream file(input_filename);
    if (!file.is_open()) {
      spdlog::error("Could not open file: {}", input_filename);
      return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    program = compile(source, input_filename, module_context);
  }

  std::string json_path = default_output_path(input_filename, ".ast.json");
  spdlog::info("Using JSON path: {}", json_path);
  {
    std::ofstream output(json_path);
    if (!output.is_open()) {
      spdlog::error("Could not open output file: {}", json_path);
      return 1;
    }
    nlohmann::json j = *program;
    output << j.dump(2) << std::endl;
    spdlog::info("Wrote AST to {}", json_path);
  }

  auto parsed_program = std::make_shared<Program>();
  {
    std::ifstream file(json_path);
    if (!file.is_open()) {
      spdlog::error("Failed to open file: {}", json_path);
      return 1;
    }
    nlohmann::json ast_json;
    file >> ast_json;
    spdlog::info("Loaded AST from temporary file: {}", json_path);
    from_json(ast_json, *parsed_program);
  }

  if (program->body->statements.size() !=
      parsed_program->body->statements.size()) {
    spdlog::error("AST mismatch after serialization/deserialization");
    spdlog::error("Original AST statements: {}",
                  program->body->statements.size());
    spdlog::error("Parsed AST statements: {}",
                  parsed_program->body->statements.size());
    return 1;
  }

  spdlog::info("AST serialization/deserialization successful");
  return 0;
}

int main(int argc, char *argv[]) {
  logging::setup();

  if (argc < 2) {
    print_global_usage(argv[0]);
    return 1;
  }

  std::string command = argv[1];

  // Adjust argc and argv to exclude the command
  argc--;
  argv++;

  if (command == "compile") {
    return compile_command(argc, argv);
  } else if (command == "serde") {
    return serde_command(argc, argv);
  } else if (command == "-h" || command == "--help") {
    print_global_usage(argv[0]);
    return 0;
  } else {
    spdlog::error("Unknown command: {}", command);
    print_global_usage(argv[0]);
    return 1;
  }

  return 0;
}
