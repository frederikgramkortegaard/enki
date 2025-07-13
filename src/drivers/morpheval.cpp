#include "../interpreter/eval.hpp"
#include "../definitions/serializations.hpp"
#include "../runtime/builtins.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {

  register_builtins();

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>\n";
    return 1;
  }

  // load the ast file
  std::ifstream ast_file(argv[1]);
  nlohmann::json ast_json;
  ast_file >> ast_json;
  std::cout << "Loaded AST from " << argv[1] << std::endl;

  auto program_ptr = ast_json.get<std::shared_ptr<Program>>();

  EvalContext ctx(*program_ptr);
  std::cout << "Program: " << program_ptr->statements.size() << std::endl;
  interpret(ctx);

  return 0;
}
