#include "../compiler/lexer.hpp"
#include "../compiler/parser.hpp"
#include "../interpreter/eval.hpp"
#include "../runtime/builtins.hpp"
#include "../utils/logging.hpp"
#include <fstream>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>

std::shared_ptr<Program> compile(const std::string &source,
                                 const std::string &filename) {
  std::vector<Token> tokens = lex(source, filename);
  return parse(tokens);
}

int main(int argc, char *argv[]) {
  logging::setup();
  if (argc < 2) {
    spdlog::error("Usage: {} <filename>", argv[0]);
    return 1;
  }

  std::string filename;
  bool from_file = false;
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "-f") {
      if (argc < i + 2) {
        spdlog::error("Usage: {} -f <filename>", argv[0]);
        return 1;
      }
      filename = argv[i + 1];
      from_file = true;
      i++;
    }
  }

  if (from_file) {
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
  }

  return 0;
}
