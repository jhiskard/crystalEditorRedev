#include "viewer_menu.h"

#include "viewer_panel.h"

#include "core/vtk/vtk_viewer.h"

#include <imgui.h>

namespace features::viewer {
namespace {

bool g_showViewerWindow = true;
bool g_initialized = false;
ViewerPanel g_viewerPanel;

} // namespace

void InitOnce(core::scene::SceneState& /*scene*/, core::vtk::MouseInteractor& /*mouseInteractor*/) {
    g_initialized = true;
    g_showViewerWindow = true;
}

void DrawMenus() {
    if (ImGui::BeginMenu("Settings")) {
        bool overlay = core::vtk::VtkViewer::Instance().PerformanceOverlayEnabled();
        if (ImGui::MenuItem("Viewer FPS Overlay", nullptr, overlay)) {
            core::vtk::VtkViewer::Instance().SetPerformanceOverlayEnabled(!overlay);
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Windows")) {
        ImGui::MenuItem("Viewer", nullptr, &g_showViewerWindow);
        ImGui::EndMenu();
    }
}

void HandleRequest(const Request& request) {
    switch (request.type) {
    case Request::Type::ShowViewer:
        g_showViewerWindow = true;
        break;
    default:
        break;
    }
}

void RenderWindows() {
    if (!g_initialized || !g_showViewerWindow) {
        return;
    }
    g_viewerPanel.Render(&g_showViewerWindow);
}

void Shutdown() {
    g_initialized = false;
    g_showViewerWindow = false;
}

} // namespace features::viewer
