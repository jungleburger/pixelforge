#pragma once
#include "../editor_context.hpp"

namespace pf::editor {

class ViewportPanel {
public:
    void draw(EditorContext& ctx);
private:
    float m_zoom{1.f};
    float m_pan_x{0.f}, m_pan_y{0.f};
};

} // namespace pf::editor
