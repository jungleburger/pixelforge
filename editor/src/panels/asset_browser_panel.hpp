#pragma once
#include "../editor_context.hpp"
#include <string>

namespace pf::editor {
class AssetBrowserPanel {
public:
    void draw(EditorContext& ctx);
private:
    std::string m_root_path{"assets/"};
};
} // namespace pf::editor
