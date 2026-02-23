#pragma once
#include <pixelforge/world/world.hpp>
#include <pixelforge/element/element_registry.hpp>

// Forward-declare sol::state to avoid pulling in sol2 into engine public headers.
namespace sol { class state; }

namespace pf {

class LuaApi {
public:
    LuaApi(World& world, ElementRegistry& registry);
    ~LuaApi();

    // Register all PixelForge bindings into a sol::state.
    void bind(sol::state& lua);

    // Load and execute a Lua element definition file.
    // Returns true on success; logs errors on failure.
    [[nodiscard]] bool load_element_file(sol::state& lua,
                                         const char* path);

private:
    World&           m_world;
    ElementRegistry& m_registry;
};

} // namespace pf
