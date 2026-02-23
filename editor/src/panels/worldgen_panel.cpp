#include "worldgen_panel.hpp"
#include <pixelforge/procgen/biome.hpp>
#include <imgui.h>
#include <format>

namespace pf::editor {

static const char* noise_type_name(FastNoiseLite::NoiseType t) {
    switch (t) {
        case FastNoiseLite::NoiseType_OpenSimplex2:  return "OpenSimplex2";
        case FastNoiseLite::NoiseType_OpenSimplex2S: return "OpenSimplex2S";
        case FastNoiseLite::NoiseType_Cellular:      return "Cellular";
        case FastNoiseLite::NoiseType_Perlin:        return "Perlin";
        case FastNoiseLite::NoiseType_ValueCubic:    return "ValueCubic";
        case FastNoiseLite::NoiseType_Value:         return "Value";
        default:                                     return "Unknown";
    }
}

static void noise_params_widget(const char* label, pf::NoiseParams& np) {
    if (ImGui::CollapsingHeader(label)) {
        ImGui::PushID(label);
        ImGui::SetNextItemWidth(130.f);
        ImGui::Text("Type: %s", noise_type_name(np.noise_type));
        ImGui::SetNextItemWidth(130.f);
        ImGui::SliderFloat("Frequency",  &np.frequency,   0.001f, 0.1f,  "%.4f");
        ImGui::SetNextItemWidth(130.f);
        ImGui::SliderInt  ("Octaves",    &np.octaves,     1, 8);
        ImGui::SetNextItemWidth(130.f);
        ImGui::SliderFloat("Lacunarity", &np.lacunarity,  1.f,  4.f,   "%.2f");
        ImGui::SetNextItemWidth(130.f);
        ImGui::SliderFloat("Gain",       &np.gain,        0.1f, 1.f,   "%.2f");
        ImGui::PopID();
    }
}

void WorldgenPanel::draw(EditorContext& ctx) {
    ImGui::Begin("World Gen");

    if (!ctx.world) {
        ImGui::TextDisabled("No world loaded.");
        ImGui::End();
        return;
    }

    // ── Current world info (read-only) ─────────────────────────────────────
    ImGui::SeparatorText("World Config");
    const auto& cfg = ctx.world->config();
    ImGui::Text("Seed:         %u",     cfg.seed);
    ImGui::Text("Dimensions:   %d \u00d7 %d px", cfg.width, cfg.height);
    ImGui::Text("Cave density: %.2f",   cfg.cave_density);
    ImGui::Text("Water level:  %.2f",   cfg.water_level);
    ImGui::Text("Lava depth:   %.2f",   cfg.lava_depth);
    ImGui::TextDisabled(" (dimensions are fixed at world construction)");

    // ── Noise parameters ───────────────────────────────────────────────────
    ImGui::SeparatorText("Noise Parameters");
    noise_params_widget("Terrain Noise", m_gen_cfg.terrain_noise);
    noise_params_widget("Cave Noise",    m_gen_cfg.cave_noise);
    noise_params_widget("Ore Noise",     m_gen_cfg.ore_noise);

    // ── Biomes ────────────────────────────────────────────────────────────
    if (ctx.biomes) {
        ImGui::SeparatorText("Registered Biomes");
        ImGui::Text("Count: %zu", ctx.biomes->size());
        const float probe_step = 0.1f;
        for (float d = 0.f; d <= 1.f; d += probe_step) {
            const pf::BiomeDef* bdef = ctx.biomes->get_biome_at(d);
            if (bdef) {
                ImGui::BulletText("depth %.1f \u2013 %.1f  \u2192  %s",
                                  bdef->min_depth, bdef->max_depth,
                                  bdef->name.c_str());
            }
        }
    }

    // ── Action ──────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Separator();
    if (!ctx.biomes) {
        ImGui::TextColored({1.f, 0.4f, 0.4f, 1.f},
            "BiomeRegistry not available — cannot generate.");
    } else {
        if (ImGui::Button("Regenerate World", {160.f, 0.f})) {
            // Clear existing settled content
            ctx.world->lattice().clear();

            pf::WorldGenerator gen(*ctx.world, *ctx.biomes, m_gen_cfg);
            gen.generate();

            m_generated  = true;
            m_status_msg = std::format("Generated {} \u00d7 {} world (seed {})",
                                       cfg.width, cfg.height, cfg.seed);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Uses current seed");
    }

    if (m_generated && !m_status_msg.empty()) {
        ImGui::TextColored({0.4f, 1.f, 0.5f, 1.f}, "\u2714 %s", m_status_msg.c_str());
    }

    ImGui::End();
}

} // namespace pf::editor

