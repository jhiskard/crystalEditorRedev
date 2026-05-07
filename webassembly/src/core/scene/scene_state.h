#pragma once

#include "events.h"
#include "hover.h"
#include "selection.h"
#include "structure_registry.h"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace core::scene {

struct AtomRecord {
    uint32_t id = 0;
    std::string symbol;
    float radius = 1.0f;
    std::array<float, 3> cartesian = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> fractional = {0.0f, 0.0f, 0.0f};
};

struct UnitCellRecord {
    bool hasCell = false;
    std::array<std::array<float, 3>, 3> matrix = {{
        {{1.0f, 0.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, 1.0f}},
    }};
};

struct StructureRecord {
    UnitCellRecord cell;
    std::vector<AtomRecord> atoms;
};

struct SceneState {
    SceneState();

    int32_t currentStructureId = -1;
    StructureRegistry structures;
    std::unordered_map<int32_t, StructureRecord> structureRecords;
    uint32_t nextAtomId = 1;
    SelectionSet selection;
    HoverInfo hover;
    EventBus events;
};

} // namespace core::scene
