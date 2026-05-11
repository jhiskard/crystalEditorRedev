/**
 * @file features/edit/atoms/atom_editor_ui.h
 * @brief Created Atoms editor window for Edit/Atoms.
 */
#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <imgui.h>

namespace features::edit::atoms {

class AtomsController;

class AtomEditorUI {
public:
    explicit AtomEditorUI(AtomsController& controller);

    void Render(bool* open);

private:
    struct RowView {
        size_t index = 0;
    };

    void RenderToolbar();
    void RenderSelectionTools();
    void RenderTransformTools();
    void RenderRadiusTools();
    void RenderTable();
    void RefreshRows();
    void SortRows();

    AtomsController& controller_;

    bool editMode_ = false;
    bool useFractionalCoords_ = false;
    bool boundaryAtomsEnabled_ = false;

    int sortColumn_ = 0;
    bool sortAscending_ = true;
    ImGuiTableSortSpecs* sortSpecs_ = nullptr;

    std::array<float, 3> translateDelta_ = {0.0f, 0.0f, 0.0f};
    float selectedRadius_ = 1.0f;

    std::vector<RowView> rows_;
};

} // namespace features::edit::atoms
