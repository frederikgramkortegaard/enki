# Enki

Enki is a C++-based custom programming language and runtime designed for data and image manipulation, extensible computation, and robust AST handling. It features a modern type system, JSON serialization, and a flexible built-in function system.

## Features
- **Custom Language:** Supports variables, expressions, function calls, and let-statements.
- **Image Processing Builtins:** Includes `open`, `save`, `greyscale`, and more for BMP images.
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

To build:
```sh
make
```

This creates a single executable `enki` with the following subcommands:
- `enki compile` — Compiles `.enki` files and outputs JSON AST
- `enki run`     — Compiles and directly interprets `.enki` files
- `enki serde`   — Compile, then serialize/deserialize AST to/from JSON
- `enki eval`    — Loads JSON AST and executes it

To install to /usr/local/bin:
```sh
sudo make install
```

To clean build artifacts:
```sh
make clean
```

## Usage
### 1. Write an Enki Program
Example (`examples/test.enki`):
```enki
print("loadig image")
let image = open("sample-bmp-files-sample_640x426.bmp")
print(image)
print("saving and grayscaling as nested dangling expression")
let greyscale_image = greyscale(image)
print(greyscale_image)
save(greyscale_image, "greyscale.bmp")
```

### 2. Compile to JSON AST
```sh
./enki compile -o test.json examples/test.enki
```

### 3. Run the Interpreter
```sh
./enki eval test.json
```

### 4. Or do both in one step
```sh
./enki run examples/test.enki
```

## Language Features
- **let statements:** Variable binding
- **Control flow:** `if` and `else` statements
- **Function calls:** User and builtin functions
- **Literals:** Int, float, string
- **Identifiers:** Variable and function names
- **Expression statements:** Dangling expressions

## Built-in Functions
- `open(filename)` — Load a BMP image
- `save(image, filename)` — Save an image as BMP
- `greyscale(image)` — Convert image to greyscale
- `print(...)` — Print any value(s)
- *(plus stubs for invert, threshold, brightness, etc.)*

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
