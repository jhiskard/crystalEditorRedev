#include "measurement_menu.h"

#include "measurement_controller.h"
#include "measurement_overlay_ui.h"
#include "measurement_store.h"

#include "core/scene/scene_state.h"
#include "core/vtk/mouse_interactor.h"

#include <imgui.h>

namespace features::measurement {
namespace {

MeasurementStore* g_store = nullptr;
MeasurementController* g_controller = nullptr;
MeasurementOverlayUI* g_ui = nullptr;
bool g_showMeasurementWindow = false;

void DrawModeItem(const char* label, MeasurementMode mode) {
    if (g_controller == nullptr) {
        return;
    }

    const bool selected = g_controller->CurrentMode() == mode;
    if (ImGui::MenuItem(label, nullptr, selected)) {
        g_controller->EnterMode(mode);
        g_showMeasurementWindow = true;
    }
}

} // namespace

void InitOnce(core::scene::SceneState& scene, core::vtk::MouseInteractor& mouseInteractor) {
    if (g_store != nullptr) {
        return;
    }

    static MeasurementStore store(scene);
    static MeasurementController controller(scene, store);
    static MeasurementOverlayUI ui;

    store.Subscribe();
    controller.SubscribeEvents();
    controller.Subscribe(mouseInteractor);

    g_store = &store;
    g_controller = &controller;
    g_ui = &ui;
}

void DrawMenu() {
    if (ImGui::BeginMenu("Measurement")) {
        DrawModeItem("Distance", MeasurementMode::Distance);
        DrawModeItem("Angle", MeasurementMode::Angle);
        DrawModeItem("Dihedral", MeasurementMode::Dihedral);
        DrawModeItem("Geometric Center", MeasurementMode::GeometricCenter);
        DrawModeItem("Center of Mass", MeasurementMode::CenterOfMass);
        ImGui::Separator();
        if (ImGui::MenuItem("Measurement List", nullptr, g_showMeasurementWindow)) {
            g_showMeasurementWindow = true;
        }
        if (g_controller != nullptr && g_controller->IsActive()) {
            if (ImGui::MenuItem("Exit Measurement Mode")) {
                g_controller->ExitMode();
            }
        }
        ImGui::EndMenu();
    }
}

void RenderWindows() {
    if (g_ui != nullptr && g_controller != nullptr && g_store != nullptr) {
        g_ui->Render(*g_controller, *g_store, &g_showMeasurementWindow);
    }
}

void Shutdown() {
    if (g_controller != nullptr) {
        g_controller->ExitMode();
    }
    if (g_store != nullptr) {
        g_store->Clear();
    }
    g_store = nullptr;
    g_controller = nullptr;
    g_ui = nullptr;
    g_showMeasurementWindow = false;
}

} // namespace features::measurement
