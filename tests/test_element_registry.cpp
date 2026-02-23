#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <pixelforge/element/element_registry.hpp>

TEST_CASE("ElementRegistry: default air element at ID 0", "[element_registry]") {
    pf::ElementRegistry reg;
    REQUIRE(reg.size() == 1);
    const auto* air = reg.get(0);
    REQUIRE(air != nullptr);
    REQUIRE(air->tag == "air");
}

TEST_CASE("ElementRegistry: register and lookup by id", "[element_registry]") {
    pf::ElementRegistry reg;

    pf::ElementDef def;
    def.name    = "Sand";
    def.tag     = "sand";
    def.physics = pf::PhysicsModel::Powder;
    def.density = 1.6f;

    const auto id = reg.register_element(def);
    REQUIRE(id > 0);

    const auto* found = reg.get(id);
    REQUIRE(found != nullptr);
    REQUIRE(found->name == "Sand");
    REQUIRE_THAT(found->density, Catch::Matchers::WithinAbs(1.6f, 0.001f));
}

TEST_CASE("ElementRegistry: lookup by tag", "[element_registry]") {
    pf::ElementRegistry reg;

    pf::ElementDef def;
    def.name = "Water";
    def.tag  = "water";
    reg.register_element(def);

    const auto* found = reg.get("water");
    REQUIRE(found != nullptr);
    REQUIRE(found->name == "Water");

    REQUIRE(reg.get("nonexistent") == nullptr);
}

TEST_CASE("ElementRegistry: id_of", "[element_registry]") {
    pf::ElementRegistry reg;

    pf::ElementDef def;
    def.name = "Stone";
    def.tag  = "stone";
    const auto id = reg.register_element(def);

    REQUIRE(reg.id_of("stone") == id);
    REQUIRE(reg.id_of("missing") == pf::INVALID_ELEMENT_ID);
}

TEST_CASE("ElementRegistry: duplicate tag throws", "[element_registry]") {
    pf::ElementRegistry reg;

    pf::ElementDef def;
    def.name = "Dup";
    def.tag  = "dup";
    reg.register_element(def);

    pf::ElementDef def2;
    def2.name = "Dup2";
    def2.tag  = "dup";
    REQUIRE_THROWS(reg.register_element(def2));
}
