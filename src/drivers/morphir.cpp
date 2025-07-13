#include "../compiler/lexer.hpp"
#include "../compiler/parser.hpp"
#include "../definitions/serializations.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

std::shared_ptr<Program> compile(const std::string &source,
                                 const std::string &filename) {
  std::vector<Token> tokens = lex(source, filename);
  return parse(tokens);
}


int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>\n";
    return 1;
  }

  std::ifstream file;
  std::ofstream output;
  std::string source;
  std::string filename;

  bool load_ast = false;

  std::string input_filename, output_filename, load_filename;
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "-i") {
      if (argc < i + 2) {
        std::cerr << "Usage: " << argv[0] << " -i <filename>\n";
        return 1;
      }
      input_filename = argv[i + 1];
      file.open(input_filename);
    }
    if (std::string(argv[i]) == "-o") {
      if (argc < i + 2) {
        std::cerr << "Usage: " << argv[0] << " -o <filename>\n";
        return 1;
      }
      output_filename = argv[i + 1];
    }
    if (std::string(argv[i]) == "-l") {
      if (argc < i + 2) {
        std::cerr << "Usage: " << argv[0] << " -l <filename>\n";
        return 1;
      }
      load_filename = argv[i + 1];
    }
  }
  // Check for filename conflicts
  if ((!output_filename.empty() && !load_filename.empty() &&
       output_filename == load_filename) ||
      (!input_filename.empty() && !output_filename.empty() &&
       input_filename == output_filename) ||
      (!input_filename.empty() && !load_filename.empty() &&
       input_filename == load_filename)) {
    std::cerr
        << "Error: Do not use the same file for both input, output, or load.\n";
    return 1;
  }
  if (!output_filename.empty())
    output.open(output_filename);
  if (!load_filename.empty())
    load_ast = true; // Set load_ast to true if -l is used

  if (load_ast) {
    // load the ast file
    std::ifstream ast_file(load_filename);
    nlohmann::json ast_json;
    ast_file >> ast_json;
    // program = ast_json.get<std::shared_ptr<Program>>(); // This line was
    // removed as per the new_code
    std::cout << "Loaded AST from " << load_filename << std::endl;
    return 0;
  }
  std::shared_ptr<Program> program;

  // If input file, lex, parse, and print to output file
  if (file.is_open()) {

    std::stringstream buffer;
    buffer << file.rdbuf();
    source = buffer.str();
    program = compile(source, input_filename);
  }

  if (output.is_open()) {
    nlohmann::json j = *program;
    output << j.dump(2) << std::endl;
  }

  return 0;
}
