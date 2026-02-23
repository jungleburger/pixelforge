#pragma once
#include "../editor_context.hpp"
#include <pixelforge/procgen/generator.hpp>

namespace pf::editor {

class WorldgenPanel {
public:
    void draw(EditorContext& ctx);

private:
    // Generator config (noise parameter tweaks)
    pf::GeneratorConfig m_gen_cfg{};

    // Last-generation status
    bool        m_generated{false};
    std::string m_status_msg;
};

} // namespace pf::editor
