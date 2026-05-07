#include "slice_ui.h"

#include "slice_controller.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <string>

namespace features::data::slice {

SliceUI::SliceUI(SliceController& controller)
    : controller_(controller) {
    hadData_ = controller_.HasData();
    SyncFromController();
}

void SliceUI::Render(bool* open) {
    if (open != nullptr && !(*open)) {
        return;
    }

    if (!ImGui::Begin("Slice Viewer", open)) {
        ImGui::End();
        return;
    }

    if (controller_.HasData() != hadData_) {
        hadData_ = controller_.HasData();
        SyncFromController();
    }
    SyncFromController();

    if (ImGui::Button("Show Plane")) {
        controller_.Show();
    }
    ImGui::SameLine();
    if (ImGui::Button("Hide Plane")) {
        controller_.Hide();
    }

    ImGui::InputText("CHGCAR Path", filePathInput_, sizeof(filePathInput_));
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        if (controller_.LoadFromFile(filePathInput_)) {
            controller_.Show();
            SyncFromController();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        controller_.ClearData();
        SyncFromController();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Sample Data")) {
        if (controller_.LoadSampleData()) {
            controller_.Show();
            SyncFromController();
        }
    }

    static const char* kPlaneItems[] = {
        "XY (Z=const)",
        "XZ (Y=const)",
        "YZ (X=const)",
        "Define plane by Miller indices"
    };
    if (ImGui::Combo("Plane", &planeIndex_, kPlaneItems, 4)) {
        controller_.SetPlane(static_cast<SlicePlane>(planeIndex_));
    }

    const bool isMillerMode = (planeIndex_ == static_cast<int>(SlicePlane::Miller));
    if (isMillerMode) {
        bool millerChanged = false;
        millerChanged |= ImGui::SliderInt("h", &millerH_, 0, 6);
        millerChanged |= ImGui::SliderInt("k", &millerK_, 0, 6);
        millerChanged |= ImGui::SliderInt("l", &millerL_, 0, 6);
        if (millerChanged) {
            controller_.SetMillerIndices(millerH_, millerK_, millerL_);
        }
        if (millerH_ == 0 && millerK_ == 0 && millerL_ == 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.35f, 1.0f), "h, k, l cannot all be 0.");
        }
    }

    const char* axisLabels[] = {"Z Position", "Y Position", "X Position"};
    const char* positionLabel = isMillerMode ? "Position" : axisLabels[std::clamp(planeIndex_, 0, 2)];
    if (ImGui::SliderFloat(positionLabel, &position_, 0.0f, 1.0f, "%.3f")) {
        controller_.SetPosition(position_);
    }

    const std::array<int, 3> grid = controller_.GridShape();
    if (isMillerMode) {
        const int maxDim = std::max({grid[0], grid[1], grid[2]});
        const int maxIndex = std::max(0, maxDim - 1);
        int gridIndex = static_cast<int>(std::round(position_ * static_cast<float>(maxIndex)));
        gridIndex = std::clamp(gridIndex, 0, maxIndex);
        if (ImGui::SliderInt("Grid Index", &gridIndex, 0, maxIndex)) {
            position_ = (maxIndex > 0)
                ? static_cast<float>(gridIndex) / static_cast<float>(maxIndex)
                : 0.0f;
            controller_.SetPosition(position_);
        }
    } else {
        int maxIndex = 0;
        if (planeIndex_ == 0) {
            maxIndex = std::max(0, grid[2] - 1);
        } else if (planeIndex_ == 1) {
            maxIndex = std::max(0, grid[1] - 1);
        } else {
            maxIndex = std::max(0, grid[0] - 1);
        }
        int gridIndex = static_cast<int>(std::round(position_ * static_cast<float>(maxIndex)));
        gridIndex = std::clamp(gridIndex, 0, maxIndex);
        if (ImGui::SliderInt("Grid Index", &gridIndex, 0, maxIndex)) {
            position_ = (maxIndex > 0)
                ? static_cast<float>(gridIndex) / static_cast<float>(maxIndex)
                : 0.0f;
            controller_.SetPosition(position_);
        }
    }

    ImGui::TextDisabled("Colormap, Color Curve, and Level/Window follow Surface/Volumetric shared settings.");

    ImGui::Text("Grid: %d x %d x %d", grid[0], grid[1], grid[2]);
    if (!controller_.HasData()) {
        ImGui::TextDisabled("No charge density data loaded.");
    }

    const std::string status = controller_.StatusMessage();
    if (!status.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", status.c_str());
    }

    ImGui::End();
}

void SliceUI::SyncFromController() {
    planeIndex_ = static_cast<int>(controller_.Plane());
    position_ = controller_.Position();
    millerH_ = controller_.MillerH();
    millerK_ = controller_.MillerK();
    millerL_ = controller_.MillerL();
}

} // namespace features::data::slice
