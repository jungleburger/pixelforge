#include <pixelforge/core/logger.hpp>
#include <iostream>
#include <chrono>
#include <format>
#include <vector>

namespace pf {

static const char* level_str(LogLevel l) noexcept {
    switch (l) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO ";
        case LogLevel::Warn:  return "WARN ";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
    }
    return "?????";
}

Logger::Logger() {
    // Default sink: stderr
    m_sinks.push_back([](LogLevel level, std::string_view msg) {
        std::cerr << std::format("[{}] {}\n", level_str(level), msg);
    });
}

Logger& Logger::instance() {
    static Logger s_instance;
    return s_instance;
}

void Logger::log(LogLevel level, std::string_view msg) {
    if (level < m_level) return;
    for (auto& sink : m_sinks) {
        sink(level, msg);
    }
}

} // namespace pf
