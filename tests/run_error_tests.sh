#!/bin/bash

# Run error tests and verify they fail as expected
# This script tests that various syntax and type errors are properly caught

echo "Running error tests..."
echo "======================"

passed=0
failed=0

test_error() {
    local test_file="$1"
    local expected_error="$2"
    
    echo -n "Testing $test_file... "
    
    # Run the test and capture output
    output=$(LOG=debug ../enki compile "$test_file" 2>&1) || {
        # Test failed as expected, check if error message matches
        if echo "$output" | grep -q "$expected_error"; then
            echo "PASSED"
            passed=$((passed + 1))
        else
            echo "FAILED (unexpected error message)"
            echo "Expected: $expected_error"
            echo "Got: $output"
            failed=$((failed + 1))
        fi
        return
    }
    
    # Test succeeded when it should have failed
    echo "FAILED (should have failed but succeeded)"
    failed=$((failed + 1))
}

# Run all error tests
test_error "syntax_error_incomplete_expression.enki" "Expected right operand for binary operator"
test_error "syntax_error_missing_equals.enki" "Missing '=' in Let statement"
# test_error "syntax_error_missing_parens.enki" "Function not found"  # This is actually valid syntax
test_error "syntax_error_missing_curly_brace.enki" "Missing '}' in Function Definition"
test_error "syntax_error_missing_arrow.enki" "Missing '->' in Function Definition"
test_error "syntax_error_missing_colon.enki" "Missing ':' in Variable Declaration"
test_error "syntax_error_unknown_character.enki" "Unknown character"
test_error "syntax_error_unterminated_string.enki" "Unterminated string literal"
test_error "syntax_error_missing_condition.enki" "Expected condition expression"
test_error "syntax_error_missing_while_body.enki" "Expected body statement"
test_error "syntax_error_missing_import_path.enki" "Expected module path in Import statement"
test_error "syntax_error_missing_return_expression.enki" "Missing return expression in non-void function"
test_error "syntax_error_void_return_expression.enki" "Cannot return a value from a void function"
test_error "syntax_error_type_mismatch.enki" "Assignment type mismatch"
test_error "syntax_error_undefined_variable.enki" "Symbol not found"
test_error "syntax_error_undefined_function.enki" "Function not found"
test_error "syntax_error_non_boolean_condition.enki" "If condition must be boolean"
test_error "syntax_error_return_outside_function.enki" "Return statement outside of function"

echo ""
echo "======================"
echo "Error test results:"
echo "Passed: $passed"
echo "Failed: $failed"
echo "Total: $((passed + failed))"

if [ $failed -eq 0 ]; then
    echo "All error tests passed! ✅"
    exit 0
else
    echo "Some error tests failed! ❌"
    exit 1
fi 