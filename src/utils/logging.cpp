#include "logging.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <cstdlib> // For getenv

void logging::setup() {
  const char *level = std::getenv("LOG");
  std::string log_level = (level != nullptr) ? level : "info";
  auto console = spdlog::stdout_color_mt("console");
  spdlog::set_default_logger(console);
  spdlog::set_level(spdlog::level::from_str(log_level));
}
