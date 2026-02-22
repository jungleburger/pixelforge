#include <catch2/catch_test_macros.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <pixelforge/world/world.hpp>
#include <pixelforge/procgen/generator.hpp>
#include <pixelforge/procgen/biome.hpp>

static pf::ElementRegistry make_registry() {
    pf::ElementRegistry reg;

    auto add = [&](const char* name, const char* tag, pf::PhysicsModel pm) {
        pf::ElementDef d;
        d.name    = name;
        d.tag     = tag;
        d.physics = pm;
        reg.register_element(d);
    };

    add("Sand",  "sand",  pf::PhysicsModel::Powder);
    add("Stone", "stone", pf::PhysicsModel::Solid);
    add("Lava",  "lava",  pf::PhysicsModel::Liquid);
    return reg;
}

TEST_CASE("WorldGenerator: terrain fills world", "[procgen]") {
    auto registry = make_registry();
    pf::WorldConfig cfg;
    cfg.seed   = 99;
    cfg.width  = 64;
    cfg.height = 64;
    pf::World world{cfg, registry};

    pf::BiomeRegistry biomes;
    pf::register_default_biomes(biomes, registry);

    pf::WorldGenerator gen{world, biomes};
    gen.generate();

    // After generation lattice should be non-empty
    REQUIRE(world.lattice().size() > 0);

    // At least 30% of cells should be populated (rough check)
    const size_t total = static_cast<size_t>(cfg.width) * cfg.height;
    REQUIRE(world.lattice().size() > total / 3);
}

TEST_CASE("WorldGenerator: biome depth ranges respected", "[procgen]") {
    auto registry = make_registry();
    pf::BiomeRegistry biomes;
    pf::register_default_biomes(biomes, registry);

    // Surface biome 0.0–0.2
    const auto* surf = biomes.get_biome_at(0.1f);
    REQUIRE(surf != nullptr);
    REQUIRE(surf->name == "Surface");

    // Underground biome 0.2–0.75
    const auto* mid = biomes.get_biome_at(0.5f);
    REQUIRE(mid != nullptr);
    REQUIRE(mid->name == "Underground");

    // Deep biome 0.75–1.0
    const auto* deep = biomes.get_biome_at(0.9f);
    REQUIRE(deep != nullptr);
    REQUIRE(deep->name == "Deep");
}

TEST_CASE("WorldGenerator: bonds initialised after generate", "[procgen]") {
    auto registry = make_registry();
    pf::WorldConfig cfg;
    cfg.seed   = 7;
    cfg.width  = 32;
    cfg.height = 32;
    pf::World world{cfg, registry};

    pf::BiomeRegistry biomes;
    pf::register_default_biomes(biomes, registry);

    pf::WorldGenerator gen{world, biomes};
    gen.generate();

    // Bonds should exist if there are adjacent cells
    // (plant_initial_bonds is called; world must have at least some bonds)
    // We can't assert a specific count without fixed-seed determinism,
    // but we can check bonds exist when lattice is non-empty.
    if (world.lattice().size() > 1) {
        REQUIRE(world.bonds().size() >= 1);
    }
}
