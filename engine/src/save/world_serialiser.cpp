#include <pixelforge/save/world_serialiser.hpp>
#include <pixelforge/core/logger.hpp>
#include <zstd.h>
#include <fstream>
#include <vector>
#include <cstring>

namespace pf {

namespace {

// Simple flat binary format:
// [4] magic "PFW1"
// [4] world width
// [4] world height
// [4] world seed
// [8] cell_count
// Per cell: [4] wx [4] wy [2] element [1] temperature [1] hp [4] color
// Then zstd-compressed.

constexpr uint32_t MAGIC = 0x31574650u; // "PFW1" LE

struct CellRecord {
    int32_t  wx, wy;
    uint16_t element;
    uint8_t  temperature, hp;
    uint32_t color;
};

struct Header {
    uint32_t magic;
    uint32_t width, height, seed;
    uint64_t cell_count;
};

} // anonymous namespace

std::expected<void, SaveError>
WorldSerialiser::save(const World& world, const char* path) {
    const auto& cfg = world.config();

    // Build raw buffer
    std::vector<CellRecord> cells;
    cells.reserve(world.lattice().size());

    AABB all{0, 0, cfg.width, cfg.height};
    for (auto [pos, sp] : world.lattice().query_rect(all)) {
        if (!sp) continue;
        cells.push_back({pos.x, pos.y,
                         sp->element, sp->temperature, sp->hp, sp->color});
    }

    Header hdr;
    hdr.magic      = MAGIC;
    hdr.width      = static_cast<uint32_t>(cfg.width);
    hdr.height     = static_cast<uint32_t>(cfg.height);
    hdr.seed       = cfg.seed;
    hdr.cell_count = cells.size();

    std::vector<uint8_t> raw(sizeof(Header) +
                              cells.size() * sizeof(CellRecord));
    std::memcpy(raw.data(), &hdr, sizeof(Header));
    if (!cells.empty()) {
        std::memcpy(raw.data() + sizeof(Header),
                    cells.data(),
                    cells.size() * sizeof(CellRecord));
    }

    // Compress
    const size_t cap     = ZSTD_compressBound(raw.size());
    std::vector<uint8_t> comp(cap);
    const size_t comp_sz = ZSTD_compress(comp.data(), cap,
                                          raw.data(), raw.size(), 3);
    if (ZSTD_isError(comp_sz)) {
        return std::unexpected(SaveError{
            std::string("zstd compress error: ") + ZSTD_getErrorName(comp_sz)});
    }
    comp.resize(comp_sz);

    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        return std::unexpected(SaveError{std::string("cannot open: ") + path});
    }
    ofs.write(reinterpret_cast<const char*>(comp.data()),
              static_cast<std::streamsize>(comp.size()));
    PF_LOG_INFO("WorldSerialiser: saved {} cells to '{}'", cells.size(), path);
    return {};
}

std::expected<World, SaveError>
WorldSerialiser::load(const char* path, ElementRegistry& registry) {
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) {
        return std::unexpected(SaveError{std::string("cannot open: ") + path});
    }
    const std::streamsize file_sz = ifs.tellg();
    ifs.seekg(0);
    std::vector<uint8_t> comp(static_cast<size_t>(file_sz));
    ifs.read(reinterpret_cast<char*>(comp.data()), file_sz);

    // Decompress
    const size_t raw_cap = ZSTD_getFrameContentSize(comp.data(), comp.size());
    if (raw_cap == ZSTD_CONTENTSIZE_ERROR || raw_cap == ZSTD_CONTENTSIZE_UNKNOWN) {
        return std::unexpected(SaveError{"invalid zstd frame"});
    }
    std::vector<uint8_t> raw(raw_cap);
    const size_t raw_sz = ZSTD_decompress(raw.data(), raw_cap,
                                           comp.data(), comp.size());
    if (ZSTD_isError(raw_sz)) {
        return std::unexpected(SaveError{
            std::string("zstd decompress error: ") + ZSTD_getErrorName(raw_sz)});
    }

    if (raw_sz < sizeof(Header)) {
        return std::unexpected(SaveError{"truncated header"});
    }
    Header hdr;
    std::memcpy(&hdr, raw.data(), sizeof(Header));
    if (hdr.magic != MAGIC) {
        return std::unexpected(SaveError{"bad magic"});
    }

    WorldConfig cfg;
    cfg.width  = static_cast<int>(hdr.width);
    cfg.height = static_cast<int>(hdr.height);
    cfg.seed   = hdr.seed;

    World world{cfg, registry};

    const CellRecord* cells = reinterpret_cast<const CellRecord*>(
        raw.data() + sizeof(Header));
    for (uint64_t i = 0; i < hdr.cell_count; ++i) {
        const CellRecord& r = cells[i];
        SettledPixel sp;
        sp.element     = r.element;
        sp.temperature = r.temperature;
        sp.hp          = r.hp;
        sp.color       = r.color;
        world.set_settled(r.wx, r.wy, sp);
    }

    PF_LOG_INFO("WorldSerialiser: loaded {} cells from '{}'",
                hdr.cell_count, path);
    return world;
}

} // namespace pf
