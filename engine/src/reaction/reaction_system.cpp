#include <pixelforge/reaction/reaction_system.hpp>
#include <pixelforge/pixel/dynamic_pixel.hpp>
#include <pixelforge/core/logger.hpp>
#include <algorithm>
#include <array>
#include <unordered_set>

namespace pf {

namespace {

// Cardinal neighbour offsets: N, S, E, W
constexpr std::array<glm::ivec2, 4> CARD = {{{0, -1}, {0, 1}, {1, 0}, {-1, 0}}};

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

ReactionSystem::ReactionSystem(World& world)
    : m_world(world)
    , m_rng(std::random_device{}())
{}

// ─────────────────────────────────────────────────────────────────────────────

void ReactionSystem::tick(float dt) {
    // Evaluate reactions on the *current* temperature snapshot before
    // propagation alters temperatures (important for elements with
    // phase-transition points at or near ambient, e.g. ice at 0 °C).
    std::vector<PendingChange> changes;
    changes.reserve(64);
    evaluate_reactions(dt, changes);
    evaluate_contact_reactions(dt, changes);
    apply_changes(changes);

    propagate_temperature(dt);
}

// ─────────────────────────────────────────────────────────────────────────────

void ReactionSystem::apply_heat(int wx, int wy, float delta_temp) {
    if (auto* px = m_world.get_settled(wx, wy)) {
        px->temperature = std::clamp(px->temperature + delta_temp, -273.f, 9999.f);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void ReactionSystem::propagate_temperature(float dt) {
    const auto& cfg  = m_world.config();
    auto&       lat  = m_world.lattice();
    auto&       reg  = m_world.registry();

    // Collect temperature deltas into a separate buffer so every pixel reads
    // "old" temperatures and we apply updates atomically.
    struct TDelta { int wx, wy; float d; };
    std::vector<TDelta> deltas;

    auto cells = lat.query_rect({0, 0, cfg.width, cfg.height});
    deltas.reserve(cells.size() * 5);

    for (auto& [pos, px] : cells) {
        if (!px || px->id == INVALID_PIXEL_ID) continue;

        const ElementDef* def = reg.get(px->element);
        if (!def) continue;

        const float T  = px->temperature;
        const float tc = def->thermal_conductivity * conduction_scale;

        // ── Conduction: exchange heat with each neighbour ──────────────────
        for (const auto& off : CARD) {
            const int nx = pos.x + off.x;
            const int ny = pos.y + off.y;

            float n_temp = ambient_temperature; // open air is at ambient
            if (const auto* npx = lat.get(nx, ny)) {
                n_temp = npx->temperature;
            }

            const float exchange = tc * (n_temp - T) * dt;
            deltas.push_back({pos.x, pos.y, exchange});
        }

        // ── Active heat emission (fire / lava etc.) ────────────────────────
        if (def->heat_output > 0.f) {
            const float emitted = def->heat_output * dt;

            // Self-heat (sustains the element's own temperature)
            deltas.push_back({pos.x, pos.y, emitted});

            // Radiate to solid neighbours — fire radiates more than lava
            const float spread = (def->physics == PhysicsModel::Fire) ? 0.5f : 0.3f;
            for (const auto& off : CARD) {
                const int nx = pos.x + off.x;
                const int ny = pos.y + off.y;
                if (lat.has(nx, ny)) {
                    deltas.push_back({nx, ny, emitted * spread});
                }
            }
        }

        // ── Passive cooling toward ambient ─────────────────────────────────
        // Gentle exponential decay: 2% of excess heat lost per second.
        const float excess = T - ambient_temperature;
        if (excess > 0.01f) {
            deltas.push_back({pos.x, pos.y, -0.02f * excess * dt});
        }
    }

    // Apply all deltas
    for (const auto& d : deltas) {
        if (auto* px = lat.get(d.wx, d.wy)) {
            px->temperature = std::clamp(px->temperature + d.d, -273.f, 9999.f);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void ReactionSystem::apply_dynamic_heat(float dt) {
    auto& lat = m_world.lattice();
    auto& reg = m_world.registry();

    for (DynamicPixel* dp : m_world.collect_all_dynamic()) {
        if (!dp) continue;
        const ElementDef* def = reg.get(dp->base.element);
        if (!def || def->heat_output <= 0.f) continue;

        const int ix = static_cast<int>(dp->pos.x);
        const int iy = static_cast<int>(dp->pos.y);
        const float heat = def->heat_output * dt * 0.25f; // dynamic pixels radiate less

        for (const auto& off : CARD) {
            if (auto* sp = lat.get(ix + off.x, iy + off.y)) {
                sp->temperature = std::clamp(sp->temperature + heat, -273.f, 9999.f);
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void ReactionSystem::evaluate_reactions(float dt, std::vector<PendingChange>& out) {
    const auto& cfg = m_world.config();
    auto&       lat = m_world.lattice();
    auto&       reg = m_world.registry();

    std::uniform_real_distribution<float> dist01(0.f, 1.f);

    for (auto& [pos, px] : lat.query_rect({0, 0, cfg.width, cfg.height})) {
        if (!px || px->id == INVALID_PIXEL_ID) continue;

        const ElementDef* def = reg.get(px->element);
        if (!def) continue;

        const float T = px->temperature;

        // ── Melting ────────────────────────────────────────────────────────
        // Use tag presence as guard so elements with melting_point == 0 (e.g.
        // ice) are handled correctly — the old `> 0.f` guard excluded them.
        if (!def->melt_into_tag.empty() && T >= def->melting_point) {
            ElementID into = resolve_tag(def->melt_into_tag);
            if (into == INVALID_ELEMENT_ID) into = def->melt_into;
            out.push_back({pos.x, pos.y, into, 0.f, -20.f});
            continue;
        }

        // ── Boiling ────────────────────────────────────────────────────────
        if (def->boiling_point > 0.f && T >= def->boiling_point) {
            ElementID into = resolve_tag(def->boil_into_tag);
            if (into == INVALID_ELEMENT_ID) into = def->boil_into;
            out.push_back({pos.x, pos.y, into, 0.f, -80.f}); // gas rises fast
            continue;
        }

        // ── Ignition ───────────────────────────────────────────────────────
        // Requires: ignition_point set, element is flammable, hot enough.
        // Uses a probability roll so ignition isn't perfectly instantaneous.
        if (def->ignition_point > 0.f &&
            def->flammability   > 0.f &&
            T >= def->ignition_point)
        {
            const float chance = def->flammability * 10.f * dt;
            if (dist01(m_rng) < chance) {
                const ElementID fire_id = resolve_tag("fire");
                if (fire_id != INVALID_ELEMENT_ID) {
                    // Spawn a fire pixel one cell above
                    out.push_back({pos.x, pos.y - 1, fire_id, 0.f, -60.f});

                    // Replace burning pixel with ash, or remove it
                    const ElementID ash_id = resolve_tag(def->ash_into_tag);
                    out.push_back({pos.x, pos.y,
                                   ash_id,  // INVALID_ELEMENT_ID = vanish
                                   0.f, 0.f});
                }
            }
            continue;
        }

        // ── Solidification / condensation ─────────────────────────────────
        // Triggered when temperature drops AT OR BELOW solidify_point.
        // Examples: lava cools → stone; steam condenses → water; water freezes → ice.
        if (def->solidify_point >= 0.f && T <= def->solidify_point) {
            ElementID into = resolve_tag(def->solidify_into_tag);
            if (into != INVALID_ELEMENT_ID) {
                // Solidification produces a settled pixel directly (vel=0)
                out.push_back({pos.x, pos.y, into, 0.f, 0.f});
            }
            continue;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void ReactionSystem::evaluate_contact_reactions(float dt,
                                                 std::vector<PendingChange>& out)
{
    const auto& cfg = m_world.config();
    auto&       lat = m_world.lattice();
    auto&       reg = m_world.registry();

    std::uniform_real_distribution<float> dist01(0.f, 1.f);

    // Track positions that are already queued for removal/replacement this
    // tick so we don't process the same cell twice.
    std::unordered_set<uint64_t> consumed;
    auto cell_key = [&](int x, int y) -> uint64_t {
        return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) |
               static_cast<uint32_t>(y);
    };

    for (auto& [pos, px] : lat.query_rect({0, 0, cfg.width, cfg.height})) {
        if (!px || px->id == INVALID_PIXEL_ID) continue;
        if (consumed.count(cell_key(pos.x, pos.y))) continue;

        const ElementDef* def = reg.get(px->element);
        if (!def || def->reactions.empty()) continue;

        for (const auto& off : CARD) {
            const int nx = pos.x + off.x;
            const int ny = pos.y + off.y;

            const SettledPixel* npx = lat.get(nx, ny);
            if (!npx || npx->id == INVALID_PIXEL_ID) continue;
            if (consumed.count(cell_key(nx, ny))) continue;

            const ElementDef* ndef = reg.get(npx->element);
            if (!ndef) continue;

            for (const auto& rxn : def->reactions) {
                if (rxn.target_tag != ndef->tag) continue;

                // Probability is per-tick (1.0 = always fires, 0.0 = never).
                // Do NOT scale by dt — contact reactions are discrete events.
                if (dist01(m_rng) >= rxn.probability) continue;

                // Queue replacement of THIS pixel
                if (!rxn.self_into_tag.empty()) {
                    const ElementID into = resolve_tag(rxn.self_into_tag);
                    out.push_back({pos.x, pos.y, into, 0.f, 0.f});
                } else {
                    // No change to self — do nothing for this cell
                }
                consumed.insert(cell_key(pos.x, pos.y));

                // Queue replacement/removal of the OTHER pixel
                {
                    const ElementID into = rxn.other_into_tag.empty()
                                           ? INVALID_ELEMENT_ID
                                           : resolve_tag(rxn.other_into_tag);
                    out.push_back({nx, ny, into, 0.f, -30.f});
                    consumed.insert(cell_key(nx, ny));
                }
                break; // one reaction per neighbour per tick
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void ReactionSystem::apply_changes(std::vector<PendingChange>& changes) {
    auto& reg = m_world.registry();

    std::uniform_real_distribution<float> life(1.5f, 4.f);

    for (const auto& ch : changes) {
        // Carry temperature forward from the consumed pixel
        float old_temp = ambient_temperature;

        if (auto* px = m_world.get_settled(ch.wx, ch.wy)) {
            old_temp = px->temperature;
            m_world.bonds().break_bonds_for_pixel(px->id);
            m_world.remove_settled(ch.wx, ch.wy);
        }

        if (ch.new_element == INVALID_ELEMENT_ID) continue;

        const ElementDef* def = reg.get(ch.new_element);
        if (!def) continue;

        DynamicPixel dp{};
        dp.base.element     = ch.new_element;
        dp.base.world_pos   = {ch.wx, ch.wy};
        dp.base.temperature = old_temp;
        dp.base.color       = def->color;
        dp.pos              = {static_cast<float>(ch.wx),
                               static_cast<float>(ch.wy)};
        dp.vel              = {ch.spawn_vel_x, ch.spawn_vel_y};
        dp.awake            = true;

        // Give fire and gas a limited lifetime so they naturally disappear
        if (def->physics == PhysicsModel::Fire ||
            def->physics == PhysicsModel::Gas)
        {
            dp.lifetime = life(m_rng);
        }

        m_world.add_dynamic(dp);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

ElementID ReactionSystem::resolve_tag(std::string_view tag) const {
    if (tag.empty()) return INVALID_ELEMENT_ID;
    return m_world.registry().id_of(tag);
}

} // namespace pf
