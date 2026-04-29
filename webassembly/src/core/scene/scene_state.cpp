#include "scene_state.h"

namespace core::scene {

SceneState::SceneState()
    : structures(&events) {
    selection.SetEventBus(&events);
}

} // namespace core::scene
