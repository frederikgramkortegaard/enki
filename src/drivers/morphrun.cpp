
#include "../compiler/lexer.hpp"
#include "../compiler/parser.hpp"
#include "../interpreter/eval.hpp"
#include "../runtime/builtins.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

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

  std::string filename = argv[1];
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Could not open file: " << filename << std::endl;
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
