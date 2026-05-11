/**
 * @file features/edit/atoms/surrounding_atom_manager.h
 * @brief PBC image atom generator for boundary visualization.
 */
#pragma once

#include "core/scene/scene_state.h"
#include "../cell/cell_manager.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace features::edit::atoms {

class SurroundingAtomManager {
public:
    SurroundingAtomManager(core::scene::SceneState& scene, cell::CellManager& cellManager);

    void Recompute(int32_t structureId);
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return enabled_; }

private:
    void ClearGenerated(int32_t structureId);

    core::scene::SceneState& scene_;
    cell::CellManager& cellManager_;
    bool enabled_ = false;
    std::unordered_map<int32_t, std::vector<uint32_t>> generatedAtomIdsByStructure_;
};

} // namespace features::edit::atoms

