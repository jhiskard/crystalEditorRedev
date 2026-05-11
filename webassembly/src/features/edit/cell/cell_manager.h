/**
 * @file features/edit/cell/cell_manager.h
 * @brief Unit cell matrix and parameter conversion helpers for Edit/Cell.
 */
#pragma once

#include "core/scene/scene_state.h"

#include <array>
#include <cstdint>

namespace features::edit::cell {

class CellManager {
public:
    explicit CellManager(core::scene::SceneState& scene);

    void SetMatrix(int32_t structureId, const std::array<std::array<float, 3>, 3>& matrix);
    void SetParameters(
        int32_t structureId,
        float a,
        float b,
        float c,
        float alpha,
        float beta,
        float gamma);

    std::array<std::array<float, 3>, 3> GetMatrix(int32_t structureId) const;
    void GetParameters(
        int32_t structureId,
        float& a,
        float& b,
        float& c,
        float& alpha,
        float& beta,
        float& gamma) const;

    float Volume(int32_t structureId) const;

    /// @brief Placeholder axis-alignment entrypoint for Phase 3.7 toolbar.
    void AlignAxis(int32_t structureId, int axis);

private:
    int32_t ResolveStructureId(int32_t structureId, bool createIfMissing);
    int32_t ResolveStructureIdConst(int32_t structureId) const;

    core::scene::SceneState& scene_;
};

} // namespace features::edit::cell

