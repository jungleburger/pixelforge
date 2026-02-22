#include "element_palette_panel.hpp"
#include <imgui.h>

namespace pf::editor {
void ElementPalettePanel::draw(EditorContext& ctx) {
    ImGui::Begin("Element Palette");
    if (ctx.registry) {
        for (const auto& def : *ctx.registry) {
            const bool selected = (ctx.selected_element == def.id);
            if (ImGui::Selectable(def.name.c_str(), selected)) {
                ctx.selected_element = static_cast<int>(def.id);
            }
        }
    }
    ImGui::End();
}
} // namespace pf::editor
