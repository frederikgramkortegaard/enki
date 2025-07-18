#pragma once

#include "../definitions/position.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace logging {
void setup();
}

// Helper function to get lines around an error span
std::string get_error_context(const std::string &source_buffer,
                              const Span &span);

// Single error logging function with optional parameters
void log_error_exit(const std::string &message,
                    const Span &span = Span{{0, 0, 0, ""}, {0, 0, 0, ""}},
                    const std::string &source_buffer = "");

// Convenience macro
#define LOG_ERROR_EXIT(...) log_error_exit(__VA_ARGS__)
