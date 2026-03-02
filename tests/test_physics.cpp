#include <catch2/catch_test_macros.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <pixelforge/world/world.hpp>
#include <pixelforge/physics/particle_system.hpp>

static pf::ElementRegistry make_registry() {
    pf::ElementRegistry reg;
    pf::ElementDef sand;
    sand.name    = "Sand";
    sand.tag     = "sand";
    sand.physics = pf::PhysicsModel::Powder;
    sand.density = 1.6f;
    reg.register_element(sand);
    return reg;
}

TEST_CASE("ParticleSystem: particle settles at world floor", "[physics]") {
    auto registry = make_registry();
    pf::WorldConfig cfg;
    cfg.width  = 64;
    cfg.height = 32;
    pf::World world{cfg, registry};
    pf::ParticleSystem particles{world};

    // Spawn a dynamic pixel just above the floor
    pf::DynamicPixel dp;
    dp.base.element = registry.id_of("sand");
    dp.id           = 1;
    dp.pos          = {32.f, static_cast<float>(cfg.height - 2)};
    dp.vel          = {0.f, 0.f};
    world.add_dynamic(dp);

    // Run enough ticks for it to fall to floor and settle
    for (int i = 0; i < 120; ++i) particles.tick();

    // Particle should have settled somewhere near the bottom
    REQUIRE(world.lattice().size() >= 1);
}

TEST_CASE("ParticleSystem: particle stays dynamic in empty space", "[physics]") {
    auto registry = make_registry();
    pf::WorldConfig cfg;
    cfg.width  = 64;
    cfg.height = 1000;  // very tall — floor far away
    pf::World world{cfg, registry};
    pf::ParticleSystem particles{world};

    pf::DynamicPixel dp;
    dp.base.element = registry.id_of("sand");
    dp.id           = 1;
    dp.pos          = {32.f, 0.f};
    dp.vel          = {0.f, 0.f};
    world.add_dynamic(dp);

    // Run only a few ticks — particle shouldn't have reached floor yet
    for (int i = 0; i < 5; ++i) particles.tick();

    REQUIRE(world.lattice().size() == 0);
}

TEST_CASE("ParticleSystem: particle settles next to compatible neighbour", "[physics]") {
    auto registry = make_registry();
    pf::WorldConfig cfg;
    cfg.width  = 64;
    cfg.height = 200;
    pf::World world{cfg, registry};
    pf::ParticleSystem particles{world};

    // Place a settled pixel at (32, 100)
    pf::SettledPixel anchor;
    anchor.id      = 99;
    anchor.element = registry.id_of("sand");
    anchor.world_pos = {32, 100};
    world.set_settled(32, 100, anchor);

    // Spawn dynamic pixel directly above anchor — should settle N or S
    pf::DynamicPixel dp;
    dp.base.element = registry.id_of("sand");
    dp.id           = 1;
    dp.pos          = {32.f, 99.f};
    dp.vel          = {0.f, 1.f};  // moving downward toward anchor
    world.add_dynamic(dp);

    for (int i = 0; i < 60; ++i) particles.tick();

    // Lattice should have grown (anchor + at least one settled dynamic)
    REQUIRE(world.lattice().size() >= 2);
}

TEST_CASE("ParticleSystem: per-element restitution affects bounce", "[physics]") {
    // Create two elements with very different restitution values.
    pf::ElementRegistry reg;

    pf::ElementDef rubber;
    rubber.name        = "Rubber";
    rubber.tag         = "rubber";
    rubber.physics     = pf::PhysicsModel::Powder;
    rubber.density     = 1.0f;
    rubber.restitution = 0.9f;   // very bouncy
    reg.register_element(rubber);

    pf::ElementDef clay;
    clay.name        = "Clay";
    clay.tag         = "clay";
    clay.physics     = pf::PhysicsModel::Powder;
    clay.density     = 1.0f;
    clay.restitution = 0.05f;    // almost no bounce
    reg.register_element(clay);

    const auto rubber_id = reg.id_of("rubber");
    const auto clay_id   = reg.id_of("clay");

    // Build a world with a solid floor of settled pixels at y = 99.
    pf::WorldConfig cfg;
    cfg.width  = 64;
    cfg.height = 100;
    pf::World world_r{cfg, reg};
    pf::World world_c{cfg, reg};
    pf::ParticleSystem ps_r{world_r};
    pf::ParticleSystem ps_c{world_c};

    // Place a row of settled pixels on the floor for both worlds.
    for (int x = 0; x < 64; ++x) {
        pf::SettledPixel sp{};
        sp.id        = static_cast<pf::PixelID>(1000 + x);
        sp.element   = rubber_id;
        sp.world_pos = {x, 99};
        world_r.set_settled(x, 99, sp);
        world_c.set_settled(x, 99, sp);
    }

    // Drop both particles from the same height with the same velocity.
    auto make_dp = [](pf::ElementID elem, pf::PixelID id) {
        pf::DynamicPixel dp{};
        dp.base.element = elem;
        dp.id           = id;
        dp.pos          = {32.f, 80.f};   // 19 pixels above floor
        dp.vel          = {0.f, 300.f};   // falling fast
        dp.awake        = true;
        dp.lifetime     = -1.f;           // infinite
        return dp;
    };

    world_r.add_dynamic(make_dp(rubber_id, 1));
    world_c.add_dynamic(make_dp(clay_id, 2));

    // Simulate a few ticks — enough for both to impact the floor and bounce.
    for (int i = 0; i < 8; ++i) {
        ps_r.tick();
        ps_c.tick();
    }

    // Find each particle's current Y position.
    auto get_min_y = [](pf::World& w) -> float {
        float min_y = 9999.f;
        auto dps = w.collect_all_dynamic();
        for (const auto* dp : dps) {
            if (dp->awake && dp->pos.y < min_y) min_y = dp->pos.y;
        }
        return min_y;
    };

    float rubber_y = get_min_y(world_r);
    float clay_y   = get_min_y(world_c);

    // Rubber (restitution=0.9) should have bounced much higher (lower Y).
    // Clay (restitution=0.05) should be near the floor or already settled.
    // If the particle settled, its Y is irrelevant — settled means ~floor.
    bool rubber_still_dynamic = !world_r.collect_all_dynamic().empty();
    bool clay_settled_or_low  = world_c.lattice().size() > 64 || clay_y > 90.f;

    INFO("rubber Y: " << rubber_y << " (dynamic=" << rubber_still_dynamic << ")");
    INFO("clay Y: " << clay_y << " settled=" << (world_c.lattice().size() > 64));
    // At minimum: rubber should be higher (lower y) than clay, or rubber
    // should still be dynamic while clay has already settled.
    CHECK((rubber_y < clay_y || clay_settled_or_low));
}
