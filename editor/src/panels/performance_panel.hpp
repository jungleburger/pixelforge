#pragma once
#include "../editor_context.hpp"
#include <array>

namespace pf::editor {

class PerformancePanel {
public:
    void draw(EditorContext& ctx);
    void record_frame(float dt);

private:
    static constexpr int HISTORY = 120;

    float m_fps{0.f};
    float m_frame_ms{0.f};

    // Rolling history for plot
    std::array<float, HISTORY> m_fps_history{};
    int                        m_history_offset{0};

    // Refresh counters for pixel counts (updated every ~10 frames)
    int    m_count_tick{0};
    size_t m_settled_count{0};
    size_t m_dynamic_count{0};
};

} // namespace pf::editor
