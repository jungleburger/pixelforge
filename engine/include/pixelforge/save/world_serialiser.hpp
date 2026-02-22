#pragma once
#include <pixelforge/world/world.hpp>
#include <expected>
#include <string>

namespace pf {

struct SaveError {
    std::string message;
};

class WorldSerialiser {
public:
    // Serialise world to a zstd-compressed binary file.
    [[nodiscard]] static std::expected<void, SaveError>
        save(const World& world, const char* path);

    // Deserialise world from a zstd-compressed binary file.
    // Returns a fully constructed World; registry must outlive it.
    [[nodiscard]] static std::expected<World, SaveError>
        load(const char* path, ElementRegistry& registry);
};

} // namespace pf
