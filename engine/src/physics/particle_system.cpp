#include <pixelforge/physics/particle_system.hpp>
#include <pixelforge/core/logger.hpp>
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
}

void ParticleSystem::integrate(DynamicPixel& px) const {
    // Apply gravity
    px.vel.y += GRAVITY * FIXED_DT;
    // Apply drag
    px.vel *= DRAG;
    // Integrate position
    px.pos += px.vel * FIXED_DT;
}

bool ParticleSystem::try_settle(DynamicPixel& px) {
    const int cx = static_cast<int>(std::round(px.pos.x));
    const int cy = static_cast<int>(std::round(px.pos.y));

    // Position occupied — nudge and stay dynamic
    if (m_world.has_settled(cx, cy)) {
        px.vel.x += rand_range(-SETTLE_NUDGE, SETTLE_NUDGE);
        px.vel.y += rand_range(-SETTLE_NUDGE, SETTLE_NUDGE);
        return false;
    }

    const int floor_y = m_world.config().height - 1;
    bool can_settle   = (cy >= floor_y);

    if (!can_settle) {
        // Check N/S/E/W neighbours
        const int dx[4] = { 0,  0, 1, -1};
        const int dy[4] = {-1,  1, 0,  0};
        for (int i = 0; i < 4 && !can_settle; ++i) {
            if (m_world.has_settled(cx + dx[i], cy + dy[i])) {
                can_settle = true;
            }
        }
    }

    if (can_settle) {
        // Snap position
        px.pos = {static_cast<float>(cx), static_cast<float>(cy)};

        SettledPixel sp;
        sp.id          = m_next_id++;
        sp.element     = px.base.element;
        sp.temperature = px.base.temperature;
        sp.hp          = px.base.hp;
        sp.color       = px.base.color;
        sp.world_pos   = {cx, cy};

        m_world.set_settled(cx, cy, sp);

        // Create bonds to neighbours
        auto* placed = m_world.get_settled(cx, cy);
        if (placed) {
            const int dx[4] = { 0,  0, 1, -1};
            const int dy[4] = {-1,  1, 0,  0};
            const Direction dirs[4] = {Direction::North, Direction::South,
                                        Direction::East,  Direction::West};
            for (int i = 0; i < 4; ++i) {
                auto* nb = m_world.get_settled(cx + dx[i], cy + dy[i]);
                if (nb && nb->id != 0 && placed->id != 0) {
                    m_world.bonds().create_bond(placed->id, nb->id, dirs[i]);
                }
            }
        }

        // Mark chunk dirty
        auto [chunkX, chunkY] = World::world_to_chunk(cx, cy);
        auto [localX, localY] = World::world_to_local(cx, cy);
        if (Chunk* chunk = m_world.get_chunk(chunkX, chunkY)) {
            chunk->mark_dirty(localX, localY);
        }

        px.awake = false;
        return true;
    }

    // Apply random impulse so pixel doesn't get stuck
    px.vel.x += rand_range(-SETTLE_NUDGE, SETTLE_NUDGE);
    px.vel.y += rand_range(-SETTLE_NUDGE, SETTLE_NUDGE);
    return false;
}

} // namespace pf
