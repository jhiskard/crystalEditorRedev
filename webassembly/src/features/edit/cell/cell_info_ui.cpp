#include "cell_info_ui.h"

#include "cell_controller.h"

#include <cstdio>

namespace features::edit::cell {

CellInfoUI::CellInfoUI(CellController& controller)
    : controller_(controller) {
}

void CellInfoUI::Render(bool* open) {
    if (open != nullptr && !(*open)) {
        return;
    }

    if (!ImGui::Begin("Cell Information", open)) {
        ImGui::End();
        return;
    }

    if (!editMode_) {
        SyncMatrixBufferFromScene();
    }

    ImGui::Checkbox("Edit mode##cellEdit", &editMode_);

    if (!prevEditMode_ && editMode_) {
        SyncMatrixBufferFromScene();
    }

    if (prevEditMode_ && !editMode_) {
        ApplyCellChangesOnEditEnd();
    }
    prevEditMode_ = editMode_;

    RenderCellMatrixTable();

    if (editMode_) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "[Edit Mode Active]");
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 1.0f, 1.0f), "Uncheck 'Edit mode' to apply cell changes");
        ImGui::TextColored(
            ImVec4(0.8f, 0.8f, 0.2f, 1.0f),
            "Changes will be batched for optimal performance");
    }

    ImGui::End();
}

void CellInfoUI::SyncMatrixBufferFromScene() {
    matrixBuffer_ = controller_.GetActiveMatrix();
}

void CellInfoUI::ApplyCellChangesOnEditEnd() {
    controller_.ApplyMatrix(matrixBuffer_, true);
}

void CellInfoUI::RenderCellMatrixTable() {
    constexpr ImGuiTableFlags kTableFlags =
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
    constexpr ImGuiTableColumnFlags kColumnFlags = ImGuiTableColumnFlags_WidthStretch;

    if (!ImGui::BeginTable("CellMatrix", 4, kTableFlags)) {
        return;
    }

    ImGui::TableSetupColumn("", kColumnFlags, 50.0f);
    ImGui::TableSetupColumn("x", kColumnFlags, 100.0f);
    ImGui::TableSetupColumn("y", kColumnFlags, 100.0f);
    ImGui::TableSetupColumn("z", kColumnFlags, 100.0f);
    ImGui::TableHeadersRow();

    static const char* kRowLabels[] = {"v1", "v2", "v3"};
    for (int row = 0; row < 3; ++row) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", kRowLabels[row]);

        for (int col = 0; col < 3; ++col) {
            ImGui::TableSetColumnIndex(col + 1);

            if (editMode_) {
                char id[32];
                std::snprintf(id, sizeof(id), "##cell_%d_%d", row, col);
                ImGui::InputFloat(id, &matrixBuffer_[row][col], 0.0f, 0.0f, "%.6f");
            } else {
                ImGui::Text("%.6f", matrixBuffer_[row][col]);
            }
        }
    }

    ImGui::EndTable();
}

} // namespace features::edit::cell

