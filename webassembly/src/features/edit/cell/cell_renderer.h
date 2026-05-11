/**
 * @file features/edit/cell/cell_renderer.h
 * @brief Unit cell actor renderer split from legacy vtk_renderer.
 */
#pragma once

#include "core/scene/scene_state.h"

#include <vtkSmartPointer.h>

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

class vtkActor;

namespace features::edit::cell {

class CellRenderer {
public:
    explicit CellRenderer(core::scene::SceneState& scene);
    ~CellRenderer();

    /// @brief Connects to SceneState EventBus::onCellChanged.
    void Subscribe();

    void CreateUnitCell(const std::array<std::array<float, 3>, 3>& matrix);
    void CreateUnitCell(int32_t structureId, const std::array<std::array<float, 3>, 3>& matrix);
    void ClearUnitCell();
    void ClearUnitCell(int32_t structureId);
    void SetUnitCellVisible(bool visible);
    void SetUnitCellVisible(int32_t structureId, bool visible);
    bool HasUnitCell(int32_t structureId) const;
    bool IsUnitCellVisible(int32_t structureId) const;

private:
    void OnCellChanged(int32_t structureId);
    void ClearAllUnitCells();

    core::scene::SceneState& scene_;
    std::unordered_map<int32_t, std::vector<vtkSmartPointer<vtkActor>>> cellEdgeActorsByStructure_;
    std::unordered_map<int32_t, bool> unitCellVisibleByStructure_;
    bool unitCellGlobalHidden_ = false;
    bool subscribed_ = false;
};

} // namespace features::edit::cell
