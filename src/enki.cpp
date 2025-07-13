
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "definitions/serializations.hpp"
#include "interpreter/eval.hpp"
#include "runtime/builtins.hpp"
#include "utils/logging.hpp"

std::shared_ptr<Program> compile(const std::string &source,
                               const std::string &filename) {
  std::vector<Token> tokens = lex(source, filename);
  return parse(tokens);
}

void print_global_usage(const char *prog_name) {
  fmt::println("Usage: {} <command> [options] [arguments]", prog_name);
  fmt::println("Commands:");
  fmt::println("  compile: Compile a enki source file to AST JSON");
  fmt::println("  eval: Evaluate a compiled AST JSON file");
  fmt::println("  run: Compile and run a enki source file");
  fmt::println("");
  fmt::println("Run '{} <command> -h' for more information on a specific command", prog_name);
}

void print_compile_usage(const char *prog_name) {
  fmt::println("Usage: {} compile [options] <input-file>", prog_name);
  fmt::println("Options:");
  fmt::println("  -o <file>: Output file for compiled AST");
  fmt::println("  -h: Show this help message");
}

void print_serde_usage(const char *prog_name) {
  fmt::println("Usage: {} serde [options] <input-file>", prog_name);
  fmt::println("Options:");
  fmt::println("  -h: Show this help message");
}

void print_eval_usage(const char *prog_name) {
  fmt::println("Usage: {} eval [options] <ast-file>", prog_name);
  fmt::println("Options:");
  fmt::println("  -h: Show this help message");
}

void print_run_usage(const char *prog_name) {
  fmt::println("Usage: {} run [options] <enki-file>", prog_name);
  fmt::println("Options:");
  fmt::println("  -h: Show this help message");
}

int compile_command(int argc, char *argv[]) {
  optind = 1; // Reset getopt
  std::string output_filename;
  int opt;
  while ((opt = getopt(argc, argv, "o:h")) != -1) {
    switch (opt) {
    case 'o':
      output_filename = optarg;
      break;
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

  // Check for filename conflicts
  if (!output_filename.empty() && output_filename == input_filename) {
    spdlog::error("Error: Do not use the same file for both input and output.");
    return 1;
  }

  std::shared_ptr<Program> program;

  // If input file, lex, parse, and print to output file
  std::ifstream file(input_filename);
  if (!file.is_open()) {
    spdlog::error("Could not open file: {}", input_filename);
    return 1;
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();
  program = compile(source, input_filename);

  if (output_filename.empty()) {
    output_filename = input_filename;
    output_filename = output_filename.substr(output_filename.find_last_of("/") + 1);
    size_t dot_pos = output_filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
      output_filename = output_filename.substr(0, dot_pos);
    }
    output_filename += ".ast.json";
    spdlog::info("No output file specified, using: {}", output_filename);
  }

  std::ofstream output(output_filename);
  if (!output.is_open()) {
    spdlog::error("Could not open output file: {}", output_filename);
    return 1;
  }
  nlohmann::json j = *program;
  output << j.dump(2) << std::endl;
  spdlog::info("Wrote AST to {}", output_filename);

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


  std::shared_ptr<Program> program;

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
    program = compile(source, input_filename);
  }

  std::string json_path = input_filename;
  json_path = json_path.substr(json_path.find_last_of("/") + 1);
  size_t dot_pos = json_path.find_last_of('.');
  if (dot_pos != std::string::npos) {
    json_path = json_path.substr(0, dot_pos);
  }
  json_path += ".ast.json";

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

  if (program->statements.size() != parsed_program->statements.size() ||
      program->symbols.size() != parsed_program->symbols.size()) {
    spdlog::error("AST mismatch after serialization/deserialization");
    spdlog::error("Original AST statements: {}", program->statements.size());
    spdlog::error("Parsed AST statements: {}", parsed_program->statements.size());
    spdlog::error("Original AST symbols: {}", program->symbols.size());
    spdlog::error("Parsed AST symbols: {}", parsed_program->symbols.size());
    return 1;
  }

  spdlog::info("AST serialization/deserialization successful");
  spdlog::info("Original AST: {} statements, {} symbols",
               program->statements.size(), program->symbols.size());
  spdlog::info("Parsed AST: {} statements, {} symbols",
               parsed_program->statements.size(), parsed_program->symbols.size());
  return 0;
}

int eval_command(int argc, char *argv[]) {
  optind = 1; // Reset getopt
  int opt;
  while ((opt = getopt(argc, argv, "h")) != -1) {
    switch (opt) {
    case 'h':
      print_eval_usage(argv[0]);
      return 0;
    default: /* '?' */
      print_eval_usage(argv[0]);
      return 1;
    }
  }

  if (optind >= argc) {
    spdlog::error("Input file required");
    print_eval_usage(argv[0]);
    return 1;
  }

  std::string filename = argv[optind];

  std::ifstream file(filename);
  if (!file.is_open()) {
    spdlog::error("Failed to open file: {}", filename);
    return 1;
  }
  nlohmann::json ast_json;
  file >> ast_json;
  spdlog::info("Loaded AST from {}", filename);

  auto program_ptr = ast_json.get<std::shared_ptr<Program>>();

  register_builtins();
  EvalContext ctx(*program_ptr);
  spdlog::debug("Program: {} statements", program_ptr->statements.size());
  interpret(ctx);

  return 0;
}

int run_command(int argc, char *argv[]) {
  optind = 1; // Reset getopt
  int opt;
  while ((opt = getopt(argc, argv, "h")) != -1) {
    switch (opt) {
    case 'h':
      print_run_usage(argv[0]);
      return 0;
    default: /* '?' */
      print_run_usage(argv[0]);
      return 1;
    }
  }

  if (optind >= argc) {
    spdlog::error("Input file required");
    print_run_usage(argv[0]);
    return 1;
  }

  std::string filename = argv[optind];

  std::ifstream file(filename);
  if (!file.is_open()) {
    spdlog::error("Could not open file: {}", filename);
    return 1;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();

  auto program = compile(source, filename);

  register_builtins();
  EvalContext ctx(*program);
  interpret(ctx);

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
  } else if (command == "eval") {
    return eval_command(argc, argv);
  } else if (command == "run") {
    return run_command(argc, argv);
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
