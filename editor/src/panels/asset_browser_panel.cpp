#include "asset_browser_panel.hpp"
#include <imgui.h>

namespace pf::editor {
void AssetBrowserPanel::draw(EditorContext& /*ctx*/) {
    ImGui::Begin("Asset Browser");
    ImGui::Text("Root: %s", m_root_path.c_str());
    ImGui::End();
}
} // namespace pf::editor
