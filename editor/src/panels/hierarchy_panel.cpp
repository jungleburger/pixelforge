#include "hierarchy_panel.hpp"
#include <pixelforge/world/chunk.hpp>
#include <imgui.h>
#include <format>
#include <cmath>

namespace pf::editor {

void HierarchyPanel::draw(EditorContext& ctx) {
    ImGui::Begin("Hierarchy");

    if (!ctx.world) {
        ImGui::TextDisabled("No world loaded.");
        ImGui::End();
        return;
    }

    const auto& cfg         = ctx.world->config();
    const size_t lat_cells  = ctx.world->lattice().size();
    const size_t num_chunks = ctx.world->chunk_count();

    // Compute chunk grid extents from world config
    const int chunks_x = static_cast<int>(std::ceil(
        static_cast<float>(cfg.width)  / pf::CHUNK_SIZE));
    const int chunks_y = static_cast<int>(std::ceil(
        static_cast<float>(cfg.height) / pf::CHUNK_SIZE));

    // ── World summary ─────────────────────────────────────────────────────────
    ImGui::SeparatorText("World");
    ImGui::Text("Size:       %d \u00d7 %d px", cfg.width, cfg.height);
    ImGui::Text("Seed:       %u",  cfg.seed);
    ImGui::Text("Chunk grid: %d \u00d7 %d  (%zu loaded)",
                chunks_x, chunks_y, num_chunks);
    ImGui::Text("Settled px: %zu", lat_cells);
    ImGui::Spacing();

    // ── Chunk tree ────────────────────────────────────────────────────────────
    ImGui::SeparatorText("Chunks");

    if (ImGui::BeginTable("##chunks", 5,
            ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_ScrollY,
            {0.f, std::min(static_cast<float>(chunks_x * chunks_y) * 20.f + 28.f, 280.f)})) {
        ImGui::TableSetupColumn("Chunk",    ImGuiTableColumnFlags_WidthFixed, 70.f);
        ImGui::TableSetupColumn("Settled",  ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Dynamic",  ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Sleeping", ImGuiTableColumnFlags_WidthFixed, 58.f);
        ImGui::TableSetupColumn("Dirty",    ImGuiTableColumnFlags_WidthFixed, 44.f);
        ImGui::TableHeadersRow();

        for (int cy = 0; cy < chunks_y; ++cy) {
            for (int cx = 0; cx < chunks_x; ++cx) {
                pf::Chunk* chunk = ctx.world->get_chunk(cx, cy);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("(%d,%d)", cx, cy);

                if (!chunk) {
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::TableSetColumnIndex(4);
                    ImGui::TableSetColumnIndex(0);
                    ImGui::SameLine();
                    ImGui::TextDisabled("  —not loaded—");
                    continue;
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", chunk->cells.size());

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%zu", chunk->dynamic_pixels.size());

                ImGui::TableSetColumnIndex(3);
                ImGui::TextColored(
                    chunk->sleeping ? ImVec4{0.5f, 0.8f, 0.5f, 1.f}
                                    : ImVec4{0.9f, 0.6f, 0.2f, 1.f},
                    chunk->sleeping ? "yes" : "no");

                ImGui::TableSetColumnIndex(4);
                ImGui::TextColored(
                    chunk->dirty ? ImVec4{1.f, 0.5f, 0.3f, 1.f}
                                 : ImVec4{0.5f, 0.5f, 0.5f, 1.f},
                    chunk->dirty ? "yes" : "no");
            }
        }
        ImGui::EndTable();
    }

    // ── Selection info ────────────────────────────────────────────────────────
    if (ctx.selection_box.has_value()) {
        ImGui::SeparatorText("Selection");
        const auto& b = *ctx.selection_box;
        ImGui::Text("Origin:  (%d, %d)", b.x, b.y);
        ImGui::Text("Size:    %d \u00d7 %d px", b.w, b.h);

        // Query settled pixels in selection
        auto hits = ctx.world->lattice().query_rect(b);
        ImGui::Text("Settled in region: %zu", hits.size());
    }

    ImGui::End();
}

} // namespace pf::editor

