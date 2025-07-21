# Enki Compiler Tests

This directory contains the test suite for the Enki programming language compiler. Tests are organized into categories based on the language features they test.

## Test Organization

Tests are organized into the following categories:

### 📁 `syntax/`
Tests for syntax errors and basic syntax validation:
- `syntax_error_*.enki` - Tests that should fail with syntax errors
- `test_*.enki` - Basic syntax tests

### 📁 `types/`
Tests for type checking and type-related features:
- `*_type_checking*.enki` - Type checking tests
- `char_*.enki` - Character literal and type tests

### 📁 `functions/`
Tests for function definitions and calls:
- `function_*.enki` - Function definition and call tests
- `nested_functions_*.enki` - Nested function tests

### 📁 `control_flow/`
Tests for control flow constructs:
- `control_flow_*.enki` - If/else, while loop tests
- `blocks_*.enki` - Block statement tests

### 📁 `structs/`
Tests for struct definitions and usage:
- `struct_*.enki` - Struct definition and field access tests

### 📁 `enums/`
Tests for enum definitions and usage:
- `enum_*.enki` - Enum definition and member access tests

### 📁 `imports/`
Tests for import statements:
- `import_*.enki` - Import statement tests

### 📁 `externs/`
Tests for extern function declarations:
- `extern_*.enki` - Extern function tests

### 📁 `pointers/`
Tests for pointer types and operations:
- `pointer_*.enki` - Pointer type tests
- `deref_*.enki` - Dereference operation tests

### 📁 `expressions/`
Tests for expressions and operators:
- `binary_ops_*.enki` - Binary operator tests

## Test Naming Convention

- `*_success.enki` - Tests that should compile successfully (exit code 0)
- `*_error.enki` - Tests that should fail with type/compilation errors (exit code 1)
- `syntax_error_*.enki` - Tests that should fail with syntax errors (exit code 1)
- Other `.enki` files - Assumed to be success tests

## Running Tests

### Run All Tests
```bash
./tests/run_all_tests_recursive.sh
```

### Run Tests by Category
```bash
./tests/run_all_tests_recursive.sh --category syntax
./tests/run_all_tests_recursive.sh --category functions
./tests/run_all_tests_recursive.sh --category structs
```

### Available Categories
- `syntax`
- `types`
- `functions`
- `control_flow`
- `structs`
- `enums`
- `imports`
- `externs`
- `pointers`
- `expressions`

### Run with Pause on Error
```bash
./tests/run_all_tests_recursive.sh --pause-on-error
```

### Legacy Test Runner
The original test runner is still available for backward compatibility:
```bash
./tests/run_all_tests_debug.sh
```

## Test Output

Tests generate output files in `./build/tests/`:
- `*.ast.json` - AST output for successful compilations
- `*.log` - Debug logs for failed tests

## Adding New Tests

1. **Choose the appropriate category** based on what the test covers
2. **Follow the naming convention**:
   - Use `_success.enki` suffix for tests that should pass
   - Use `_error.enki` suffix for tests that should fail
   - Use `syntax_error_` prefix for syntax error tests
3. **Place the test file** in the appropriate subdirectory
4. **Run the tests** to verify your new test works as expected

## Example Test Structure

```
tests/
├── syntax/
│   ├── syntax_error_missing_semicolon.enki
│   └── test_basic_syntax.enki
├── functions/
│   ├── function_call_success.enki
│   └── function_definition_error.enki
├── structs/
│   ├── struct_definition_success.enki
│   └── struct_field_access_error.enki
└── run_all_tests_recursive.sh
``` 