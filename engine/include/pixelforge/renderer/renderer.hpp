#pragma once
#include <cstdint>

namespace pf {

// Pure abstract renderer interface — no GL symbols here.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual bool init(void* sdl_window, int viewport_w, int viewport_h) = 0;
    virtual void shutdown() = 0;

    virtual void begin_frame() = 0;
    virtual void end_frame()   = 0;

    virtual void resize(int w, int h) = 0;
};

} // namespace pf
