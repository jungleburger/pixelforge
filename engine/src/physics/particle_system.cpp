#include <pixelforge/physics/particle_system.hpp>
#include <pixelforge/core/logger.hpp>
#include <algorithm>
#include <cmath>
#include <random>

namespace pf {

namespace {
    std::mt19937& rng() {
        static std::mt19937 s_rng{std::random_device{}()};
        return s_rng;
    }
    float rand_range(float lo, float hi) {
        return std::uniform_real_distribution<float>{lo, hi}(rng());
    }
}

ParticleSystem::ParticleSystem(World& world)
    : m_world(world)
{}

void ParticleSystem::update(float dt) {
    m_accumulator += dt;
    while (m_accumulator >= FIXED_DT) {
        tick();
        m_accumulator -= FIXED_DT;
    }
}

void ParticleSystem::tick() {
    auto dynamics = m_world.collect_all_dynamic();
    std::vector<DynamicPixel*> still_dynamic;

    for (DynamicPixel* px : dynamics) {
        if (!px->awake) continue;

        integrate(*px);
        px->age += FIXED_DT;

        // Remove expired particles
        if (px->lifetime >= 0.f && px->age >= px->lifetime) {
            px->pending_settle = false;
            px->awake = false;
            continue;
        }

        if (!try_settle(*px)) {
            still_dynamic.push_back(px);
        }
    }

    // Garbage-collect dead (settled / expired) dynamic pixels from chunks.
    // We do this once per tick to avoid unbounded memory growth.
    sweep_dead();
}

void ParticleSystem::sweep_dead() {
    m_world.sweep_dead_dynamics();
}

void ParticleSystem::integrate(DynamicPixel& px) const {
    // Apply gravity
    px.vel.y += GRAVITY * FIXED_DT;
    // Apply drag
    px.vel *= DRAG;
    // Apply angular drag
    px.angular_vel *= ANGULAR_DRAG;
    // Integrate position
    px.pos += px.vel * FIXED_DT;
    // Integrate angle
    px.angle += px.angular_vel * FIXED_DT;

    // Spin–linear coupling: angular velocity nudges the particle sideways.
    // Kept very small to avoid runaway lateral drift.
    px.vel.x += px.angular_vel * 0.02f;

    // ------------------------------------------------------------------
    // Boundary handling
    // X axis: infinite_x == true  → no walls.
    //         infinite_x == false → hard walls at 0 and width-1.
    // Y axis: ceiling at 0 (soft bounce), hard floor at floor_y.
    // ------------------------------------------------------------------
    const auto& cfg = m_world.config();
    const float floor = static_cast<float>(cfg.floor_y);

    if (!cfg.infinite_x) {
        const float max_x = static_cast<float>(cfg.width - 1);
        if (px.pos.x < 0.f)   { px.pos.x = 0.f;   px.vel.x =  std::abs(px.vel.x) * 0.3f; }
        if (px.pos.x > max_x) { px.pos.x = max_x;  px.vel.x = -std::abs(px.vel.x) * 0.3f; }
    }
    if (px.pos.y < 0.f)    { px.pos.y = 0.f;    px.vel.y =  std::abs(px.vel.y) * 0.3f; }
    if (px.pos.y > floor)  { px.pos.y = floor;   px.vel.y = 0.f; } // stop at floor

    // Lateral collision with settled pixels — if the particle moved
    // horizontally into a settled cell, bounce it back.
    int gx = static_cast<int>(std::round(px.pos.x));
    int gy = static_cast<int>(std::round(px.pos.y));

    // Clamp Y to floor; only clamp X when walls are enabled.
    gy = std::min(gy, cfg.floor_y);
    if (!cfg.infinite_x) {
        gx = std::clamp(gx, 0, cfg.width - 1);
    }

    if (m_world.has_settled(gx, gy)) {
        // Look up per-element restitution for bounce.
        const ElementDef* edef = m_world.registry().get(px.base.element);
        float restitution = edef ? edef->restitution : DEFAULT_RESTITUTION;
        // Reverse horizontal velocity (hit the side of a settled pixel).
        px.vel.x = -px.vel.x * restitution;
        // Apply torque from lateral impact — offset is the vertical sub-pixel position.
        float y_offset = px.pos.y - static_cast<float>(gy); // moment arm
        float impact   = std::abs(px.vel.x) + std::abs(px.vel.y);
        px.angular_vel += y_offset * impact * TORQUE_SCALE;
        // Push back so we don't embed.
        px.pos.x -= px.vel.x * FIXED_DT * 2.f;
    }
}

bool ParticleSystem::try_settle(DynamicPixel& px) {
    int cx = static_cast<int>(std::round(px.pos.x));
    int cy = static_cast<int>(std::round(px.pos.y));

    const auto& cfg = m_world.config();

    // Clamp Y to configured floor.  X is only clamped when walls are on.
    cy = std::min(cy, cfg.floor_y);
    if (!cfg.infinite_x)
        cx = std::clamp(cx, 0, cfg.width - 1);

    // Floor detection: the configured floor_y always acts as solid ground.
    auto is_floor = [&](int y) -> bool {
        return y >= cfg.floor_y;
    };

    constexpr float SETTLE_SPEED_SQ = SETTLE_SPEED * SETTLE_SPEED;
    // Per-element restitution (configurable in the editor).
    const ElementDef* edef = m_world.registry().get(px.base.element);
    const float restitution = edef ? edef->restitution : DEFAULT_RESTITUTION;

    // -----------------------------------------------------------------
    // 1. COLLISION — particle tunnelled into a settled cell
    //    (common at high velocity; integrate() can skip 10+ cells/tick)
    // -----------------------------------------------------------------
    if (m_world.has_settled(cx, cy)) {
        // Walk upward to find the surface (first free cell).
        int sy = cy - 1;
        while (sy >= 0 && m_world.has_settled(cx, sy))
            --sy;

        if (sy < 0) {
            // Column completely packed — forceful sideways ejection.
            px.pos.x += rand_range(-2.f, 2.f);
            px.vel.x += rand_range(-80.f, 80.f);
            px.vel.y = -60.f;
            return false;
        }

        // Snap to the surface cell.
        px.pos.y = static_cast<float>(sy);
        cy = sy;

        // Liquids absorb impact — no bounce, go straight to spread/settle.
        if (edef && edef->physics == PhysicsModel::Liquid) {
            px.vel = {0.f, 0.f};
            if (try_spread_lateral(px, cx, cy)) {
                cx = static_cast<int>(std::round(px.pos.x));
                cy = static_cast<int>(std::round(px.pos.y));
                return settle_pixel(px, cx, cy);
            }
            if (try_slide_diagonal(px, cx, cy))
                return false;
            cx = static_cast<int>(std::round(px.pos.x));
            cy = static_cast<int>(std::round(px.pos.y));
            return settle_pixel(px, cx, cy);
        }

        // Measure impact energy.
        float speed_sq = px.vel.x * px.vel.x + px.vel.y * px.vel.y;

        if (speed_sq <= SETTLE_SPEED_SQ) {
            // Try diagonal slide for non-liquid elements.
            if (try_slide_diagonal(px, cx, cy))
                return false;
            // Re-derive cell coords — diagonal may have repositioned px.
            cx = static_cast<int>(std::round(px.pos.x));
            cy = static_cast<int>(std::round(px.pos.y));
            return settle_pixel(px, cx, cy);
        }

        // High energy — bounce off the surface.
        // Reflect vertical velocity with restitution.
        float vy_impact = std::abs(px.vel.y);
        px.vel.y = -vy_impact * restitution;

        // Small lateral deflection based on sub-pixel offset from cell centre.
        // Scaled by vertical impact (not total speed) and by restitution.
        float offset = px.pos.x - static_cast<float>(cx);   // -0.5..+0.5
        float lateral = vy_impact * restitution * 0.15f;

        // Apply torque: moment arm = offset, force ∝ vertical impact.
        px.angular_vel += offset * vy_impact * TORQUE_SCALE;

        if (std::abs(offset) > 0.05f) {
            px.vel.x += (offset > 0.f ? lateral : -lateral);
        } else {
            // Dead centre — tiny random nudge so particles don't stack perfectly.
            px.vel.x += rand_range(-lateral * 0.2f, lateral * 0.2f);
        }
        // Friction: slightly damp existing horizontal velocity on impact.
        px.vel.x *= 0.85f;
        return false;
    }

    // -----------------------------------------------------------------
    // 2. FREE CELL — is there support below?
    // -----------------------------------------------------------------
    bool on_floor  = is_floor(cy);
    bool supported = on_floor || m_world.has_settled(cx, cy + 1);

    if (!supported) {
        // Nothing solid below — keep falling, gravity does the work.
        return false;
    }

    // -----------------------------------------------------------------
    // 3. SUPPORTED — liquids absorb impact; solids/powders bounce
    // -----------------------------------------------------------------
    float speed_sq = px.vel.x * px.vel.x + px.vel.y * px.vel.y;

    // Liquids never bounce — absorb all energy, spread or settle.
    if (edef && edef->physics == PhysicsModel::Liquid) {
        px.vel = {0.f, 0.f};
        if (try_spread_lateral(px, cx, cy)) {
            cx = static_cast<int>(std::round(px.pos.x));
            cy = static_cast<int>(std::round(px.pos.y));
            return settle_pixel(px, cx, cy);
        }
        if (try_slide_diagonal(px, cx, cy))
            return false;
        cx = static_cast<int>(std::round(px.pos.x));
        cy = static_cast<int>(std::round(px.pos.y));
        return settle_pixel(px, cx, cy);
    }

    if (speed_sq > SETTLE_SPEED_SQ) {
        // Reflect downward velocity (bounce off a horizontal surface).
        float vy_impact = std::abs(px.vel.y);
        if (px.vel.y > 0.f)
            px.vel.y = -vy_impact * restitution;

        // Small lateral deflection based on sub-pixel offset from the
        // support cell centre.  Scaled by vertical impact and restitution.
        float below_cx = static_cast<float>(cx);
        float offset   = px.pos.x - below_cx;     // -0.5..+0.5
        float lateral  = vy_impact * restitution * 0.15f;

        // Apply torque: moment arm = offset, force ∝ vertical impact.
        px.angular_vel += offset * vy_impact * TORQUE_SCALE;

        if (std::abs(offset) > 0.05f) {
            px.vel.x += (offset > 0.f ? lateral : -lateral);
        } else {
            // Dead centre — tiny random nudge.
            px.vel.x += rand_range(-lateral * 0.2f, lateral * 0.2f);
        }
        // Friction: damp horizontal velocity on impact.
        px.vel.x *= 0.85f;

        // Nudge above the surface so next tick's integrate() starts airborne.
        px.pos.y = static_cast<float>(cy) - 0.5f;
        return false;
    }

    // -----------------------------------------------------------------
    // 4. LOW ENERGY + SUPPORTED — slide / anchor (non-liquid)
    // -----------------------------------------------------------------
    // Solids / powders slide diagonally off peaks (angle of repose).
    if (try_slide_diagonal(px, cx, cy))
        return false;  // particle was nudged diagonally — keep it dynamic

    // Re-derive cell coords — diagonal may have repositioned px.
    cx = static_cast<int>(std::round(px.pos.x));
    cy = static_cast<int>(std::round(px.pos.y));
    return settle_pixel(px, cx, cy);
}

bool ParticleSystem::try_slide_diagonal(DynamicPixel& px, int cx, int cy) {
    // Any dynamic (dropped / spawned) particle can slide off a peak.
    // Gas / Fire / Energy elements are non-solid — skip them.
    const ElementDef* edef = m_world.registry().get(px.base.element);
    if (!edef) return false;
    if (edef->physics == PhysicsModel::Gas   ||
        edef->physics == PhysicsModel::Fire  ||
        edef->physics == PhysicsModel::Energy)
        return false;

    const auto& cfg = m_world.config();
    auto is_floor = [&](int y) -> bool { return y >= cfg.floor_y; };

    // If we’re on the world floor (not on another settled pixel), no slide.
    if (is_floor(cy)) return false;

    // Check the two diagonal cells below: (cx-1, cy+1) and (cx+1, cy+1).
    bool left_free  = !is_floor(cy + 1) &&
                      !m_world.has_settled(cx - 1, cy + 1) &&
                      !m_world.has_settled(cx - 1, cy);
    bool right_free = !is_floor(cy + 1) &&
                      !m_world.has_settled(cx + 1, cy + 1) &&
                      !m_world.has_settled(cx + 1, cy);

    // Honour X bounds when walls are enabled.
    if (!cfg.infinite_x) {
        if (cx - 1 < 0)            left_free  = false;
        if (cx + 1 >= cfg.width)   right_free = false;
    }

    if (!left_free && !right_free) return false;

    // Pick direction: prefer the side the particle is already leaning toward.
    int slide_x = cx;
    if (left_free && right_free) {
        // Use sub-pixel offset; break ties randomly.
        float offset = px.pos.x - static_cast<float>(cx);
        if (std::abs(offset) < 0.05f)
            slide_x = (rand_range(0.f, 1.f) < 0.5f) ? cx - 1 : cx + 1;
        else
            slide_x = offset < 0.f ? cx - 1 : cx + 1;
    } else {
        slide_x = left_free ? cx - 1 : cx + 1;
    }

    int dest_x = slide_x;
    int dest_y = cy + 1;

    // If the destination cell has support below it (settled pixel or floor),
    // settle there directly instead of bouncing around dynamically.
    bool dest_supported = is_floor(dest_y)
                       || m_world.has_settled(dest_x, dest_y + 1);

    if (dest_supported && !m_world.has_settled(dest_x, dest_y)) {
        // Reposition with zero velocity — caller will settle at new coords.
        px.pos.x = static_cast<float>(dest_x);
        px.pos.y = static_cast<float>(dest_y);
        px.vel   = {0.f, 0.f};
        return false;  // false → caller settles at the new position
    }

    // Destination has no support — nudge with minimal velocity so it
    // continues falling and settles once it reaches support.
    px.pos.x = static_cast<float>(dest_x);
    px.pos.y = static_cast<float>(cy) + 0.1f;
    px.vel.x = static_cast<float>(dest_x - cx) * 3.f;
    px.vel.y = 5.f;
    return true;
}

bool ParticleSystem::try_spread_lateral(DynamicPixel& px, int cx, int cy) {
    // Only liquid elements spread sideways.
    const ElementDef* edef = m_world.registry().get(px.base.element);
    if (!edef) return false;
    if (edef->physics != PhysicsModel::Liquid) return false;

    const auto& cfg = m_world.config();
    auto is_floor = [&](int y) -> bool { return y >= cfg.floor_y; };

    // Viscosity check: high viscosity → less likely to spread each tick.
    // viscosity 0.0 = always spread, 1.0 = never spread.
    if (edef->viscosity > 0.f && rand_range(0.f, 1.f) < edef->viscosity)
        return false;

    // Helper: does cell (x, cy) have ground beneath it?
    auto has_support = [&](int x) -> bool {
        return is_floor(cy) || is_floor(cy + 1) || m_world.has_settled(x, cy + 1);
    };

    // Scan LEFT: find how far the free, supported run extends.
    // Stop at walls, occupied cells, or unsupported gaps.
    int left_end = cx;   // furthest free cell to the left (inclusive)
    for (int x = cx - 1; ; --x) {
        if (!cfg.infinite_x && x < 0) break;
        if (m_world.has_settled(x, cy)) break;
        if (!has_support(x)) {
            // No support — this is a ledge / drop-off.  The diagonal
            // slide will handle falling.  Stop scanning here.
            break;
        }
        left_end = x;
    }

    // Scan RIGHT: same logic.
    int right_end = cx;
    for (int x = cx + 1; ; ++x) {
        if (!cfg.infinite_x && x >= cfg.width) break;
        if (m_world.has_settled(x, cy)) break;
        if (!has_support(x)) break;
        right_end = x;
    }

    int left_dist  = cx - left_end;   // ≥ 0
    int right_dist = right_end - cx;   // ≥ 0

    // If there's no free space on either side, nothing to do.
    if (left_dist == 0 && right_dist == 0) return false;

    // Pick direction: go toward the side with MORE open space so
    // liquid flows to equalise, filling gaps.  Break ties randomly.
    int dir = 0;  // -1 = left, +1 = right
    if (left_dist > 0 && right_dist > 0) {
        if (left_dist != right_dist)
            dir = (left_dist > right_dist) ? -1 : 1;
        else
            dir = (rand_range(0.f, 1.f) < 0.5f) ? -1 : 1;
    } else {
        dir = (left_dist > 0) ? -1 : 1;
    }

    // Move to the FURTHEST free cell in the chosen direction.
    // The caller will settle the particle at its new position.
    int max_dist = (dir < 0) ? left_dist : right_dist;
    int dest_x   = cx + dir * max_dist;

    // Reposition the particle with zero velocity.
    // Return false so the caller settles it immediately at the new spot.
    px.pos.x = static_cast<float>(dest_x);
    px.vel   = {0.f, 0.f};
    return true;  // signal that we repositioned (caller will settle)
}

bool ParticleSystem::settle_pixel(DynamicPixel& px, int cx, int cy) {
    // Cell must be free — another particle may have claimed it this tick.
    if (m_world.has_settled(cx, cy)) {
        // Give a small random kick so it finds a neighbouring free cell.
        // Keep velocity below SETTLE_SPEED so it doesn't bounce forever.
        px.vel.x += rand_range(-10.f, 10.f);
        px.vel.y = -10.f;
        return false;
    }

    px.pos = {static_cast<float>(cx), static_cast<float>(cy)};

    SettledPixel sp;
    sp.id          = m_next_id++;
    sp.element     = px.base.element;
    sp.temperature = px.base.temperature;
    sp.hp          = px.base.hp;
    sp.color       = px.base.color;
    sp.world_pos   = {cx, cy};

    m_world.set_settled(cx, cy, sp);

    // Bond to neighbours.
    auto* placed = m_world.get_settled(cx, cy);
    if (placed) {
        const int dx[4] = { 0,  0, 1, -1};
        const int dy[4] = {-1,  1, 0,  0};
        const Direction dirs[4] = {Direction::North, Direction::South,
                                    Direction::East,  Direction::West};
        for (int i = 0; i < 4; ++i) {
            auto* nb = m_world.get_settled(cx + dx[i], cy + dy[i]);
            if (nb && nb->id != 0 && placed->id != 0) {
                (void)m_world.bonds().create_bond(placed->id, nb->id, dirs[i]);
            }
        }
    }

    // Mark chunk dirty.
    auto [chunkX, chunkY] = World::world_to_chunk(cx, cy);
    auto [localX, localY] = World::world_to_local(cx, cy);
    if (Chunk* chunk = m_world.get_chunk(chunkX, chunkY)) {
        chunk->mark_dirty(localX, localY);
    }

    px.awake = false;
    return true;
}

} // namespace pf
