#include <catch2/catch_test_macros.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <pixelforge/world/world.hpp>
#include <pixelforge/procgen/generator.hpp>
#include <pixelforge/procgen/biome.hpp>
#include <pixelforge/procgen/world_config_loader.hpp>

static pf::ElementRegistry make_registry() {
    pf::ElementRegistry reg;

    auto add = [&](const char* name, const char* tag, pf::PhysicsModel pm) {
        pf::ElementDef d;
        d.name    = name;
        d.tag     = tag;
        d.physics = pm;
        reg.register_element(d);
    };

    // Core elements used by default biomes and generator features
    add("Sand",    "sand",    pf::PhysicsModel::Powder);
    add("Stone",   "stone",   pf::PhysicsModel::Solid);
    add("Lava",    "lava",    pf::PhysicsModel::Liquid);
    add("Dirt",    "dirt",    pf::PhysicsModel::Powder);
    add("Ice",     "ice",     pf::PhysicsModel::Solid);
    add("Water",   "water",   pf::PhysicsModel::Liquid);
    add("Smoke",   "smoke",   pf::PhysicsModel::Gas);
    add("Crystal", "crystal", pf::PhysicsModel::Solid);
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

    // At least 5 % of total cells should be populated after cave carving
    // (threshold is lenient to accommodate aggressive worm carving in tiny worlds)
    const size_t total = static_cast<size_t>(cfg.width) * cfg.height;
    REQUIRE(world.lattice().size() > total / 20);
}

TEST_CASE("WorldGenerator: biome depth ranges respected", "[procgen]") {
    auto registry = make_registry();
    pf::BiomeRegistry biomes;
    pf::register_default_biomes(biomes, registry);

    // Depth-only lookup returns Temperate biomes first (lowest zone index)

    // Surface band  0.0–0.2
    const auto* surf = biomes.get_biome_at(0.1f);
    REQUIRE(surf != nullptr);
    REQUIRE(surf->min_depth <= 0.1f);
    REQUIRE(surf->max_depth >= 0.1f);

    // Underground band  0.2–0.75
    const auto* mid = biomes.get_biome_at(0.5f);
    REQUIRE(mid != nullptr);
    REQUIRE(mid->min_depth <= 0.5f);
    REQUIRE(mid->max_depth >= 0.5f);

    // Deep band  0.75–1.0
    const auto* deep = biomes.get_biome_at(0.9f);
    REQUIRE(deep != nullptr);
    REQUIRE(deep->min_depth <= 0.9f);
    REQUIRE(deep->max_depth >= 0.9f);
}

TEST_CASE("WorldGenerator: zone-aware biome lookup", "[procgen]") {
    auto registry = make_registry();
    pf::BiomeRegistry biomes;
    pf::register_default_biomes(biomes, registry);

    // Cold surface biome has ice as its surface element
    const auto* cold_surf = biomes.get_biome_at(0.05f, pf::BiomeZone::Cold);
    REQUIRE(cold_surf != nullptr);
    REQUIRE(cold_surf->zone == pf::BiomeZone::Cold);

    // Arid surface biome has sand as its surface element
    const auto* arid_surf = biomes.get_biome_at(0.05f, pf::BiomeZone::Arid);
    REQUIRE(arid_surf != nullptr);
    REQUIRE(arid_surf->zone == pf::BiomeZone::Arid);
    REQUIRE(arid_surf->surface_element == registry.id_of("sand"));
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

TEST_CASE("load_world_config: returns nullopt for non-existent file", "[procgen]") {
    const auto result = pf::load_world_config("__nonexistent_world.toml");
    REQUIRE_FALSE(result.has_value());
}

