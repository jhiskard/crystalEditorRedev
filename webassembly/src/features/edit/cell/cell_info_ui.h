#pragma once

#include <array>

#include <imgui.h>

namespace features::edit::cell {

class CellController;

class CellInfoUI {
public:
    explicit CellInfoUI(CellController& controller);

    void Render(bool* open);

private:
    void SyncMatrixBufferFromScene();
    void ApplyCellChangesOnEditEnd();
    void RenderCellMatrixTable();

    CellController& controller_;

    bool editMode_ = false;
    bool prevEditMode_ = false;
    std::array<std::array<float, 3>, 3> matrixBuffer_ = {{
        {{1.0f, 0.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, 1.0f}},
    }};
};

} // namespace features::edit::cell

