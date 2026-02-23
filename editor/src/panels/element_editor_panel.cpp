#include "element_editor_panel.hpp"
#include <imgui.h>

namespace pf::editor {
void ElementEditorPanel::draw(EditorContext& /*ctx*/) {
    ImGui::Begin("Element Editor");
    ImGui::Text("Select an element to edit its properties.");
    ImGui::End();
}
} // namespace pf::editor
