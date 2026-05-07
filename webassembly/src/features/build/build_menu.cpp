#include "build_menu.h"

#include "bravais/bravais_controller.h"
#include "bravais/bravais_lattice_ui.h"
#include "periodic_table/periodic_table_controller.h"
#include "periodic_table/periodic_table_ui.h"

#include "core/scene/scene_state.h"

#include <imgui.h>

namespace features::build {
namespace {

bool g_showPeriodicTableWindow = false;
bool g_showBravaisWindow = false;

periodic_table::PeriodicTableController* g_periodicController = nullptr;
periodic_table::PeriodicTableUI* g_periodicUI = nullptr;
bravais::BravaisController* g_bravaisController = nullptr;
bravais::BravaisLatticeUI* g_bravaisUI = nullptr;

} // namespace

void InitOnce(core::scene::SceneState& scene) {
    if (g_periodicController != nullptr) {
        return;
    }

    static periodic_table::PeriodicTableController periodicController(scene);
    static periodic_table::PeriodicTableUI periodicUI(periodicController);
    static bravais::BravaisController bravaisController(scene);
    static bravais::BravaisLatticeUI bravaisUI(bravaisController);

    g_periodicController = &periodicController;
    g_periodicUI = &periodicUI;
    g_bravaisController = &bravaisController;
    g_bravaisUI = &bravaisUI;
}

void DrawMenu() {
    if (ImGui::BeginMenu("Build")) {
        if (ImGui::MenuItem("Add atoms")) {
            g_showPeriodicTableWindow = true;
        }
        if (ImGui::MenuItem("Bravais Lattice Templates")) {
            g_showBravaisWindow = true;
        }
        ImGui::EndMenu();
    }
}

void HandleRequest(const Request& request) {
    switch (request.type) {
    case Request::Type::ShowPeriodicTable:
        g_showPeriodicTableWindow = true;
        break;
    case Request::Type::ShowBravaisLattice:
        g_showBravaisWindow = true;
        break;
    default:
        break;
    }
}

void RenderWindows() {
    if (g_showPeriodicTableWindow && g_periodicUI != nullptr) {
        g_periodicUI->Render(&g_showPeriodicTableWindow);
    }

    if (g_showBravaisWindow && g_bravaisUI != nullptr) {
        g_bravaisUI->Render(&g_showBravaisWindow);
    }
}

void Tick(float /*dt*/) {
}

void Shutdown() {
}

} // namespace features::build
