#pragma once

#include "events.h"
#include "hover.h"
#include "selection.h"
#include "structure_registry.h"

#include <cstdint>

namespace core::scene {

struct SceneState {
    SceneState();

    int32_t currentStructureId = -1;
    StructureRegistry structures;
    SelectionSet selection;
    HoverInfo hover;
    EventBus events;
};

} // namespace core::scene
