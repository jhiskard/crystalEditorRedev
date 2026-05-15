#include "measurement_overlay_ui.h"

#include "measurement_controller.h"
#include "measurement_store.h"

#include "core/scene/scene_state.h"

#include <imgui.h>

#include <algorithm>
#include <string>

namespace features::measurement {
namespace {

const char* ModeOverlayLabel(MeasurementMode mode) {
    switch (mode) {
    case MeasurementMode::Distance:
        return "Distance Mode";
    case MeasurementMode::Angle:
        return "Angle Mode";
    case MeasurementMode::Dihedral:
        return "Dihedral Mode";
    case MeasurementMode::GeometricCenter:
        return "Geometric Center Mode";
    case MeasurementMode::CenterOfMass:
        return "Center of Mass Mode";
    case MeasurementMode::None:
    default:
        return "Measurement Mode";
    }
}

} // namespace

void MeasurementOverlayUI::Render(
    MeasurementController& controller,
    MeasurementStore& store,
    bool* showListWindow) {
    (void)controller;
    if (showListWindow != nullptr && *showListWindow) {
        RenderListWindow(store, showListWindow);
    }
}

void MeasurementOverlayUI::RenderModeOverlay(
    MeasurementController& controller,
    MeasurementStore& store,
    const ImVec2& viewerContentMin,
    const ImVec2& viewerContentMax) {
    if (!controller.IsActive()) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const float uiScale = std::max(1.0f, io.FontGlobalScale);
    const float overlayX = viewerContentMin.x + 12.0f * uiScale;
    const float toolbarTopPadding = 5.0f * uiScale;
    const float toolbarReservedHeight = 56.0f * uiScale;
    const float overlayY = viewerContentMin.y + toolbarTopPadding + toolbarReservedHeight;
    const float maxOverlayWidth = std::max(260.0f * uiScale, viewerContentMax.x - overlayX - 12.0f * uiScale);
    const float maxOverlayHeight = std::max(160.0f * uiScale, viewerContentMax.y - overlayY - 12.0f * uiScale);

    ImGui::SetNextWindowPos(ImVec2(overlayX, overlayY), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(maxOverlayWidth, maxOverlayHeight));

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoNav;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f * uiScale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f * uiScale, 10.0f * uiScale));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.35f));
    if (ImGui::Begin("##MeasurementModeOverlay", nullptr, flags)) {
        const MeasurementMode mode = controller.CurrentMode();
        const bool centerMode = IsCenterMode(mode);
        const size_t pickedCount = controller.PickedAtomIds().size();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(ModeOverlayLabel(mode));
        ImGui::SameLine();
        if (ImGui::Button("Exit")) {
            controller.ExitMode();
        }

        if (centerMode) {
            const bool canApply = pickedCount >= 2;
            ImGui::Spacing();
            ImGui::Text("Selected: %zu", pickedCount);

            bool applyTriggered = false;
            ImGui::BeginDisabled(!canApply);
            if (ImGui::Button("Apply")) {
                applyTriggered = true;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                controller.ClearPickedAtoms();
            }

            const bool typing = io.WantTextInput || ImGui::IsAnyItemActive();
            if (!typing && canApply &&
                (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
                 ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))) {
                applyTriggered = true;
            }
            if (applyTriggered) {
                controller.ApplyCenterMeasurement();
            }
        }

        if (bool* expanded = StylePanelExpanded(mode); expanded != nullptr) {
            ImGui::Spacing();
            if (ImGui::Button(*expanded ? "Preference (Collapse)" : "Preference (Expand)")) {
                *expanded = !(*expanded);
            }
            if (*expanded) {
                RenderStyleOptions(controller, store);
            }
        }

        const bool typing = io.WantTextInput || ImGui::IsAnyItemActive();
        if (!typing && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            controller.ExitMode();
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

bool* MeasurementOverlayUI::StylePanelExpanded(MeasurementMode mode) {
    switch (mode) {
    case MeasurementMode::Distance:
        return &distanceStylePanelExpanded_;
    case MeasurementMode::Angle:
        return &angleStylePanelExpanded_;
    case MeasurementMode::Dihedral:
        return &dihedralStylePanelExpanded_;
    case MeasurementMode::GeometricCenter:
        return &geometricCenterStylePanelExpanded_;
    case MeasurementMode::CenterOfMass:
        return &centerOfMassStylePanelExpanded_;
    case MeasurementMode::None:
    default:
        return nullptr;
    }
}

void MeasurementOverlayUI::RenderStyleOptions(MeasurementController& controller, MeasurementStore& store) {
    bool styleChanged = false;
    bool rebuildAngleGeometry = false;

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextUnformatted("Style");

    constexpr ImGuiColorEditFlags colorFlags = ImGuiColorEditFlags_NoInputs;
    const MeasurementMode mode = controller.CurrentMode();

    switch (mode) {
    case MeasurementMode::Distance: {
        DistanceStyle& style = store.DistanceStyleConfig();
        styleChanged |= ImGui::ColorEdit3("Line Color##DistanceStyle", style.lineColor.data(), colorFlags);
        styleChanged |= ImGui::SliderFloat(
            "Line Width (px)##DistanceStyle",
            &style.lineWidth,
            2.0f,
            18.0f,
            "%.1f");
        styleChanged |= ImGui::SliderFloat(
            "Value Label Size##DistanceStyle",
            &style.labelScale,
            1.0f,
            3.0f,
            "%.1fx");
        if (ImGui::Button("Reset Style##DistanceStyle")) {
            store.ResetStyle(mode);
            styleChanged = true;
        }
        break;
    }
    case MeasurementMode::Angle: {
        AngleStyle& style = store.AngleStyleConfig();
        styleChanged |= ImGui::ColorEdit3("Line(1,2) Color##AngleStyle", style.lineColor.data(), colorFlags);
        styleChanged |= ImGui::SliderFloat(
            "Line Width (px)##AngleStyle",
            &style.lineWidth,
            2.0f,
            18.0f,
            "%.1f");
        if (ImGui::SliderFloat(
                "Arc Radius Scale##AngleStyle",
                &style.arcRadiusScale,
                0.6f,
                2.0f,
                "%.1fx")) {
            styleChanged = true;
            rebuildAngleGeometry = true;
        }
        styleChanged |= ImGui::SliderFloat(
            "Value Label Size##AngleStyle",
            &style.labelScale,
            1.0f,
            3.0f,
            "%.1fx");
        if (ImGui::Button("Reset Style##AngleStyle")) {
            store.ResetStyle(mode);
            styleChanged = true;
            rebuildAngleGeometry = true;
        }
        break;
    }
    case MeasurementMode::Dihedral: {
        DihedralStyle& style = store.DihedralStyleConfig();
        styleChanged |= ImGui::ColorEdit3(
            "Base Line Color##DihedralStyle",
            style.baseLineColor.data(),
            colorFlags);
        styleChanged |= ImGui::SliderFloat(
            "Base Line Width (px)##DihedralStyle",
            &style.baseLineWidth,
            2.0f,
            18.0f,
            "%.1f");
        styleChanged |= ImGui::ColorEdit3(
            "Helper Plane 1 Color##DihedralStyle",
            style.helperPlane1Color.data(),
            colorFlags);
        styleChanged |= ImGui::SliderFloat(
            "Helper Plane 1 Opacity##DihedralStyle",
            &style.helperPlane1Opacity,
            0.05f,
            0.60f,
            "%.2f");
        styleChanged |= ImGui::ColorEdit3(
            "Helper Plane 2 Color##DihedralStyle",
            style.helperPlane2Color.data(),
            colorFlags);
        styleChanged |= ImGui::SliderFloat(
            "Helper Plane 2 Opacity##DihedralStyle",
            &style.helperPlane2Opacity,
            0.05f,
            0.60f,
            "%.2f");
        styleChanged |= ImGui::SliderFloat(
            "Value Label Size##DihedralStyle",
            &style.labelScale,
            1.0f,
            3.0f,
            "%.1fx");
        if (ImGui::Button("Reset Style##DihedralStyle")) {
            store.ResetStyle(mode);
            styleChanged = true;
        }
        break;
    }
    case MeasurementMode::GeometricCenter: {
        CenterStyle& style = store.GeometricCenterStyleConfig();
        styleChanged |= ImGui::ColorEdit3(
            "Center + Color##GeometricCenterStyle",
            style.centerPlusColor.data(),
            colorFlags);
        styleChanged |= ImGui::SliderFloat(
            "Center + Size##GeometricCenterStyle",
            &style.centerPlusScale,
            1.0f,
            3.0f,
            "%.1fx");
        styleChanged |= ImGui::ColorEdit3(
            "Selected + Color##GeometricCenterStyle",
            style.selectedPlusColor.data(),
            colorFlags);
        styleChanged |= ImGui::SliderFloat(
            "Selected + Size##GeometricCenterStyle",
            &style.selectedPlusScale,
            1.0f,
            3.0f,
            "%.1fx");
        styleChanged |= ImGui::SliderFloat(
            "Coordinate Text Size##GeometricCenterStyle",
            &style.coordinateLabelScale,
            1.0f,
            3.0f,
            "%.1fx");
        if (ImGui::Button("Reset Style##GeometricCenterStyle")) {
            store.ResetStyle(mode);
            styleChanged = true;
        }
        break;
    }
    case MeasurementMode::CenterOfMass: {
        CenterStyle& style = store.CenterOfMassStyleConfig();
        styleChanged |= ImGui::ColorEdit3(
            "Center + Color##CenterOfMassStyle",
            style.centerPlusColor.data(),
            colorFlags);
        styleChanged |= ImGui::SliderFloat(
            "Center + Size##CenterOfMassStyle",
            &style.centerPlusScale,
            1.0f,
            3.0f,
            "%.1fx");
        styleChanged |= ImGui::ColorEdit3(
            "Selected + Color##CenterOfMassStyle",
            style.selectedPlusColor.data(),
            colorFlags);
        styleChanged |= ImGui::SliderFloat(
            "Selected + Size##CenterOfMassStyle",
            &style.selectedPlusScale,
            1.0f,
            3.0f,
            "%.1fx");
        styleChanged |= ImGui::SliderFloat(
            "Coordinate Text Size##CenterOfMassStyle",
            &style.coordinateLabelScale,
            1.0f,
            3.0f,
            "%.1fx");
        if (ImGui::Button("Reset Style##CenterOfMassStyle")) {
            store.ResetStyle(mode);
            styleChanged = true;
        }
        break;
    }
    case MeasurementMode::None:
    default:
        break;
    }

    if (styleChanged) {
        store.ApplyStyleForMode(mode, rebuildAngleGeometry);
    }
}

void MeasurementOverlayUI::RenderListWindow(MeasurementStore& store, bool* showListWindow) {
    if (!ImGui::Begin("Measurement", showListWindow)) {
        ImGui::End();
        return;
    }

    const auto items = store.MeasurementsForStructure(-1);
    if (items.empty()) {
        ImGui::TextUnformatted("No measurements for the active structure.");
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear Active Structure")) {
        if (!items.empty()) {
            store.RemoveByStructure(items.front().structureId);
            ImGui::End();
            return;
        }
    }
    ImGui::Separator();

    if (ImGui::BeginTable("MeasurementList", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Visible", ImGuiTableColumnFlags_WidthFixed, 64.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Remove", ImGuiTableColumnFlags_WidthFixed, 72.0f);
        ImGui::TableHeadersRow();

        for (const MeasurementListItem& item : items) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            bool visible = item.visible;
            const std::string visibleId = "##visible_" + std::to_string(item.id);
            if (ImGui::Checkbox(visibleId.c_str(), &visible)) {
                store.SetVisible(item.id, visible);
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(TypeLabel(item.type));

            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(item.displayName.c_str());

            ImGui::TableSetColumnIndex(3);
            const std::string removeId = "Remove##measurement_" + std::to_string(item.id);
            if (ImGui::SmallButton(removeId.c_str())) {
                store.Remove(item.id);
            }
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

} // namespace features::measurement
