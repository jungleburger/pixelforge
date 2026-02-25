#pragma once
#include "../editor_context.hpp"
#include <pixelforge/core/logger.hpp>
#include <deque>
#include <string>
#include <array>

namespace pf::editor {

// One log entry stored in the console.
struct LogEntry {
    pf::LogLevel level{pf::LogLevel::Info};
    std::string  timestamp;   // "HH:MM:SS"
    std::string  text;
};

class ConsolePanel {
public:
    ConsolePanel();

    void draw(EditorContext& ctx);

    // Level-aware push — called by the logger sink registered in EditorApp.
    void push(pf::LogLevel level, std::string msg);

    // Backwards-compatible plain-text push (defaults to Info).
    void push(std::string msg);

private:
    static constexpr size_t MAX_LINES = 1024;

    std::deque<LogEntry>  m_entries;
    bool                  m_auto_scroll{true};
    bool                  m_scroll_to_bottom{false};

    // Per-level filter: indexed by static_cast<int>(LogLevel)
    std::array<bool, 6>   m_level_filter;

    // Text filter
    char                  m_text_filter[256]{};

    // Lua REPL command input
    char                  m_cmd_buf[512]{};

    static const char*  level_tag(pf::LogLevel level) noexcept;
    static std::string  current_timestamp();

    // Copies all visible lines to the OS clipboard.
    void copy_to_clipboard() const;
};

} // namespace pf::editor
