#include "console_panel.hpp"
#include <imgui.h>
#include <chrono>
#include <format>
#include <sstream>

namespace pf::editor {

// ── File-local helpers ────────────────────────────────────────────────────────

static ImVec4 level_color(pf::LogLevel level) noexcept {
    switch (level) {
        case pf::LogLevel::Trace: return {0.55f, 0.55f, 0.55f, 1.f};
        case pf::LogLevel::Debug: return {0.70f, 0.85f, 1.00f, 1.f};
        case pf::LogLevel::Info:  return {0.90f, 0.90f, 0.90f, 1.f};
        case pf::LogLevel::Warn:  return {1.00f, 0.85f, 0.20f, 1.f};
        case pf::LogLevel::Error: return {1.00f, 0.40f, 0.35f, 1.f};
        case pf::LogLevel::Fatal: return {1.00f, 0.20f, 0.20f, 1.f};
    }
    return {1.f, 1.f, 1.f, 1.f};
}

// ── ConsolePanel ─────────────────────────────────────────────────────────────

ConsolePanel::ConsolePanel() {
    // All levels visible by default
    m_level_filter.fill(true);
}

/*static*/ const char* ConsolePanel::level_tag(pf::LogLevel level) noexcept {
    switch (level) {
        case pf::LogLevel::Trace: return "[TRACE]";
        case pf::LogLevel::Debug: return "[DEBUG]";
        case pf::LogLevel::Info:  return "[INFO ]";
        case pf::LogLevel::Warn:  return "[WARN ]";
        case pf::LogLevel::Error: return "[ERROR]";
        case pf::LogLevel::Fatal: return "[FATAL]";
    }
    return "[?????]";
}

/*static*/ std::string ConsolePanel::current_timestamp() {
    using namespace std::chrono;
    auto now      = system_clock::now();
    auto time_s   = system_clock::to_time_t(now);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &time_s);
#else
    localtime_r(&time_s, &tm_buf);
#endif
    return std::format("{:02d}:{:02d}:{:02d}",
                       tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
}

void ConsolePanel::push(pf::LogLevel level, std::string msg) {
    m_entries.push_back({level, current_timestamp(), std::move(msg)});
    if (m_entries.size() > MAX_LINES) m_entries.pop_front();
    m_scroll_to_bottom = m_auto_scroll;
}

void ConsolePanel::push(std::string msg) {
    push(pf::LogLevel::Info, std::move(msg));
}

void ConsolePanel::copy_to_clipboard() const {
    std::ostringstream oss;
    for (const auto& e : m_entries) {
        oss << e.timestamp << ' ' << level_tag(e.level) << ' ' << e.text << '\n';
    }
    ImGui::SetClipboardText(oss.str().c_str());
}

// ── draw() ────────────────────────────────────────────────────────────────────

void ConsolePanel::draw(EditorContext& ctx) {
    ImGui::Begin("Console");

    // ── Toolbar ───────────────────────────────────────────────────────────
    if (ImGui::Button("Clear")) {
        m_entries.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy All")) {
        copy_to_clipboard();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_auto_scroll);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.f);
    ImGui::InputText("Filter##text", m_text_filter, sizeof(m_text_filter));

    // ── Level filter ──────────────────────────────────────────────────────
    ImGui::Separator();
    const char* level_names[6] = {"Trace","Debug","Info","Warn","Error","Fatal"};
    for (int i = 0; i < 6; ++i) {
        if (i > 0) ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text,
            level_color(static_cast<pf::LogLevel>(i)));
        ImGui::Checkbox(level_names[i], &m_level_filter[i]);
        ImGui::PopStyleColor();
    }

    // ── Log area ──────────────────────────────────────────────────────────
    ImGui::Separator();
    const float log_height = ImGui::GetContentRegionAvail().y - 34.f;
    ImGui::BeginChild("##log_scroll", {0.f, log_height < 40.f ? 40.f : log_height},
                      false, ImGuiWindowFlags_HorizontalScrollbar);

    const std::string_view text_sv(m_text_filter);

    for (const auto& e : m_entries) {
        const int li = static_cast<int>(e.level);
        if (!m_level_filter[li]) continue;

        // Text filter (case-sensitive for now)
        if (!text_sv.empty() &&
            e.text.find(text_sv) == std::string::npos &&
            e.timestamp.find(text_sv) == std::string::npos)
            continue;

        ImGui::PushStyleColor(ImGuiCol_Text, level_color(e.level));
        ImGui::TextUnformatted(e.timestamp.c_str());
        ImGui::SameLine();
        ImGui::TextUnformatted(level_tag(e.level));
        ImGui::SameLine();
        ImGui::TextUnformatted(e.text.c_str());
        ImGui::PopStyleColor();
    }

    if (m_scroll_to_bottom) {
        ImGui::SetScrollHereY(1.0f);
        m_scroll_to_bottom = false;
    }
    ImGui::EndChild();

    // ── Lua REPL input ────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 48.f);

    const bool execute = ImGui::InputText("##cmd", m_cmd_buf, sizeof(m_cmd_buf),
        ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    const bool btn_run = ImGui::Button("Run");

    if ((execute || btn_run) && m_cmd_buf[0] != '\0') {
        const std::string cmd(m_cmd_buf);
        push(pf::LogLevel::Debug, std::format("> {}", cmd));
        if (ctx.exec_lua) {
            std::string result = ctx.exec_lua(cmd);
            if (!result.empty()) {
                push(pf::LogLevel::Info, std::format("  {}", result));
            }
        } else {
            push(pf::LogLevel::Warn, "  Lua REPL not available yet.");
        }
        m_cmd_buf[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

} // namespace pf::editor
