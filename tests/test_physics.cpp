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
