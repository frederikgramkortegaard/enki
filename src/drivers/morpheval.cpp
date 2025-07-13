#include "../interpreter/eval.hpp"
#include "../definitions/serializations.hpp"
#include "../runtime/builtins.hpp"
#include "../utils/logging.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <spdlog/spdlog.h>
#include <vector>

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
    nlohmann::json ast_json;
    file >> ast_json;
    spdlog::info("Loaded AST from {}", filename);

    auto program_ptr = ast_json.get<std::shared_ptr<Program>>();

    EvalContext ctx(*program_ptr);
    spdlog::debug("Program: {} statements", program_ptr->statements.size());
    interpret(ctx);
  }

  return 0;
}
