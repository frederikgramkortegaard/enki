# MorphIR

MorphIR is a C++-based interpreter and runtime for a custom programming language designed for data and image manipulation, extensible computation, and robust AST handling. It features a modern type system, JSON serialization, and a flexible built-in function system.

## Features
- **Custom Language:** Supports variables, expressions, function calls, and let-statements.
- **Image Processing Builtins:** Includes `open`, `save`, `greyscale`, and more for BMP images.
- **AST & Type System:** Strongly-typed AST with extensible node and value types.
- **Serialization:** Full JSON serialization/deserialization of ASTs using [nlohmann/json](https://github.com/nlohmann/json).
- **Interpreter:** Polymorphic value system for extensibility and type safety.
- **Pretty Printing:** Utilities for AST and value visualization.

## Directory Structure
```
MorphIR/
├── src/
│   ├── morph.cpp        # Main entry point
│   ├── compiler/        # Lexer, parser
│   ├── definitions/     # AST, types, serialization
│   ├── interpreter/     # Interpreter logic and value system
│   ├── runtime/         # Builtin functions and runtime support
│   └── utils/           # Utilities (e.g., AST pretty printer)
├── examples/            # Example MorphIR programs
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

This creates a single executable `morph` with the following subcommands:
- `morph compile` — Compiles `.morph` files and outputs JSON AST
- `morph run`     — Compiles and directly interprets `.morph` files
- `morph serde`   — Compile, then serialize/deserialize AST to/from JSON
- `morph eval`    — Loads JSON AST and executes it

To install to /usr/local/bin:
```sh
sudo make install
```

To clean build artifacts:
```sh
make clean
```

## Usage
### 1. Write a MorphIR Program
Example (`examples/test.morph`):
```morph
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
./morph compile -o test.json examples/test.morph
```

### 3. Run the Interpreter
```sh
./morph eval test.json
```

### 4. Or do both in one step
```sh
./morph run examples/test.morph
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