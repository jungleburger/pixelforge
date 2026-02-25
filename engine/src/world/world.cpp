#include <pixelforge/world/world.hpp>
#include <pixelforge/core/logger.hpp>

namespace pf {

World::World(WorldConfig config, ElementRegistry& registry)
    : m_config(config), m_registry(registry)
{}

Chunk* World::get_chunk(int cx, int cy) {
    auto it = m_chunks.find(chunk_key(cx, cy));
    return (it != m_chunks.end()) ? it->second.get() : nullptr;
}

Chunk& World::get_or_create_chunk(int cx, int cy) {
    auto key = chunk_key(cx, cy);
    auto it  = m_chunks.find(key);
    if (it != m_chunks.end()) return *it->second;

    auto chunk      = std::make_unique<Chunk>();
    chunk->chunk_x  = cx;
    chunk->chunk_y  = cy;
    Chunk& ref      = *chunk;
    m_chunks.emplace(key, std::move(chunk));
    return ref;
}

SettledPixel* World::get_settled(int wx, int wy) {
    return m_lattice.get(wx, wy);
}

const SettledPixel* World::get_settled(int wx, int wy) const {
    return m_lattice.get(wx, wy);
}

void World::set_settled(int wx, int wy, SettledPixel pixel) {
    // Auto-assign a unique ID if the caller left it unset (id == 0)
    if (pixel.id == INVALID_PIXEL_ID)
        pixel.id = m_next_pixel_id++;
    auto [cx, cy] = world_to_chunk(wx, wy);
    auto [lx, ly] = world_to_local(wx, wy);
    get_or_create_chunk(cx, cy).set_cell(lx, ly, pixel);
    m_lattice.set(wx, wy, std::move(pixel));
}

void World::remove_settled(int wx, int wy) {
    auto [cx, cy] = world_to_chunk(wx, wy);
    auto [lx, ly] = world_to_local(wx, wy);
    if (Chunk* c = get_chunk(cx, cy)) {
        c->clear_cell(lx, ly);
    }
    m_lattice.remove(wx, wy);
}

bool World::has_settled(int wx, int wy) const {
    return m_lattice.has(wx, wy);
}

void World::add_dynamic(DynamicPixel pixel) {
    auto [cx, cy] = world_to_chunk(static_cast<int>(pixel.pos.x),
                                   static_cast<int>(pixel.pos.y));
    get_or_create_chunk(cx, cy).dynamic_pixels.push_back(std::move(pixel));
}

std::vector<DynamicPixel*> World::collect_all_dynamic() {
    std::vector<DynamicPixel*> out;
    for (auto& [key, chunk] : m_chunks) {
        for (auto& dp : chunk->dynamic_pixels) {
            out.push_back(&dp);
        }
    }
    return out;
}

std::pair<int,int> World::world_to_chunk(int wx, int wy) noexcept {
    // Floor-division so negative coords work correctly.
    auto fdiv = [](int a, int b) {
        return a / b - (a % b != 0 && (a ^ b) < 0);
    };
    return {fdiv(wx, CHUNK_SIZE), fdiv(wy, CHUNK_SIZE)};
}

std::pair<int,int> World::world_to_local(int wx, int wy) noexcept {
    int lx = ((wx % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
    int ly = ((wy % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
    return {lx, ly};
}

std::pair<int,int> World::chunk_to_world(int cx, int cy, int lx, int ly) noexcept {
    return {cx * CHUNK_SIZE + lx, cy * CHUNK_SIZE + ly};
}

uint64_t World::chunk_key(int cx, int cy) noexcept {
    return (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32) |
            static_cast<uint64_t>(static_cast<uint32_t>(cy));
}

} // namespace pf
