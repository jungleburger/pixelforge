#include "element_editor_panel.hpp"
#include <imgui.h>

namespace pf::editor {

void ElementEditorPanel::draw(EditorContext& ctx) {
    ImGui::Begin("Element Editor");

    if (!ctx.registry) {
        ImGui::TextDisabled("No registry.");
        ImGui::End();
        return;
    }

    // Element picker
    const int elem_count = static_cast<int>(ctx.registry->size());
    if (elem_count == 0) {
        ImGui::TextDisabled("No elements registered.");
        ImGui::End();
        return;
    }

    const auto* def = ctx.registry->get(
        static_cast<pf::ElementID>(ctx.selected_element));
    if (!def) {
        ImGui::TextDisabled("Invalid selection.");
        ImGui::End();
        return;
    }

    ImGui::Text("Viewing: %s  [id=%d]", def->name.c_str(), def->id);
    ImGui::SameLine();
    if (ImGui::ArrowButton("##prev", ImGuiDir_Left) && ctx.selected_element > 0)
        --ctx.selected_element;
    ImGui::SameLine();
    if (ImGui::ArrowButton("##next", ImGuiDir_Right) && ctx.selected_element < elem_count - 1)
        ++ctx.selected_element;

    ImGui::Separator();

    // ── Physics ───────────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Density:    %.3f",  def->density);
        ImGui::Text("Viscosity:  %.3f",  def->viscosity);
        ImGui::Text("Flamm:      %.2f",  def->flammability);
    }

    // ── Temperature & Phase Changes ───────────────────────────────────────────
    if (ImGui::CollapsingHeader("Temperature / Phase Changes", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextColored({0.9f, 0.6f, 0.2f, 1.f}, "Thermal conductivity: %.4f", def->thermal_conductivity);
        ImGui::TextColored({1.f,  0.4f, 0.1f, 1.f}, "Heat output:          %.1f \u00b0C/s", def->heat_output);

        ImGui::Spacing();

        if (def->melting_point >= 0.f)
            ImGui::Text("\u26a0 Melts   \u2265  %.0f \u00b0C  \u2192  \"%s\"",
                        def->melting_point, def->melt_into_tag.c_str());
        else
            ImGui::TextDisabled("  Melts   : never");

        if (def->boiling_point >= 0.f)
            ImGui::Text("\u2601 Boils   \u2265  %.0f \u00b0C  \u2192  \"%s\"",
                        def->boiling_point, def->boil_into_tag.c_str());
        else
            ImGui::TextDisabled("  Boils   : never");

        if (def->solidify_point >= 0.f)
            ImGui::Text("\u2744 Solidify \u2264  %.0f \u00b0C  \u2192  \"%s\"",
                        def->solidify_point, def->solidify_into_tag.c_str());
        else
            ImGui::TextDisabled("  Solidify: never");

        if (def->ignition_point >= 0.f)
            ImGui::Text("  Ignites >= %.0f \u00b0C  ash: \"%s\"",
                        def->ignition_point, def->ash_into_tag.c_str());
        else
            ImGui::TextDisabled("  Ignites : never");
    }

    // ── Contact Reactions ─────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Contact Reactions", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (def->reactions.empty()) {
            ImGui::TextDisabled("  (none)");
        } else {
            if (ImGui::BeginTable("##rxns", 4,
                    ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInner |
                    ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Target");
                ImGui::TableSetupColumn("Self \u2192");
                ImGui::TableSetupColumn("Other \u2192");
                ImGui::TableSetupColumn("Prob/s");
                ImGui::TableHeadersRow();

                for (const auto& r : def->reactions) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(r.target_tag.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(
                        r.self_into_tag.empty() ? "—" : r.self_into_tag.c_str());
                    ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(
                        r.other_into_tag.empty() ? "(remove)" : r.other_into_tag.c_str());
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%.2f", r.probability);
                }
                ImGui::EndTable();
            }
        }
    }

    ImGui::End();
}

} // namespace pf::editor
