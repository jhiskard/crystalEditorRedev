#include "bz_plot_ui.h"

#include "bz_plot_controller.h"
#include "special_points.h"

#include <imgui.h>

namespace features::utilities::bz {

BZPlotUI::BZPlotUI(BZPlotController& controller)
    : controller_(controller) {
}

void BZPlotUI::Render(bool* open) {
    if (open != nullptr && !(*open)) {
        return;
    }

    if (!ImGui::Begin("Brillouin Zone Plot", open)) {
        ImGui::End();
        return;
    }

    RenderBandpathConfig();
    ImGui::Spacing();
    RenderOptions();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    RenderActionButtons();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    RenderStatus();
    ImGui::Spacing();
    RenderSpecialPointsTable();

    ImGui::End();
}

void BZPlotUI::RenderBandpathConfig() {
    ImGui::TextUnformatted("Bandpath Configuration:");
    ImGui::Spacing();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Path:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputText("##bz-path", pathInput_, IM_ARRAYSIZE(pathInput_));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Examples:\n"
            "  All (automatic path by lattice type)\n"
            "  GXMGRX (cubic)\n"
            "  TEST (hardcoded cubic test cell)");
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Npoints:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::InputInt("##bz-npoints", &npointsInput_)) {
        if (npointsInput_ < 10) {
            npointsInput_ = 10;
        }
        if (npointsInput_ > 500) {
            npointsInput_ = 500;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Interpolation points per segment (10-500)");
    }
}

void BZPlotUI::RenderOptions() {
    ImGui::TextUnformatted("Options:");
    ImGui::Spacing();

    const bool prevShowVectors = showVectors_;
    const bool prevShowLabels = showLabels_;

    ImGui::Checkbox("Show reciprocal vectors", &showVectors_);
    ImGui::Checkbox("Show special point labels", &showLabels_);

    const bool optionsChanged = (prevShowVectors != showVectors_) || (prevShowLabels != showLabels_);
    if (optionsChanged && controller_.IsShowing()) {
        controller_.Show(pathInput_, npointsInput_, showVectors_, showLabels_, lastErrorMessage_);
    }
}

void BZPlotUI::RenderActionButtons() {
    const bool showing = controller_.IsShowing();
    const char* buttonText = showing ? "Show Crystal" : "Show BZ Plot";

    if (ImGui::Button(buttonText, ImVec2(170.0f, 0.0f))) {
        lastErrorMessage_.clear();

        if (!showing) {
            controller_.Show(pathInput_, npointsInput_, showVectors_, showLabels_, lastErrorMessage_);
        } else {
            controller_.Clear();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear BZ", ImVec2(170.0f, 0.0f))) {
        controller_.Clear();
        lastErrorMessage_.clear();
    }

    ImGui::SameLine();
    if (ImGui::Button("Show Test BZ", ImVec2(170.0f, 0.0f))) {
        lastErrorMessage_.clear();
        controller_.Show("TEST", npointsInput_, showVectors_, showLabels_, lastErrorMessage_);
    }
}

void BZPlotUI::RenderStatus() {
    if (controller_.IsShowing()) {
        ImGui::TextColored(ImVec4(0.0f, 0.6f, 1.0f, 1.0f), "BZ Plot Mode");
    } else {
        ImGui::TextColored(ImVec4(0.1f, 0.9f, 0.2f, 1.0f), "Crystal Mode");
    }

    if (!lastErrorMessage_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextWrapped("%s", lastErrorMessage_.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::TextDisabled("Note: structure cell wiring arrives in Phase 3.4.");
}

void BZPlotUI::RenderSpecialPointsTable() {
    CellInfo cellInfo;
    if (!controller_.TryGetCurrentCellInfo(cellInfo) || !cellInfo.valid) {
        ImGui::TextDisabled("Cell info will be wired in Phase 3.4.");
        return;
    }

    double matrix[3][3] = {
        {cellInfo.matrix[0][0], cellInfo.matrix[0][1], cellInfo.matrix[0][2]},
        {cellInfo.matrix[1][0], cellInfo.matrix[1][1], cellInfo.matrix[1][2]},
        {cellInfo.matrix[2][0], cellInfo.matrix[2][1], cellInfo.matrix[2][2]}};

    const std::string latticeType = SpecialPointsDatabase::detectLatticeType(matrix);
    const auto specialPoints = SpecialPointsDatabase::getSpecialPoints(latticeType);

    ImGui::Text("Special k-points (%s)", latticeType.c_str());
    if (specialPoints.empty()) {
        ImGui::TextDisabled("No points available.");
        return;
    }

    if (ImGui::BeginTable("BZSpecialPoints", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Label");
        ImGui::TableSetupColumn("a");
        ImGui::TableSetupColumn("b");
        ImGui::TableSetupColumn("c");
        ImGui::TableHeadersRow();

        for (const auto& kv : specialPoints) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", kv.first.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.6f", kv.second[0]);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.6f", kv.second[1]);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.6f", kv.second[2]);
        }

        ImGui::EndTable();
    }
}

} // namespace features::utilities::bz
