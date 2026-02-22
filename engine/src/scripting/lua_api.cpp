#include <pixelforge/scripting/lua_api.hpp>
#include <pixelforge/core/logger.hpp>
#include <sol/sol.hpp>

namespace pf {

LuaApi::LuaApi(World& world, ElementRegistry& registry)
    : m_world(world), m_registry(registry)
{}

LuaApi::~LuaApi() = default;

void LuaApi::bind(sol::state& lua) {
    lua.open_libraries(sol::lib::base, sol::lib::math,
                        sol::lib::string, sol::lib::table,
                        sol::lib::io, sol::lib::os);

    // ElementDef builder
    auto& reg = m_registry;
    lua.set_function("define_element", [&reg](sol::table t) {
        ElementDef def;
        def.name        = t.get_or<std::string>("name", "Unknown");
        def.tag         = t.get_or<std::string>("tag",  "unknown");
        def.color       = t.get_or<uint32_t>("color", 0xFFAAFF88u);
        def.density     = t.get_or<float>("density", 1.f);
        def.viscosity   = t.get_or<float>("viscosity", 0.f);
        def.flammability= t.get_or<float>("flammability", 0.f);
        def.melting_point= t.get_or<float>("melting_point", -1.f);
        def.boiling_point= t.get_or<float>("boiling_point", -1.f);

        std::string physics_s = t.get_or<std::string>("physics", "solid");
        if      (physics_s == "powder") def.physics = PhysicsModel::Powder;
        else if (physics_s == "liquid") def.physics = PhysicsModel::Liquid;
        else if (physics_s == "gas")    def.physics = PhysicsModel::Gas;
        else if (physics_s == "fire")   def.physics = PhysicsModel::Fire;
        else if (physics_s == "plasma") def.physics = PhysicsModel::Plasma;
        else if (physics_s == "energy") def.physics = PhysicsModel::Energy;
        else                            def.physics = PhysicsModel::Solid;

        ElementID id = reg.register_element(std::move(def));
        PF_LOG_INFO("LuaApi: registered element '{}' id={}", def.tag, id);
        return static_cast<int>(id);
    });

    // Logger
    lua.set_function("pf_log", [](std::string msg) {
        PF_LOG_INFO("[Lua] {}", msg);
    });
}

bool LuaApi::load_element_file(sol::state& lua, const char* path) {
    auto result = lua.load_file(path);
    if (!result.valid()) {
        sol::error err = result;
        PF_LOG_ERROR("LuaApi: failed to load '{}': {}", path, err.what());
        return false;
    }
    auto call_result = result();
    if (!call_result.valid()) {
        sol::error err = call_result;
        PF_LOG_ERROR("LuaApi: error executing '{}': {}", path, err.what());
        return false;
    }
    return true;
}

} // namespace pf
