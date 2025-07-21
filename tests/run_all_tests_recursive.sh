#!/bin/bash

# Test runner script for MorphIR compiler with recursive directory support
# Runs all tests in subdirectories and shows detailed debug output

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Compiler path
COMPILER="./enki"
TEST_DIR="./tests"
TEST_OUTPUT_DIR="./build/tests/"

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
FAILED_FILES=()  # Array to store failed test filenames

# Parse command line arguments
PAUSE_ON_ERROR=false
RUN_SPECIFIC_CATEGORY=""
while [[ $# -gt 0 ]]; do
    case $1 in
        --pause-on-error)
            PAUSE_ON_ERROR=true
            shift
            ;;
        --category)
            RUN_SPECIFIC_CATEGORY="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --pause-on-error    Pause and wait for input when a test fails"
            echo "  --category CAT      Run only tests in the specified category"
            echo "  -h, --help          Show this help message"
            echo ""
            echo "Available categories:"
            echo "  syntax, types, functions, control_flow, structs, enums, imports, externs, pointers, expressions"
            echo ""
            echo "Examples:"
            echo "  $0                    # Run all tests normally"
            echo "  $0 --category syntax  # Run only syntax tests"
            echo "  $0 --pause-on-error   # Pause on test failures"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}=== Enki Compiler Test Suite (Recursive Mode) ===${NC}"
echo "Running all tests with LOG=debug..."
if [ "$PAUSE_ON_ERROR" = true ]; then
    echo -e "${YELLOW}Pause on error: ENABLED${NC}"
fi
if [ -n "$RUN_SPECIFIC_CATEGORY" ]; then
    echo -e "${PURPLE}Running only category: $RUN_SPECIFIC_CATEGORY${NC}"
fi
echo ""

indent() {
    NUM_SPACES=$1
    sed "s/^/$(printf ' %.0s' $(seq 1 $NUM_SPACES))/"
}

# Function to run a test
run_test() {
    local test_file="$1"
    local test_name="$2"
    local expected_exit="$3"  # 0 for success, 1 for error
    local category="$4"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    TMP_OUT_FILE="$TEST_OUTPUT_DIR/${test_name}.ast.json"
    CMD="$COMPILER compile $test_file -o $TMP_OUT_FILE"

    set +e # Disable exit on error for this test
    OUTPUT=$($CMD 2>&1)
    EXIT_CODE=$?
    set -e # Re-enable exit on error

    # Check result
    if [ $EXIT_CODE -eq $expected_exit ]; then
        echo -e "${GREEN}✓ PASSED: [$category] $test_name${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}✗ FAILED: [$category] $test_name (expected exit $expected_exit, got $EXIT_CODE)${NC}"

        # Show test content
        echo -e "${RED}  - Test content: ($test_file)${NC}"
        cat "$test_file" | indent 6
        echo ""
        echo -e "${RED}  - Output:${NC}"
        echo "$OUTPUT"  | indent 6

        set +e # Disable exit on error for this block
        LOGS_FILE="$TEST_OUTPUT_DIR/${test_name}.log"
        LOG=debug $CMD 2>&1 | cat > "$LOGS_FILE"
        set -e # Re-enable exit on error

        echo -e "${RED}  - Logs saved to: $LOGS_FILE${NC}"

        FAILED_TESTS=$((FAILED_TESTS + 1))
        FAILED_FILES+=("$test_file")  # Add failed file to array

        # Pause on error if flag is set
        if [ "$PAUSE_ON_ERROR" = true ]; then
            echo ""
            echo -e "${YELLOW}Press Enter to continue to next test, or 'q' to quit...${NC}"
            read -r input
            if [ "$input" = "q" ]; then
                echo "Exiting due to user request..."
                exit 1
            fi
        fi
    fi
}

# Function to run tests in a specific directory
run_tests_in_directory() {
    local dir="$1"
    local category="$2"
    
    if [ ! -d "$dir" ]; then
        return
    fi
    
    echo -e "${CYAN}=== Running $category tests ===${NC}"
    
    # Success tests (should exit with 0)
    for test_file in "$dir"/*_success.enki; do
        if [ -f "$test_file" ]; then
            test_name=$(basename "$test_file" .enki)
            run_test "$test_file" "$test_name" 0 "$category"
        fi
    done

    # Error tests (should exit with 1)
    for test_file in "$dir"/*_error.enki; do
        if [ -f "$test_file" ]; then
            test_name=$(basename "$test_file" .enki)
            run_test "$test_file" "$test_name" 1 "$category"
        fi
    done

    # Syntax error tests (should exit with 1)
    for test_file in "$dir"/syntax_error_*.enki; do
        if [ -f "$test_file" ]; then
            test_name=$(basename "$test_file" .enki)
            run_test "$test_file" "$test_name" 1 "$category"
        fi
    done

    # Other .enki files that don't match the pattern
    for test_file in "$dir"/*.enki; do
        if [ -f "$test_file" ]; then
            # Skip files we already processed
            if [[ "$test_file" != *"_success.enki" && "$test_file" != *"_error.enki" && "$test_file" != *"syntax_error_"* ]]; then
                test_name=$(basename "$test_file" .enki)
                # Assume success for files without explicit suffix
                run_test "$test_file" "$test_name" 0 "$category"
            fi
        fi
    done
    
    echo ""
}

# Function to run all tests recursively
run_all_tests_recursive() {
    echo -e "${BLUE}Running tests from: $TEST_DIR${NC}"
    echo ""

    # Define test categories
    categories=("syntax" "types" "functions" "control_flow" "structs" "enums" "imports" "externs" "pointers" "expressions")
    
    if [ -n "$RUN_SPECIFIC_CATEGORY" ]; then
        # Run only the specified category
        if [[ " ${categories[*]} " =~ " ${RUN_SPECIFIC_CATEGORY} " ]]; then
            run_tests_in_directory "$TEST_DIR/$RUN_SPECIFIC_CATEGORY" "$RUN_SPECIFIC_CATEGORY"
        else
            echo -e "${RED}Error: Unknown category '$RUN_SPECIFIC_CATEGORY'${NC}"
            echo "Available categories: ${categories[*]}"
            exit 1
        fi
    else
        # Run all categories
        for category in "${categories[@]}"; do
            run_tests_in_directory "$TEST_DIR/$category" "$category"
        done
    fi
}

# Re-build the compiler
make
mkdir -p "$TEST_OUTPUT_DIR"

# Check if compiler exists
if [ ! -f "$COMPILER" ]; then
    echo -e "${RED}Error: Compiler not found at $COMPILER${NC}"
    echo "Please run 'make' from the project root first."
    exit 1
fi

# Run tests recursively
run_all_tests_recursive

# Summary
echo -e "${BLUE}=== Test Summary ===${NC}"
echo -e "Total tests: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
echo -e "${RED}Failed: $FAILED_TESTS${NC}"

# Print failed test filenames if any
if [ ${#FAILED_FILES[@]} -gt 0 ]; then
    echo ""
    echo -e "${RED}Failed test files:${NC}"
    for failed_file in "${FAILED_FILES[@]}"; do
        echo -e "${RED}  - $(basename "$failed_file")${NC}"
    done
    echo ""
fi

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}All tests passed! 🎉${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed! 😞${NC}"
    exit 1
fi 