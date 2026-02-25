#pragma once
#include <pixelforge/world/chunk.hpp>
#include <pixelforge/lattice/lattice.hpp>
#include <pixelforge/bond/bond.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <pixelforge/core/types.hpp>
#include <unordered_map>
#include <memory>
#include <vector>

namespace pf {

struct WorldConfig {
    uint32_t seed         = 42;
    int      width        = 4096;
    int      height       = 2048;
    float    cave_density = 0.35f;
    float    water_level  = 0.45f;
    float    lava_depth   = 0.82f;
};

class World {
public:
    explicit World(WorldConfig config, ElementRegistry& registry);

    [[nodiscard]] Chunk*  get_chunk(int cx, int cy);
    [[nodiscard]] Chunk&  get_or_create_chunk(int cx, int cy);

    [[nodiscard]] SettledPixel*       get_settled(int wx, int wy);
    [[nodiscard]] const SettledPixel* get_settled(int wx, int wy) const;
    void set_settled(int wx, int wy, SettledPixel pixel);
    void remove_settled(int wx, int wy);
    [[nodiscard]] bool has_settled(int wx, int wy) const;

    void add_dynamic(DynamicPixel pixel);
    [[nodiscard]] std::vector<DynamicPixel*> collect_all_dynamic();

    [[nodiscard]] BondManager&             bonds()    { return m_bonds; }
    [[nodiscard]] Lattice&                 lattice()  { return m_lattice; }
    [[nodiscard]] const Lattice&           lattice()  const { return m_lattice; }
    [[nodiscard]] ElementRegistry&         registry() { return m_registry; }
    [[nodiscard]] const ElementRegistry&   registry() const { return m_registry; }
    [[nodiscard]] const WorldConfig&       config()   const { return m_config; }

    [[nodiscard]] static std::pair<int,int> world_to_chunk(int wx, int wy) noexcept;
    [[nodiscard]] static std::pair<int,int> world_to_local(int wx, int wy) noexcept;
    [[nodiscard]] static std::pair<int,int> chunk_to_world(int cx, int cy, int lx, int ly) noexcept;

    [[nodiscard]] size_t chunk_count() const { return m_chunks.size(); }

private:
    WorldConfig      m_config;
    ElementRegistry& m_registry;
    Lattice          m_lattice;
    BondManager      m_bonds;
    PixelID          m_next_pixel_id{1};  // starts at 1; 0 == INVALID_PIXEL_ID

    std::unordered_map<uint64_t, std::unique_ptr<Chunk>> m_chunks;

    [[nodiscard]] static uint64_t chunk_key(int cx, int cy) noexcept;
};

} // namespace pf
