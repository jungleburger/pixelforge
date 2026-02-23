#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <pixelforge/reaction/reaction_system.hpp>
#include <pixelforge/world/world.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static pf::ElementRegistry make_registry() {
    pf::ElementRegistry reg;

    // Stone — high melting point, low conductivity
    pf::ElementDef stone;
    stone.name                = "Stone";
    stone.tag                 = "stone";
    stone.physics             = pf::PhysicsModel::Solid;
    stone.density             = 2.5f;
    stone.melting_point       = 1600.f;
    stone.melt_into_tag       = "lava";
    stone.thermal_conductivity= 0.02f;
    reg.register_element(stone);

    // Lava — liquid, radiates heat, solidifies back below 800 °C
    pf::ElementDef lava;
    lava.name                 = "Lava";
    lava.tag                  = "lava";
    lava.physics              = pf::PhysicsModel::Liquid;
    lava.density              = 2.2f;
    lava.melting_point        = 800.f;
    lava.melt_into_tag        = "stone";
    lava.heat_output          = 500.f;
    lava.thermal_conductivity = 0.5f;
    reg.register_element(lava);

    // Ice — solid, melts to water at 0 °C
    pf::ElementDef ice;
    ice.name                  = "Ice";
    ice.tag                   = "ice";
    ice.physics               = pf::PhysicsModel::Solid;
    ice.density               = 0.92f;
    ice.melting_point         = 0.f;     // any temperature >= 0 °C melts it
    ice.melt_into_tag         = "water";
    ice.thermal_conductivity  = 0.2f;
    reg.register_element(ice);

    // Water — liquid, boils to steam at 100 °C
    pf::ElementDef water;
    water.name                = "Water";
    water.tag                 = "water";
    water.physics             = pf::PhysicsModel::Liquid;
    water.density             = 1.f;
    water.boiling_point       = 100.f;
    water.boil_into_tag       = "steam";
    water.thermal_conductivity= 0.3f;
    reg.register_element(water);

    // Steam — gas
    pf::ElementDef steam;
    steam.name                = "Steam";
    steam.tag                 = "steam";
    steam.physics             = pf::PhysicsModel::Gas;
    steam.density             = 0.0006f;
    reg.register_element(steam);

    // Wood — flammable solid
    pf::ElementDef wood;
    wood.name                 = "Wood";
    wood.tag                  = "wood";
    wood.physics              = pf::PhysicsModel::Solid;
    wood.density              = 0.6f;
    wood.flammability         = 0.4f;
    wood.ignition_point       = 300.f;
    wood.ash_into_tag         = "stone";
    wood.thermal_conductivity = 0.05f;
    reg.register_element(wood);

    // Fire — active heat source
    pf::ElementDef fire;
    fire.name                 = "Fire";
    fire.tag                  = "fire";
    fire.physics              = pf::PhysicsModel::Fire;
    fire.density              = 0.01f;
    fire.heat_output          = 800.f;
    fire.thermal_conductivity = 0.f;
    reg.register_element(fire);

    return reg;
}

static pf::World make_world(pf::ElementRegistry& reg) {
    pf::WorldConfig cfg;
    cfg.width  = 64;
    cfg.height = 64;
    return pf::World{cfg, reg};
}

static pf::SettledPixel make_pixel(pf::ElementID elem, int wx, int wy,
                                   float temp = 20.f) {
    static pf::PixelID next_id = 1;
    pf::SettledPixel px;
    px.id          = next_id++;
    px.element     = elem;
    px.world_pos   = {wx, wy};
    px.temperature = temp;
    return px;
}

// ─────────────────────────────────────────────────────────────────────────────
// Temperature propagation
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("ReactionSystem: heat conducts from hot pixel to cold neighbour", "[reactions]") {
    auto reg   = make_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID stone_id = reg.id_of("stone");

    // Place two stone pixels side by side with different temperatures
    world.set_settled(10, 10, make_pixel(stone_id, 10, 10, 500.f)); // hot
    world.set_settled(11, 10, make_pixel(stone_id, 11, 10, 20.f));  // cold

    reactions.tick(1.0f); // 1-second tick for measurable change

    const auto* hot  = world.get_settled(10, 10);
    const auto* cold = world.get_settled(11, 10);

    REQUIRE(hot  != nullptr);
    REQUIRE(cold != nullptr);

    // Hot pixel should have lost some heat
    REQUIRE(hot->temperature < 500.f);
    // Cold pixel should have gained some heat
    REQUIRE(cold->temperature > 20.f);
}

TEST_CASE("ReactionSystem: isolated pixel cools toward ambient", "[reactions]") {
    auto reg   = make_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID stone_id = reg.id_of("stone");
    world.set_settled(5, 5, make_pixel(stone_id, 5, 5, 400.f));

    // Run 60 ticks (~1 second at 60Hz DT)
    for (int i = 0; i < 60; ++i) {
        reactions.tick(1.f / 60.f);
    }

    const auto* px = world.get_settled(5, 5);
    REQUIRE(px != nullptr);
    // Should have cooled, still above ambient after just one second
    REQUIRE(px->temperature < 400.f);
    REQUIRE(px->temperature >= reactions.ambient_temperature);
}

// ─────────────────────────────────────────────────────────────────────────────
// Melting
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("ReactionSystem: ice melts when above 0 C", "[reactions]") {
    auto reg   = make_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID ice_id = reg.id_of("ice");
    world.set_settled(20, 20, make_pixel(ice_id, 20, 20, 50.f)); // above 0 °C

    reactions.tick(1.f / 60.f);

    // Ice pixel should be gone from the lattice (became a dynamic water pixel)
    REQUIRE_FALSE(world.has_settled(20, 20));

    // A dynamic pixel (water) should have been spawned
    REQUIRE(world.collect_all_dynamic().size() >= 1);
}

TEST_CASE("ReactionSystem: stone does not melt at low temperature", "[reactions]") {
    auto reg   = make_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID stone_id = reg.id_of("stone");
    world.set_settled(30, 30, make_pixel(stone_id, 30, 30, 500.f)); // well below 1600 °C

    reactions.tick(1.f / 60.f);

    // Stone should still be there
    REQUIRE(world.has_settled(30, 30));
}

TEST_CASE("ReactionSystem: stone melts to lava above melting point", "[reactions]") {
    auto reg   = make_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID stone_id = reg.id_of("stone");
    world.set_settled(30, 30, make_pixel(stone_id, 30, 30, 1700.f)); // above 1600 °C

    reactions.tick(1.f / 60.f);

    // Stone pixel removed from lattice
    REQUIRE_FALSE(world.has_settled(30, 30));

    // A dynamic lava pixel should have been spawned
    auto dynamics = world.collect_all_dynamic();
    REQUIRE(dynamics.size() >= 1);
    const pf::ElementID lava_id = reg.id_of("lava");
    REQUIRE(dynamics.front()->base.element == lava_id);
}

// ─────────────────────────────────────────────────────────────────────────────
// Boiling
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("ReactionSystem: water boils to steam above 100 C", "[reactions]") {
    auto reg   = make_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID water_id = reg.id_of("water");
    world.set_settled(15, 15, make_pixel(water_id, 15, 15, 150.f)); // above 100 °C

    reactions.tick(1.f / 60.f);

    REQUIRE_FALSE(world.has_settled(15, 15));

    auto dynamics = world.collect_all_dynamic();
    REQUIRE(dynamics.size() >= 1);
    const pf::ElementID steam_id = reg.id_of("steam");
    REQUIRE(dynamics.front()->base.element == steam_id);
}

TEST_CASE("ReactionSystem: water does not boil below 100 C", "[reactions]") {
    auto reg   = make_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID water_id = reg.id_of("water");
    world.set_settled(15, 15, make_pixel(water_id, 15, 15, 80.f));

    reactions.tick(1.f / 60.f);

    REQUIRE(world.has_settled(15, 15));
}

// ─────────────────────────────────────────────────────────────────────────────
// Ignition
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("ReactionSystem: wood eventually ignites above ignition point", "[reactions]") {
    auto reg   = make_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID wood_id = reg.id_of("wood");
    // Place wood high enough that spawned fire (one cell up) is still in bounds
    world.set_settled(32, 32, make_pixel(wood_id, 32, 32, 500.f)); // above 300 °C

    // Ignition is probabilistic — run many ticks to guarantee it fires
    bool ignited = false;
    for (int i = 0; i < 600; ++i) {
        reactions.tick(1.f / 60.f);
        if (!world.has_settled(32, 32)) {
            ignited = true;
            break;
        }
    }

    REQUIRE(ignited);
}

TEST_CASE("ReactionSystem: wood does not ignite below ignition point", "[reactions]") {
    auto reg   = make_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID wood_id = reg.id_of("wood");
    world.set_settled(32, 32, make_pixel(wood_id, 32, 32, 200.f)); // below 300 °C

    for (int i = 0; i < 120; ++i) {
        reactions.tick(1.f / 60.f);
    }

    // Wood should still be there
    REQUIRE(world.has_settled(32, 32));
}

// ─────────────────────────────────────────────────────────────────────────────
// apply_heat (editor tool injection)
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("ReactionSystem: apply_heat raises pixel temperature", "[reactions]") {
    auto reg   = make_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID stone_id = reg.id_of("stone");
    world.set_settled(5, 5, make_pixel(stone_id, 5, 5, 20.f));

    reactions.apply_heat(5, 5, 300.f);

    const auto* px = world.get_settled(5, 5);
    REQUIRE(px != nullptr);
    REQUIRE_THAT(px->temperature, Catch::Matchers::WithinAbs(320.f, 1.f));
}

TEST_CASE("ReactionSystem: apply_heat clamps to max temperature", "[reactions]") {
    auto reg   = make_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID stone_id = reg.id_of("stone");
    world.set_settled(5, 5, make_pixel(stone_id, 5, 5, 9990.f));

    reactions.apply_heat(5, 5, 1000.f); // would exceed 9999

    const auto* px = world.get_settled(5, 5);
    REQUIRE(px != nullptr);
    REQUIRE(px->temperature <= 9999.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 4: Solidification / condensation
// ─────────────────────────────────────────────────────────────────────────────

static pf::ElementRegistry make_phase4_registry() {
    pf::ElementRegistry reg;

    // Stone
    pf::ElementDef stone;
    stone.name                = "Stone";
    stone.tag                 = "stone";
    stone.physics             = pf::PhysicsModel::Solid;
    stone.density             = 2.5f;
    stone.melting_point       = 1600.f;
    stone.melt_into_tag       = "lava";
    stone.thermal_conductivity= 0.02f;
    reg.register_element(stone);

    // Lava — solidifies to stone below 800 °C
    pf::ElementDef lava;
    lava.name                 = "Lava";
    lava.tag                  = "lava";
    lava.physics              = pf::PhysicsModel::Liquid;
    lava.density              = 2.2f;
    lava.heat_output          = 500.f;
    lava.thermal_conductivity = 0.5f;
    lava.solidify_point       = 800.f;
    lava.solidify_into_tag    = "stone";
    reg.register_element(lava);

    // Water
    pf::ElementDef water;
    water.name                = "Water";
    water.tag                 = "water";
    water.physics             = pf::PhysicsModel::Liquid;
    water.density             = 1.f;
    water.boiling_point       = 100.f;
    water.boil_into_tag       = "steam";
    water.thermal_conductivity= 0.3f;
    water.solidify_point      = 0.f;
    water.solidify_into_tag   = "ice";
    reg.register_element(water);

    // Ice
    pf::ElementDef ice;
    ice.name                  = "Ice";
    ice.tag                   = "ice";
    ice.physics               = pf::PhysicsModel::Solid;
    ice.density               = 0.92f;
    ice.melting_point         = 0.f;
    ice.melt_into_tag         = "water";
    ice.thermal_conductivity  = 0.2f;
    reg.register_element(ice);

    // Steam — condenses back to water below 100 °C
    pf::ElementDef steam;
    steam.name                = "Steam";
    steam.tag                 = "steam";
    steam.physics             = pf::PhysicsModel::Gas;
    steam.density             = 0.0006f;
    steam.solidify_point      = 100.f;
    steam.solidify_into_tag   = "water";
    reg.register_element(steam);

    // Acid — dissolves stone and wood on contact
    pf::ElementDef acid;
    acid.name                 = "Acid";
    acid.tag                  = "acid";
    acid.physics              = pf::PhysicsModel::Liquid;
    acid.density              = 1.2f;
    acid.thermal_conductivity = 0.1f;
    acid.reactions.push_back({"stone", "", "", 1.f});       // destroys stone, acid unchanged
    acid.reactions.push_back({"ice",   "water", "water", 1.f}); // acid + ice → water + water
    reg.register_element(acid);

    // Wood
    pf::ElementDef wood;
    wood.name                 = "Wood";
    wood.tag                  = "wood";
    wood.physics              = pf::PhysicsModel::Solid;
    wood.density              = 0.6f;
    wood.flammability         = 0.4f;
    wood.ignition_point       = 300.f;
    wood.ash_into_tag         = "stone";
    wood.thermal_conductivity = 0.05f;
    reg.register_element(wood);

    // Fire
    pf::ElementDef fire;
    fire.name                 = "Fire";
    fire.tag                  = "fire";
    fire.physics              = pf::PhysicsModel::Fire;
    fire.density              = 0.01f;
    fire.heat_output          = 800.f;
    fire.thermal_conductivity = 0.f;
    reg.register_element(fire);

    return reg;
}

TEST_CASE("ReactionSystem: lava solidifies to stone below solidify_point", "[reactions][phase4]") {
    auto reg   = make_phase4_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID lava_id = reg.id_of("lava");

    // Lava at 500 °C (below 800 °C solidify_point)
    world.set_settled(10, 10, make_pixel(lava_id, 10, 10, 500.f));

    reactions.tick(1.f / 60.f);

    // Original lava pixel should have been removed from lattice
    REQUIRE_FALSE(world.has_settled(10, 10));

    // A dynamic stone pixel should have been spawned (will settle next frame)
    auto dynamics = world.collect_all_dynamic();
    REQUIRE(dynamics.size() >= 1);
    const pf::ElementID stone_id = reg.id_of("stone");
    REQUIRE(dynamics.front()->base.element == stone_id);
}

TEST_CASE("ReactionSystem: lava does NOT solidify above solidify_point", "[reactions][phase4]") {
    auto reg   = make_phase4_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID lava_id = reg.id_of("lava");

    // Lava at 1000 °C (above 800 °C solidify_point)
    world.set_settled(10, 10, make_pixel(lava_id, 10, 10, 1000.f));

    reactions.tick(1.f / 60.f);

    // Lava should still be present (it's hot enough to stay liquid)
    REQUIRE(world.has_settled(10, 10));
}

TEST_CASE("ReactionSystem: steam condenses to water below solidify_point", "[reactions][phase4]") {
    auto reg   = make_phase4_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID steam_id = reg.id_of("steam");

    // Steam at 50 °C (below 100 °C solidify_point)
    world.set_settled(20, 20, make_pixel(steam_id, 20, 20, 50.f));

    reactions.tick(1.f / 60.f);

    REQUIRE_FALSE(world.has_settled(20, 20));

    auto dynamics = world.collect_all_dynamic();
    REQUIRE(dynamics.size() >= 1);
    const pf::ElementID water_id = reg.id_of("water");
    REQUIRE(dynamics.front()->base.element == water_id);
}

TEST_CASE("ReactionSystem: water freezes to ice at/below solidify_point", "[reactions][phase4]") {
    auto reg   = make_phase4_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID water_id = reg.id_of("water");

    // Water at exactly 0 °C — should freeze
    world.set_settled(15, 15, make_pixel(water_id, 15, 15, 0.f));

    reactions.tick(1.f / 60.f);

    REQUIRE_FALSE(world.has_settled(15, 15));

    auto dynamics = world.collect_all_dynamic();
    REQUIRE(dynamics.size() >= 1);
    const pf::ElementID ice_id = reg.id_of("ice");
    REQUIRE(dynamics.front()->base.element == ice_id);
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 4: Contact reactions
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("ReactionSystem: acid dissolves adjacent stone", "[reactions][phase4]") {
    auto reg   = make_phase4_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID acid_id  = reg.id_of("acid");
    const pf::ElementID stone_id = reg.id_of("stone");

    world.set_settled(5, 5, make_pixel(acid_id,  5, 5, 20.f));
    world.set_settled(6, 5, make_pixel(stone_id, 6, 5, 20.f));

    // Acid has probability=1.0 for stone, so it fires on the first tick
    reactions.tick(1.f / 60.f);

    // Stone should have been removed
    REQUIRE_FALSE(world.has_settled(6, 5));
    // Acid itself should be unchanged (self_into = "" means no change)
    REQUIRE(world.has_settled(5, 5));
}

TEST_CASE("ReactionSystem: acid reacts with ice producing water", "[reactions][phase4]") {
    auto reg   = make_phase4_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID acid_id = reg.id_of("acid");
    const pf::ElementID ice_id  = reg.id_of("ice");

    world.set_settled(10, 10, make_pixel(acid_id, 10, 10, 20.f));
    world.set_settled(11, 10, make_pixel(ice_id,  11, 10, 20.f));

    reactions.tick(1.f / 60.f);

    // Ice should be gone
    REQUIRE_FALSE(world.has_settled(11, 10));

    // Acid should have been replaced with water (self_into = "water")
    // The acid pixel is removed from settled lattice and a dynamic water is spawned
    auto dynamics = world.collect_all_dynamic();
    const pf::ElementID water_id = reg.id_of("water");
    bool found_water = false;
    for (const auto* dp : dynamics) {
        if (dp && dp->base.element == water_id) { found_water = true; break; }
    }
    REQUIRE(found_water);
}

TEST_CASE("ReactionSystem: contact reaction does not fire when not adjacent", "[reactions][phase4]") {
    auto reg   = make_phase4_registry();
    auto world = make_world(reg);
    pf::ReactionSystem reactions{world};

    const pf::ElementID acid_id  = reg.id_of("acid");
    const pf::ElementID stone_id = reg.id_of("stone");

    // Place stone 2 cells away — not adjacent
    world.set_settled(5, 5, make_pixel(acid_id,  5, 5, 20.f));
    world.set_settled(7, 5, make_pixel(stone_id, 7, 5, 20.f));

    for (int i = 0; i < 60; ++i) {
        reactions.tick(1.f / 60.f);
    }

    // Stone should still be present — acid only reacts with direct neighbours
    REQUIRE(world.has_settled(7, 5));
}
