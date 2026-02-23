#pragma once
#include "../editor_context.hpp"

namespace pf::editor {
class WorldgenPanel {
public:
    void draw(EditorContext& ctx);
private:
    int  m_seed{42};
    int  m_width{512};
    int  m_height{256};
};
} // namespace pf::editor
