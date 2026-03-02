#pragma once
#include <pixelforge/world/world.hpp>
#include <vector>

namespace pf {

constexpr float FIXED_DT      = 1.0f / 60.0f;
constexpr float GRAVITY       = 980.0f;   // px/s²
constexpr float DRAG          = 0.98f;    // velocity multiplier per frame
constexpr float SETTLE_NUDGE  = 5.0f;     // random impulse magnitude (px/s)
constexpr float SETTLE_SPEED  = 25.0f;    // px/s — below this, a supported particle anchors
constexpr float DEFAULT_RESTITUTION = 0.35f;  // default bounciness (0 = dead stop, 1 = perfect)
constexpr float ANGULAR_DRAG  = 0.96f;    // spin damping per frame
constexpr float TORQUE_SCALE  = 3.0f;     // rad/s per unit offset × impact speed

class ParticleSystem {
public:
    explicit ParticleSystem(World& world);

    // Advance by one fixed timestep (1/60 s).
    void tick();

    // Accumulate real time and run as many fixed ticks as needed.
    void update(float dt);

private:
    World& m_world;
    float  m_accumulator{0.f};
    PixelID m_next_id{1};

    void  integrate(DynamicPixel& px) const;
    [[nodiscard]] bool try_settle(DynamicPixel& px);
    [[nodiscard]] bool try_slide_diagonal(DynamicPixel& px, int cx, int cy);
    [[nodiscard]] bool try_spread_lateral(DynamicPixel& px, int cx, int cy);
    [[nodiscard]] bool settle_pixel(DynamicPixel& px, int cx, int cy);
    void  sweep_dead();
};

} // namespace pf
