#pragma once
#include <pixelforge/world/world.hpp>
#include <random>
#include <vector>

namespace pf {

/// Drives Phase-3 simulation: temperature propagation and element reactions.
///
/// Call once per fixed timestep, *after* ParticleSystem::tick():
///   reactions.tick(FIXED_DT);
///
/// Reactions handled:
///   - Melting   — temperature ≥ melting_point  → spawn melt_into as DynamicPixel
///   - Boiling   — temperature ≥ boiling_point  → spawn boil_into as DynamicPixel
///   - Ignition  — temperature ≥ ignition_point — probabilistic, controlled by flammability
///                 → spawn fire above pixel; replace with ash_into (or remove)
///
/// Heat sources:
///   - Settled fire/lava pixels radiate heat_output °C/s to neighbours
///   - apply_dynamic_heat() propagates heat from airborne fire DynamicPixels
///   - apply_heat() lets editor tools inject heat directly
class ReactionSystem {
public:
    explicit ReactionSystem(World& world);

    /// Run one fixed-timestep reaction update (temperature propagation + reactions).
    void tick(float dt);

    /// Propagate heat from airborne dynamic pixels (fire, lava) to nearby settled pixels.
    /// Call this each frame alongside tick().
    void apply_dynamic_heat(float dt);

    /// Inject heat at a specific world position (e.g. from the Heat Brush editor tool).
    void apply_heat(int wx, int wy, float delta_temp);

    /// World background temperature (°C). All pixels cool toward this passively.
    float ambient_temperature{20.f};

    /// Global multiplier applied to every element's thermal_conductivity.
    float conduction_scale{1.f};

private:
    struct PendingChange {
        int       wx, wy;
        ElementID new_element;   ///< INVALID_ELEMENT_ID = remove pixel only
        float     spawn_vel_x{0.f};
        float     spawn_vel_y{-50.f};
    };

    World&         m_world;
    std::mt19937   m_rng;

    void propagate_temperature(float dt);
    void evaluate_reactions(float dt, std::vector<PendingChange>& out);
    void apply_changes(std::vector<PendingChange>& changes);

    [[nodiscard]] ElementID resolve_tag(std::string_view tag) const;
};

} // namespace pf
