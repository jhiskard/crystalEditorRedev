#include "bz_menu.h"

#include "bz_plot_controller.h"
#include "bz_plot_ui.h"

#include "core/scene/scene_state.h"

#include <imgui.h>

namespace features::utilities::bz {

namespace {

bool g_showWindow = false;
BZPlotController* g_controller = nullptr;
BZPlotUI* g_ui = nullptr;

} // namespace

void InitOnce(core::scene::SceneState& scene) {
    if (g_controller != nullptr) {
        return;
    }

    static BZPlotController controller(scene);
    static BZPlotUI ui(controller);

    g_controller = &controller;
    g_ui = &ui;
}

void DrawMenu() {
    if (ImGui::BeginMenu("Utilities")) {
        if (ImGui::MenuItem("Brillouin Zone")) {
            g_showWindow = true;
        }
        ImGui::EndMenu();
    }
}

void HandleRequest(const Request& request) {
    if (request.type == Request::Type::Show) {
        g_showWindow = true;
    }
}

void RenderWindows() {
    if (!g_showWindow || g_ui == nullptr) {
        return;
    }

    g_ui->Render(&g_showWindow);
}

void Tick(float /*dt*/) {
}

void Shutdown() {
    if (g_controller != nullptr) {
        g_controller->Clear();
    }
}

BZPlotController& Controller() {
    return *g_controller;
}

} // namespace features::utilities::bz
