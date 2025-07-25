#include "logging.hpp"
#include <cstdlib> // For getenv
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

void logging::setup() {
  const char *level = std::getenv("LOG");
  std::string log_level = (level != nullptr) ? level : "info";
  auto console = spdlog::stdout_color_mt("console");
  spdlog::set_default_logger(console);
  spdlog::set_level(spdlog::level::from_str(log_level));
}

std::string get_error_context(const std::string &source_buffer,
                              const Span &span, bool colorize = true) {
  if (source_buffer.empty()) {
    return ""; // No source buffer available
  }

  std::vector<std::string> lines;
  std::stringstream ss(source_buffer);
  std::string line;
  while (std::getline(ss, line)) {
    lines.push_back(line);
  }

  // Convert to 0-based indexing
  int line_index = span.start.row;

  // Check bounds
  if (line_index < 0 || line_index >= static_cast<int>(lines.size())) {
    return "";
  }

  std::stringstream context;

  int line_num_length = std::to_string(line_index + 1).length();

  // Show line above (if it exists)
  if (line_index > 0) {
    // Make sure to align the line number
    context << "  " << std::setw(line_num_length) << (line_index) << " | "
            << lines[line_index - 1] << std::endl;
  }

  // Show error line with line number
  context << "  " << std::setw(line_num_length) << (line_index + 1) << " | "
          << lines[line_index] << std::endl;

  // Show underline for the error span
  if (colorize) {
    context << "\033[31m"; // Set text color to red
  }
  context << std::string(line_num_length + 3, ' ') << "> ";
  context << std::string(span.start.col, ' ');

  for (int i = span.start.col;
       i < span.end.col && i < static_cast<int>(lines[line_index].length());
       i++) {
    context << "^";
  }
  if (colorize) {
    context << "\033[0m"; // Reset text color
  }
  context << std::endl;

  // Show line below (if it exists)
  if (line_index + 1 < static_cast<int>(lines.size())) {
    context << "  " << std::setw(line_num_length) << (line_index + 2) << " | "
            << lines[line_index + 1] << std::endl;
  }

  return context.str();
}

// Single error logging function with optional parameters
void log_error_exit(const std::string &message, const Span &span,
                    const std::string &source_buffer) {
  // Basic error message
  if (span.start.file_name.empty()) {
    std::cerr << "Error: " << message << std::endl;
  } else {
    // Full context with source code
    std::cerr << "Error at " << std::string(span.start.file_name) << ":"
              << (span.start.row + 1) << ":" << (span.start.col + 1) << ": "
              << message << std::endl;
    std::string context = get_error_context(source_buffer, span, /*colorize*/ isatty(STDERR_FILENO));
    if (!context.empty()) {
      std::cerr << context << std::endl;
    }
  }
  std::exit(1);
}
