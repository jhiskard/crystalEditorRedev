#include "viewer_panel.h"

#include "core/vtk/vtk_viewer.h"
#include "features/measurement/measurement_menu.h"

#include <imgui.h>

#include <algorithm>

namespace features::viewer {

void ViewerPanel::Render(bool* open) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const bool visible = ImGui::Begin("Viewer", open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    if (!visible) {
        ImGui::End();
        return;
    }

    const ImVec2 contentMin = ImGui::GetCursorScreenPos();
    ImVec2 available = ImGui::GetContentRegionAvail();
    available.x = std::max(available.x, 16.0f);
    available.y = std::max(available.y, 16.0f);
    const ImVec2 contentMax(contentMin.x + available.x, contentMin.y + available.y);

    if (!core::vtk::VtkViewer::Instance().DrawRenderTexture(available, contentMin)) {
        ImGui::Dummy(available);
    }
    toolbar_.Render(contentMin, contentMax);
    features::measurement::RenderViewerOverlay(contentMin, contentMax);

    ImGui::End();
}

} // namespace features::viewer
