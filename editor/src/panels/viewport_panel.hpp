#pragma once
#include "../editor_context.hpp"
#include "../tools/paint_tool.hpp"
#include "../tools/erase_tool.hpp"
#include "../tools/select_tool.hpp"

namespace pf::editor {

class ViewportPanel {
public:
    void draw(EditorContext& ctx);

private:
    // Camera
    float m_zoom{1.f};
    float m_pan_x{0.f}, m_pan_y{0.f};

    // Canvas screen-space origin (updated every frame)
    float m_canvas_x{0.f}, m_canvas_y{0.f};

    // Interaction tools (owned here; dispatched by active_tool in ctx)
    PaintTool  m_paint;
    EraseTool  m_erase;
    SelectTool m_select;

    // Convert screen-space mouse position → world integer coordinates
    void world_coords(float mx, float my, int& wx, int& wy) const noexcept;

    // Route a mouse event to the correct tool
    void dispatch_tool(float mx, float my, bool pressed, bool held,
                       bool released, EditorContext& ctx);
};

} // namespace pf::editor
