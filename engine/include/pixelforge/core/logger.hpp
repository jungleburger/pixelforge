#pragma once
#include <string_view>
#include <string>
#include <format>
#include <functional>
#include <vector>
#include <cstdint>

namespace pf {

enum class LogLevel : uint8_t {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

class Logger {
public:
    using SinkFn = std::function<void(LogLevel, std::string_view)>;

    static Logger& instance();

    void set_level(LogLevel level) noexcept { m_level = level; }
    void add_sink(SinkFn sink) { m_sinks.push_back(std::move(sink)); }
    void clear_sinks() { m_sinks.clear(); }

    void log(LogLevel level, std::string_view msg);

    template<typename... Args>
    void trace(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Trace, std::format(fmt, std::forward<Args>(args)...));
    }
    template<typename... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Debug, std::format(fmt, std::forward<Args>(args)...));
    }
    template<typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Info, std::format(fmt, std::forward<Args>(args)...));
    }
    template<typename... Args>
    void warn(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Warn, std::format(fmt, std::forward<Args>(args)...));
    }
    template<typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Error, std::format(fmt, std::forward<Args>(args)...));
    }
    template<typename... Args>
    void fatal(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Fatal, std::format(fmt, std::forward<Args>(args)...));
    }

private:
    Logger();
    LogLevel m_level{LogLevel::Info};
    std::vector<SinkFn> m_sinks;
};

} // namespace pf

#define PF_LOG_TRACE(...) ::pf::Logger::instance().trace(__VA_ARGS__)
#define PF_LOG_DEBUG(...) ::pf::Logger::instance().debug(__VA_ARGS__)
#define PF_LOG_INFO(...)  ::pf::Logger::instance().info(__VA_ARGS__)
#define PF_LOG_WARN(...)  ::pf::Logger::instance().warn(__VA_ARGS__)
#define PF_LOG_ERROR(...) ::pf::Logger::instance().error(__VA_ARGS__)
#define PF_LOG_FATAL(...) ::pf::Logger::instance().fatal(__VA_ARGS__)
