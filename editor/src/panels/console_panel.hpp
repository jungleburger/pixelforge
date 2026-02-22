#pragma once
#include "../editor_context.hpp"
#include <deque>
#include <string>

namespace pf::editor {
class ConsolePanel {
public:
    void draw(EditorContext& ctx);
    void push(std::string msg);
private:
    std::deque<std::string> m_lines;
    static constexpr size_t MAX_LINES = 512;
};
} // namespace pf::editor
