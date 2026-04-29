#pragma once

#include <cstdint>

namespace core::scene {

struct HoverInfo {
    uint32_t atomId = UINT32_MAX;
    int32_t structureId = -1;
    double pickPos[3] = {0.0, 0.0, 0.0};
    bool hasHover = false;
};

} // namespace core::scene
