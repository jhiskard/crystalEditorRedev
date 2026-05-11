#pragma once

#include "cell_manager.h"
#include "cell_renderer.h"

#include "core/scene/scene_state.h"

#include <array>
#include <cstdint>

namespace features::edit::cell {

class CellController {
public:
    explicit CellController(core::scene::SceneState& scene);

    void Subscribe();

    int32_t ActiveStructureId() const;
    int32_t EnsureActiveStructure();
    bool HasActiveUnitCell() const;

    std::array<std::array<float, 3>, 3> GetActiveMatrix() const;
    void ApplyMatrix(const std::array<std::array<float, 3>, 3>& matrix, bool preserveCartesian = true);

    void SetActiveMatrixParameters(
        float a,
        float b,
        float c,
        float alpha,
        float beta,
        float gamma,
        bool preserveCartesian = true);
    void GetActiveMatrixParameters(
        float& a,
        float& b,
        float& c,
        float& alpha,
        float& beta,
        float& gamma) const;

    float ActiveCellVolume() const;

    void AlignAxis(int axis);

    void SetUnitCellVisible(bool visible);
    bool IsUnitCellVisible() const;
    bool HasUnitCell() const;

    CellManager& Manager() { return manager_; }
    CellRenderer& Renderer() { return renderer_; }

private:
    core::scene::SceneState& scene_;
    CellManager manager_;
    CellRenderer renderer_;
};

} // namespace features::edit::cell

