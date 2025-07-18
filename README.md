# Enki

Enki is a C++-based custom programming language and runtime designed for data and image manipulation, extensible computation, and robust AST handling. It features a modern type system, JSON serialization, and a flexible built-in function system.

## Features
- **Custom Language:** Supports variables, expressions, function calls, and let-statements.
- **AST & Type System:** Strongly-typed AST with extensible node and value types.
- **Serialization:** Full JSON serialization/deserialization of ASTs using [nlohmann/json](https://github.com/nlohmann/json).
- **Interpreter:** Polyenkiic value system for extensibility and type safety.
- **Pretty Printing:** Utilities for AST and value visualization.

## Directory Structure
```
enkiIR/
├── src/
│   ├── enki.cpp        # Main entry point
│   ├── compiler/        # Lexer, parser
│   ├── definitions/     # AST, types, serialization
│   ├── interpreter/     # Interpreter logic and value system
│   ├── runtime/         # Builtin functions and runtime support
│   └── utils/           # Utilities (e.g., AST pretty printer)
├── examples/            # Example enkiIR programs
├── Makefile             # Build system
├── test.json            # Example serialized AST
```

## Build Instructions
Requirements:
- C++20 compiler (e.g., g++)
- [nlohmann/json](https://github.com/nlohmann/json)
- [magic_enum](https://github.com/Neargye/magic_enum) (header-only)
- [spdlog](https://github.com/gabime/spdlog)

On MacOS:
```
brew install spdlog nlohmann-json magic_enum
```

## Extensibility
- **Add new AST nodes:** Edit `ast.hpp` and update serializers/printers
- **Add new value types:** Subclass `ValueBase` in `eval.hpp`
- **Add new builtins:** Implement in `impls.cpp` and register in `register_builtins()`

## Contribution Guidelines
- Keep code modular and well-documented
- Update serializers and printers when adding new types
- Write example programs in `examples/`
- Use modern C++ best practices

## License
MIT (or specify your license here) 
