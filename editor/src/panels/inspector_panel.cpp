#include "inspector_panel.hpp"
#include <imgui.h>

namespace pf::editor {

void InspectorPanel::draw(EditorContext& ctx) {
    ImGui::Begin("Inspector");

    if (!ctx.registry) {
        ImGui::TextDisabled("No registry loaded.");
        ImGui::End();
        return;
    }

    const auto* def = ctx.registry->get(
        static_cast<pf::ElementID>(ctx.selected_element));

    if (!def) {
        ImGui::TextDisabled("No element selected.");
        ImGui::End();
        return;
    }

    // ── Identity ─────────────────────────────────────────────────────────────
    ImGui::SeparatorText("Identity");
    ImGui::Text("Name:    %s", def->name.c_str());
    ImGui::Text("Tag:     %s", def->tag.c_str());
    ImGui::Text("Density: %.3f", def->density);

    // ── Physics properties ─────────────────────────────────────────────────
    ImGui::SeparatorText("Physics");
    {
        // Cast away const so the slider can mutate the live ElementDef.
        // Changes take effect immediately; Lua files are the persistent source.
        auto* mut = const_cast<pf::ElementDef*>(def);
        ImGui::SliderFloat("Restitution", &mut->restitution, 0.f, 1.f, "%.2f");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Bounciness: 0 = dead stop, 1 = perfect bounce");
    }

    // ── Thermal properties ────────────────────────────────────────────────────
    ImGui::SeparatorText("Thermal (Phase 4)");
    ImGui::Text("Conductivity:  %.3f /s", def->thermal_conductivity);
    ImGui::Text("Heat output:   %.1f \u00b0C/s", def->heat_output);

    if (def->melting_point >= 0.f)
        ImGui::Text("Melting point: %.1f \u00b0C  \u2192  %s",
                    def->melting_point, def->melt_into_tag.c_str());
    else
        ImGui::TextDisabled("Melting point: none");

    if (def->boiling_point >= 0.f)
        ImGui::Text("Boiling point: %.1f \u00b0C  \u2192  %s",
                    def->boiling_point, def->boil_into_tag.c_str());
    else
        ImGui::TextDisabled("Boiling point: none");

    if (def->solidify_point >= 0.f)
        ImGui::Text("Solidify:      \u2264 %.1f \u00b0C  \u2192  %s",
                    def->solidify_point, def->solidify_into_tag.c_str());
    else
        ImGui::TextDisabled("Solidify:      none");

    if (def->ignition_point >= 0.f)
        ImGui::Text("Ignition:      %.1f \u00b0C  (flam %.2f)",
                    def->ignition_point, def->flammability);
    else
        ImGui::TextDisabled("Ignition:      not flammable");

    // ── Contact reactions ─────────────────────────────────────────────────────
    if (!def->reactions.empty()) {
        ImGui::SeparatorText("Contact Reactions");
        for (const auto& r : def->reactions) {
            std::string self_s  = r.self_into_tag.empty()  ? "(unchanged)" : r.self_into_tag;
            std::string other_s = r.other_into_tag.empty() ? "(remove)"    : r.other_into_tag;
            ImGui::BulletText("+ %-12s  self\u2192%-12s  other\u2192%-12s  p=%.2f",
                              r.target_tag.c_str(),
                              self_s.c_str(), other_s.c_str(),
                              r.probability);
        }
    }

    // ── Heat brush ────────────────────────────────────────────────────────────
    ImGui::SeparatorText("Heat Brush");
    ImGui::Checkbox("Active", &ctx.heat_brush_active);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.f);
    ImGui::SliderFloat("Amount (°C)", &ctx.heat_brush_amount, -500.f, 2000.f, "%.0f");
    ImGui::TextDisabled("Click in the Viewport to apply heat.");

    ImGui::End();
}

} // namespace pf::editor
