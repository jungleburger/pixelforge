#pragma once
#include "../editor_context.hpp"
#include <glm/vec2.hpp>
#include <optional>

namespace pf::editor {
class SelectTool {
public:
    void on_mouse_down(int wx, int wy, EditorContext& ctx);
    void on_mouse_drag(int wx, int wy, EditorContext& ctx);
    void on_mouse_up(int wx, int wy, EditorContext& ctx);

    [[nodiscard]] std::optional<pf::AABB> selection() const { return m_selection; }

private:
    bool              m_dragging{false};
    glm::ivec2        m_start{0, 0};
    std::optional<pf::AABB> m_selection;
};
} // namespace pf::editor
