#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <pixelforge/world/world.hpp>
#include <pixelforge/save/world_serialiser.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

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
    add("Water", "water", pf::PhysicsModel::Liquid);
    add("Fire",  "fire",  pf::PhysicsModel::Fire);
    return reg;
}

// RAII temp file — deleted on scope exit
struct TempFile {
    std::string path;
    TempFile()
        : path((fs::temp_directory_path() / "pf_save_test.pfw").string()) {}
    ~TempFile() {
        std::error_code ec;
        fs::remove(path, ec);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Round-trip tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("WorldSerialiser: empty world round-trip", "[save]") {
    auto registry = make_registry();
    TempFile tmp;

    pf::WorldConfig cfg;
    cfg.seed   = 12345;
    cfg.width  = 64;
    cfg.height = 32;
    pf::World world{cfg, registry};

    auto save_result = pf::WorldSerialiser::save(world, tmp.path.c_str());
    REQUIRE(save_result.has_value());

    auto load_result = pf::WorldSerialiser::load(tmp.path.c_str(), registry);
    REQUIRE(load_result.has_value());

    const pf::World& loaded = *load_result;
    REQUIRE(loaded.config().width  == cfg.width);
    REQUIRE(loaded.config().height == cfg.height);
    REQUIRE(loaded.config().seed   == cfg.seed);
    REQUIRE(loaded.lattice().size() == 0);
}

TEST_CASE("WorldSerialiser: settled pixel attributes preserved", "[save]") {
    auto registry = make_registry();
    TempFile tmp;

    const pf::ElementID sand_id  = registry.id_of("sand");
    const pf::ElementID stone_id = registry.id_of("stone");
    const pf::ElementID water_id = registry.id_of("water");

    pf::WorldConfig cfg;
    cfg.seed   = 7;
    cfg.width  = 128;
    cfg.height = 64;
    pf::World world{cfg, registry};

    // Three pixels with distinct attributes
    {
        pf::SettledPixel sp;
        sp.element     = sand_id;
        sp.temperature = 42.5f;
        sp.hp          = 200;
        sp.color       = 0xFF'CC'99'FFu;
        world.set_settled(10, 20, sp);
    }
    {
        pf::SettledPixel sp;
        sp.element     = stone_id;
        sp.temperature = -10.f;
        sp.hp          = 255;
        sp.color       = 0x88'88'88'FFu;
        world.set_settled(0, 0, sp);
    }
    {
        pf::SettledPixel sp;
        sp.element     = water_id;
        sp.temperature = 100.f;
        sp.hp          = 128;
        sp.color       = 0x44'88'FF'FFu;
        world.set_settled(127, 63, sp);  // corner
    }

    auto save_result = pf::WorldSerialiser::save(world, tmp.path.c_str());
    REQUIRE(save_result.has_value());

    auto load_result = pf::WorldSerialiser::load(tmp.path.c_str(), registry);
    REQUIRE(load_result.has_value());

    const pf::World& loaded = *load_result;
    REQUIRE(loaded.lattice().size() == 3);

    {
        const pf::SettledPixel* sp = loaded.get_settled(10, 20);
        REQUIRE(sp != nullptr);
        REQUIRE(sp->element     == sand_id);
        REQUIRE(sp->temperature == Catch::Approx(42.5f));
        REQUIRE(sp->hp          == 200);
        REQUIRE(sp->color       == 0xFF'CC'99'FFu);
    }
    {
        const pf::SettledPixel* sp = loaded.get_settled(0, 0);
        REQUIRE(sp != nullptr);
        REQUIRE(sp->element     == stone_id);
        REQUIRE(sp->temperature == Catch::Approx(-10.f));
        REQUIRE(sp->hp          == 255);
        REQUIRE(sp->color       == 0x88'88'88'FFu);
    }
    {
        const pf::SettledPixel* sp = loaded.get_settled(127, 63);
        REQUIRE(sp != nullptr);
        REQUIRE(sp->element     == water_id);
        REQUIRE(sp->temperature == Catch::Approx(100.f));
        REQUIRE(sp->hp          == 128);
        REQUIRE(sp->color       == 0x44'88'FF'FFu);
    }
}

TEST_CASE("WorldSerialiser: world config fields preserved", "[save]") {
    auto registry = make_registry();
    TempFile tmp;

    pf::WorldConfig cfg;
    cfg.seed         = 0xDEAD'BEEFu;
    cfg.width        = 256;
    cfg.height       = 128;
    cfg.cave_density = 0.42f;
    cfg.water_level  = 0.33f;
    pf::World world{cfg, registry};

    REQUIRE(pf::WorldSerialiser::save(world, tmp.path.c_str()).has_value());

    auto loaded = pf::WorldSerialiser::load(tmp.path.c_str(), registry);
    REQUIRE(loaded.has_value());

    REQUIRE(loaded->config().seed   == cfg.seed);
    REQUIRE(loaded->config().width  == cfg.width);
    REQUIRE(loaded->config().height == cfg.height);
}

TEST_CASE("WorldSerialiser: large world round-trip", "[save]") {
    auto registry = make_registry();
    TempFile tmp;

    const pf::ElementID stone_id = registry.id_of("stone");

    pf::WorldConfig cfg;
    cfg.seed   = 1;
    cfg.width  = 256;
    cfg.height = 128;
    pf::World world{cfg, registry};

    constexpr int FILL_W = 100;
    constexpr int FILL_H = 50;

    for (int y = 0; y < FILL_H; ++y) {
        for (int x = 0; x < FILL_W; ++x) {
            pf::SettledPixel sp;
            sp.element     = stone_id;
            sp.temperature = static_cast<float>(x + y);
            sp.hp          = 255;
            sp.color       = 0xFF'FF'FF'FFu;
            world.set_settled(x, y, sp);
        }
    }

    REQUIRE(world.lattice().size() == static_cast<size_t>(FILL_W * FILL_H));

    REQUIRE(pf::WorldSerialiser::save(world, tmp.path.c_str()).has_value());

    auto loaded = pf::WorldSerialiser::load(tmp.path.c_str(), registry);
    REQUIRE(loaded.has_value());
    REQUIRE(loaded->lattice().size() == static_cast<size_t>(FILL_W * FILL_H));

    // Spot-check temperature encoding at regular intervals
    for (int y = 0; y < FILL_H; y += 10) {
        for (int x = 0; x < FILL_W; x += 10) {
            const pf::SettledPixel* sp = loaded->get_settled(x, y);
            REQUIRE(sp != nullptr);
            REQUIRE(sp->temperature == Catch::Approx(static_cast<float>(x + y)));
        }
    }
}

TEST_CASE("WorldSerialiser: multiple saves overwrite correctly", "[save]") {
    auto registry = make_registry();
    TempFile tmp;

    const pf::ElementID sand_id  = registry.id_of("sand");
    const pf::ElementID stone_id = registry.id_of("stone");

    pf::WorldConfig cfg;
    cfg.seed   = 3;
    cfg.width  = 32;
    cfg.height = 32;

    // First save: one sand pixel
    {
        pf::World world{cfg, registry};
        pf::SettledPixel sp;
        sp.element = sand_id;
        world.set_settled(5, 5, sp);
        REQUIRE(pf::WorldSerialiser::save(world, tmp.path.c_str()).has_value());
    }

    // Second save: one stone pixel at different position — must overwrite first
    {
        pf::World world{cfg, registry};
        pf::SettledPixel sp;
        sp.element = stone_id;
        world.set_settled(15, 15, sp);
        REQUIRE(pf::WorldSerialiser::save(world, tmp.path.c_str()).has_value());
    }

    auto loaded = pf::WorldSerialiser::load(tmp.path.c_str(), registry);
    REQUIRE(loaded.has_value());
    REQUIRE(loaded->lattice().size() == 1);

    // Should contain the stone pixel, not the sand
    REQUIRE(loaded->get_settled(15, 15) != nullptr);
    REQUIRE(loaded->get_settled(15, 15)->element == stone_id);
    REQUIRE(loaded->get_settled(5, 5) == nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// Error-handling tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("WorldSerialiser: save to invalid path returns error", "[save]") {
    auto registry = make_registry();
    pf::WorldConfig cfg;
    cfg.width  = 16;
    cfg.height = 16;
    pf::World world{cfg, registry};

    auto result = pf::WorldSerialiser::save(world, "/nonexistent_dir_pf/bad.pfw");
    REQUIRE_FALSE(result.has_value());
    REQUIRE_FALSE(result.error().message.empty());
}

TEST_CASE("WorldSerialiser: load from missing file returns error", "[save]") {
    auto registry = make_registry();
    auto result = pf::WorldSerialiser::load("/no_such_file_pf_test.pfw", registry);
    REQUIRE_FALSE(result.has_value());
    REQUIRE_FALSE(result.error().message.empty());
}

TEST_CASE("WorldSerialiser: load corrupt data returns error", "[save]") {
    auto registry = make_registry();
    TempFile tmp;

    // Write non-zstd garbage
    {
        std::ofstream of(tmp.path, std::ios::binary | std::ios::trunc);
        REQUIRE(of.is_open());
        const char garbage[] = "NOT_VALID_ZSTD_DATA_ABCDEFGHIJK";
        of.write(garbage, static_cast<std::streamsize>(sizeof(garbage)));
    }

    auto result = pf::WorldSerialiser::load(tmp.path.c_str(), registry);
    REQUIRE_FALSE(result.has_value());
    REQUIRE_FALSE(result.error().message.empty());
}
