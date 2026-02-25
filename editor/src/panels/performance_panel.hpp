#pragma once
#include "../editor_context.hpp"
#include <array>
#include <limits>

namespace pf::editor {

// Per-subsystem frame timings (all in milliseconds).
struct SubsystemTimes {
    float physics_ms{0.f};
    float reactions_ms{0.f};
    float renderer_upload_ms{0.f};
    float imgui_ms{0.f};
};

class PerformancePanel {
public:
    void draw(EditorContext& ctx);

    // Called once per frame with the total delta time.
    void record_frame(float dt);

    // Called by EditorApp with per-subsystem wall-clock measurements.
    void record_subsystem_times(const SubsystemTimes& t);

private:
    static constexpr int HISTORY = 128;

    float m_fps{0.f};
    float m_frame_ms{0.f};
    float m_fps_min{std::numeric_limits<float>::max()};
    float m_fps_max{0.f};

    // Rolling history for FPS plot
    std::array<float, HISTORY> m_fps_history{};
    int                        m_history_offset{0};

    SubsystemTimes m_sys_times;

    // Refresh counters for pixel/bond counts (updated every 10 frames)
    int    m_count_tick{0};
    size_t m_settled_count{0};
    size_t m_dynamic_count{0};
    size_t m_bond_count{0};
    size_t m_awake_chunks{0};
};

} // namespace pf::editor
