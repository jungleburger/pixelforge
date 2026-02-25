#include <pixelforge/core/logger.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <pixelforge/scripting/lua_api.hpp>
#include <pixelforge/world/world.hpp>
#include <pixelforge/physics/particle_system.hpp>
#include <pixelforge/procgen/generator.hpp>
#include <pixelforge/procgen/biome.hpp>
#include <pixelforge/procgen/world_config_loader.hpp>
#include <sol/sol.hpp>
#include <filesystem>
#include <iostream>

int main(int /*argc*/, char* /*argv*/[]) {
    pf::Logger::instance().set_level(pf::LogLevel::Debug);

    // ── Load world config from TOML ───────────────────────────────────────
    pf::WorldConfig cfg;
    if (auto loaded = pf::load_world_config("assets/worlds/sandbox_world.toml")) {
        cfg = *loaded;
        PF_LOG_INFO("Sandbox: using TOML world config ({} × {})", cfg.width, cfg.height);
    } else {
        PF_LOG_WARN("Sandbox: TOML config not found — using defaults");
        cfg.seed   = 12345;
        cfg.width  = 256;
        cfg.height = 128;
    }

    // ── Element registry + Lua ────────────────────────────────────────────
    pf::ElementRegistry registry;
    sol::state lua;

    pf::World  world{cfg, registry};
    pf::LuaApi api{world, registry};
    api.bind(lua);

    const std::filesystem::path elem_dir = "assets/elements";
    for (const auto& entry : std::filesystem::directory_iterator(elem_dir)) {
        if (entry.path().extension() == ".lua") {
            PF_LOG_INFO("Loading element: {}", entry.path().string());
            api.load_element_file(lua, entry.path().string().c_str());
        }
    }

    // ── Procedural generation ─────────────────────────────────────────────
    pf::BiomeRegistry biomes;
    pf::register_default_biomes(biomes, registry);

    pf::WorldGenerator gen{world, biomes};
    gen.generate();

    // ── Physics simulation ────────────────────────────────────────────────
    pf::ParticleSystem particles{world};
    for (int i = 0; i < 120; ++i) {
        particles.tick();
    }

    PF_LOG_INFO("Sandbox: lattice size = {}", world.lattice().size());
    PF_LOG_INFO("Sandbox: done.");
    return 0;
}

