#pragma once
#include "../editor_context.hpp"

namespace pf::editor {
class PerformancePanel {
public:
    void draw(EditorContext& ctx);
    void record_frame(float dt);
private:
    float m_fps{0.f};
    float m_frame_ms{0.f};
};
} // namespace pf::editor
