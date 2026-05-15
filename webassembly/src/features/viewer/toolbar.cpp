#include "toolbar.h"

#include "core/vtk/vtk_viewer.h"
#include "features/data/data_menu.h"
#include "features/edit/edit_menu.h"

#include <algorithm>
#include <cmath>

namespace features::viewer {
namespace {

void tooltip(const char* text) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("%s", text);
    }
}

const char* meshModeLabel(MeshDisplayMode mode) {
    switch (mode) {
    case MeshDisplayMode::Solid:
        return "Solid";
    case MeshDisplayMode::Wireframe:
        return "Wire";
    case MeshDisplayMode::SurfaceWithEdges:
        return "Edges";
    default:
        return "Mesh";
    }
}

} // namespace

void Toolbar::Render(const ImVec2& viewerContentMin, const ImVec2& viewerContentMax) {
    hovered_ = false;

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::SetNextWindowBgAlpha(0.78f);
    ImGui::SetNextWindowPos(ImVec2(viewerContentMin.x + 12.0f, viewerContentMin.y + 12.0f), ImGuiCond_Always);
    if (ImGui::Begin("##ViewerToolbar", nullptr, flags)) {
        renderBoundaryAtomsButton();
        ImGui::SameLine();
        renderMeshDisplayModeButton();
        ImGui::SameLine();
        renderProjectionButton();
        ImGui::SameLine();
        renderResetViewButton();
        ImGui::SameLine();
        renderCellAlignButton();
        ImGui::SameLine();
        renderChargeDensityControls();
        hovered_ = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    }
    ImGui::End();

    constexpr float stepWidth = 252.0f;
    constexpr float stepHeight = 46.0f;
    const ImVec2 stepPos(
        viewerContentMin.x + (viewerContentMax.x - viewerContentMin.x - stepWidth) * 0.5f,
        viewerContentMax.y - stepHeight - 12.0f);
    ImGui::SetNextWindowBgAlpha(0.78f);
    ImGui::SetNextWindowPos(stepPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(stepWidth, stepHeight), ImGuiCond_Always);
    if (ImGui::Begin("##ViewerArrowStepToolbar", nullptr, flags)) {
        renderArrowStepControl();
        hovered_ = hovered_ || ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    }
    ImGui::End();
}

void Toolbar::renderBoundaryAtomsButton() {
    const bool enabled = features::edit::IsBoundaryAtomsEnabled();
    if (ImGui::Button(enabled ? "Boundary On" : "Boundary")) {
        features::edit::SetBoundaryAtomsEnabled(!enabled);
    }
    tooltip("Toggle boundary atoms");
}

void Toolbar::renderMeshDisplayModeButton() {
    if (ImGui::Button(meshModeLabel(meshDisplayMode_))) {
        ImGui::OpenPopup("##MeshDisplayModePopup");
    }
    tooltip("Mesh display mode");

    if (ImGui::BeginPopup("##MeshDisplayModePopup")) {
        if (ImGui::MenuItem("Solid", nullptr, meshDisplayMode_ == MeshDisplayMode::Solid)) {
            meshDisplayMode_ = MeshDisplayMode::Solid;
        }
        if (ImGui::MenuItem("Wireframe", nullptr, meshDisplayMode_ == MeshDisplayMode::Wireframe)) {
            meshDisplayMode_ = MeshDisplayMode::Wireframe;
        }
        if (ImGui::MenuItem("Surface with Edges", nullptr, meshDisplayMode_ == MeshDisplayMode::SurfaceWithEdges)) {
            meshDisplayMode_ = MeshDisplayMode::SurfaceWithEdges;
        }
        ImGui::TextDisabled("Deferred");
        ImGui::EndPopup();
    }
}

void Toolbar::renderProjectionButton() {
    core::vtk::VtkViewer& viewer = core::vtk::VtkViewer::Instance();
    const bool parallel = viewer.GetProjectionMode() == core::vtk::ProjectionMode::Parallel;
    if (ImGui::Button(parallel ? "Parallel" : "Perspective")) {
        viewer.SetProjectionMode(parallel ? core::vtk::ProjectionMode::Perspective : core::vtk::ProjectionMode::Parallel);
    }
    tooltip("Toggle projection mode");
}

void Toolbar::renderResetViewButton() {
    if (ImGui::Button("Reset")) {
        core::vtk::VtkViewer::Instance().ResetView();
    }
    tooltip("Reset view");
}

void Toolbar::renderCellAlignButton() {
    if (ImGui::Button("Cell")) {
        ImGui::OpenPopup("##CellAlignPopup");
    }
    tooltip("Align camera to cell axis");

    if (ImGui::BeginPopup("##CellAlignPopup")) {
        if (ImGui::MenuItem("+a axis")) {
            features::edit::AlignCameraToCurrentCellAxis(0);
        }
        if (ImGui::MenuItem("+b axis")) {
            features::edit::AlignCameraToCurrentCellAxis(1);
        }
        if (ImGui::MenuItem("+c axis")) {
            features::edit::AlignCameraToCurrentCellAxis(2);
        }
        ImGui::EndPopup();
    }
}

void Toolbar::renderChargeDensityControls() {
    const bool hasChargeDensity = features::data::HasChargeDensity();
    ImGui::BeginDisabled(!hasChargeDensity);
    if (ImGui::Button(features::data::IsQuickAnimationActive() ? "Stop CD" : "Charge")) {
        if (features::data::IsQuickAnimationActive()) {
            features::data::StopQuickAnimation();
        } else {
            features::data::StartQuickAnimation();
        }
    }
    tooltip(hasChargeDensity ? features::data::GetActiveChargeDensityName().c_str() : "No charge density loaded");
    ImGui::EndDisabled();

    if (!hasChargeDensity) {
        return;
    }

    if (features::data::IsQuickAnimationActive()) {
        const float phase = static_cast<float>(ImGui::GetTime());
        chargeDensityLevelPercent_ = 50.0f + 45.0f * std::sin(phase * 1.5f);
        features::data::SetChargeDensityLevelPercent(chargeDensityLevelPercent_);
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(96.0f);
    if (ImGui::SliderFloat("##ChargeLevel", &chargeDensityLevelPercent_, 0.0f, 100.0f, "%.0f%%")) {
        chargeDensityLevelPercent_ = std::clamp(chargeDensityLevelPercent_, 0.0f, 100.0f);
        features::data::SetChargeDensityLevelPercent(chargeDensityLevelPercent_);
    }
    tooltip("Charge density level");
}

void Toolbar::renderArrowStepControl() {
    core::vtk::VtkViewer& viewer = core::vtk::VtkViewer::Instance();
    float step = viewer.GetArrowRotateStepDeg();
    ImGui::SetNextItemWidth(148.0f);
    if (ImGui::SliderFloat("Arrow Step", &step, 1.0f, 180.0f, "%.0f deg")) {
        viewer.SetArrowRotateStepDeg(step);
    }
    tooltip("Keyboard arrow rotation step");
}

} // namespace features::viewer
