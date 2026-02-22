#include <pixelforge/core/logger.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <pixelforge/scripting/lua_api.hpp>
#include <pixelforge/world/world.hpp>
#include <pixelforge/physics/particle_system.hpp>
#include <pixelforge/procgen/generator.hpp>
#include <pixelforge/procgen/biome.hpp>
#include <sol/sol.hpp>
#include <filesystem>
#include <iostream>

int main(int /*argc*/, char* /*argv*/[]) {
    pf::Logger::instance().set_level(pf::LogLevel::Debug);

    pf::ElementRegistry registry;

    sol::state lua;
    pf::WorldConfig cfg;
    cfg.seed   = 12345;
    cfg.width  = 256;
    cfg.height = 128;

    pf::World world{cfg, registry};
    pf::LuaApi api{world, registry};
    api.bind(lua);

    // Load element definitions from assets/elements/
    const std::filesystem::path elem_dir = "assets/elements";
    for (const auto& entry : std::filesystem::directory_iterator(elem_dir)) {
        if (entry.path().extension() == ".lua") {
            PF_LOG_INFO("Loading element: {}", entry.path().string());
            api.load_element_file(lua, entry.path().string().c_str());
        }
    }

    // Setup biomes and generate world
    pf::BiomeRegistry biomes;
    pf::register_default_biomes(biomes, registry);

    pf::WorldGenerator gen{world, biomes};
    gen.generate();

    // Run a few physics ticks
    pf::ParticleSystem particles{world};
    for (int i = 0; i < 120; ++i) {
        particles.tick();
    }

    PF_LOG_INFO("Sandbox: lattice size = {}", world.lattice().size());
    PF_LOG_INFO("Sandbox: done.");
    return 0;
}
