#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <pixelforge/bond/bond.hpp>

TEST_CASE("BondManager: create bond", "[bonding]") {
    pf::BondManager bonds;
    const uint64_t bid = bonds.create_bond(1, 2, pf::Direction::North, 1.0f);
    REQUIRE(bid != 0);
    REQUIRE(bonds.size() == 1);

    const pf::Bond* b = bonds.get(bid);
    REQUIRE(b != nullptr);
    REQUIRE(b->pixel_a_id == 1);
    REQUIRE(b->pixel_b_id == 2);
    REQUIRE(b->direction  == pf::Direction::North);
}

TEST_CASE("BondManager: get bonds for pixel", "[bonding]") {
    pf::BondManager bonds;
    bonds.create_bond(10, 20, pf::Direction::East);
    bonds.create_bond(10, 30, pf::Direction::West);

    auto v = bonds.get_bonds_for_pixel(10);
    REQUIRE(v.size() == 2);
}

TEST_CASE("BondManager: break bond", "[bonding]") {
    pf::BondManager bonds;
    const auto bid = bonds.create_bond(1, 2, pf::Direction::South);
    REQUIRE(bonds.size() == 1);

    bonds.break_bond(bid);
    REQUIRE(bonds.size() == 0);
    REQUIRE(bonds.get(bid) == nullptr);
}

TEST_CASE("BondManager: break all bonds for pixel", "[bonding]") {
    pf::BondManager bonds;
    bonds.create_bond(5, 6, pf::Direction::North);
    bonds.create_bond(5, 7, pf::Direction::South);
    bonds.create_bond(8, 9, pf::Direction::East);  // unrelated

    bonds.break_bonds_for_pixel(5);

    REQUIRE(bonds.size() == 1);  // only the unrelated bond remains
    REQUIRE(bonds.get_bonds_for_pixel(5).empty());
}

TEST_CASE("BondManager: tick_age increments age", "[bonding]") {
    pf::BondManager bonds;
    const auto bid = bonds.create_bond(1, 2, pf::Direction::North);
    bonds.tick_age(0.5f);
    const pf::Bond* b = bonds.get(bid);
    REQUIRE(b != nullptr);
    REQUIRE_THAT(b->age, Catch::Matchers::WithinAbs(0.5f, 0.001f));
}
