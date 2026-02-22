#include <catch2/catch_test_macros.hpp>
#include <pixelforge/lattice/lattice.hpp>

static pf::SettledPixel make_pixel(pf::ElementID elem, int wx, int wy) {
    pf::SettledPixel sp;
    sp.element   = elem;
    sp.world_pos = {wx, wy};
    return sp;
}

TEST_CASE("Lattice: set and get", "[lattice]") {
    pf::Lattice lat;
    lat.set(10, 20, make_pixel(1, 10, 20));

    REQUIRE(lat.has(10, 20));
    REQUIRE(lat.get(10, 20) != nullptr);
    REQUIRE(lat.get(10, 20)->element == 1);
    REQUIRE(lat.get(0, 0) == nullptr);
}

TEST_CASE("Lattice: remove", "[lattice]") {
    pf::Lattice lat;
    lat.set(5, 5, make_pixel(2, 5, 5));
    REQUIRE(lat.has(5, 5));

    lat.remove(5, 5);
    REQUIRE_FALSE(lat.has(5, 5));
    REQUIRE(lat.get(5, 5) == nullptr);
}

TEST_CASE("Lattice: has", "[lattice]") {
    pf::Lattice lat;
    REQUIRE_FALSE(lat.has(100, 200));
    lat.set(100, 200, make_pixel(3, 100, 200));
    REQUIRE(lat.has(100, 200));
}

TEST_CASE("Lattice: query_rect", "[lattice]") {
    pf::Lattice lat;
    for (int x = 0; x < 5; ++x)
        for (int y = 0; y < 5; ++y)
            lat.set(x, y, make_pixel(1, x, y));

    pf::AABB region{1, 1, 3, 3}; // x=1..3, y=1..3
    auto results = lat.query_rect(region);
    REQUIRE(results.size() == 9);
}

TEST_CASE("Lattice: size", "[lattice]") {
    pf::Lattice lat;
    REQUIRE(lat.size() == 0);
    lat.set(1, 1, make_pixel(1, 1, 1));
    lat.set(2, 2, make_pixel(1, 2, 2));
    REQUIRE(lat.size() == 2);
    lat.remove(1, 1);
    REQUIRE(lat.size() == 1);
}

TEST_CASE("Lattice: overwrite pixel", "[lattice]") {
    pf::Lattice lat;
    lat.set(0, 0, make_pixel(1, 0, 0));
    lat.set(0, 0, make_pixel(2, 0, 0));
    REQUIRE(lat.get(0, 0)->element == 2);
    REQUIRE(lat.size() == 1);
}
