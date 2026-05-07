#include "data_menu.h"

#include "charge_density/charge_density_controller.h"
#include "charge_density/charge_density_ui.h"
#include "slice/slice_controller.h"
#include "slice/slice_ui.h"

#include "core/io/chgcar_parser.h"
#include "core/io/format_registry.h"
#include "core/scene/scene_state.h"

#include <imgui.h>

namespace features::data {
namespace {

bool g_showChargeDensityWindow = false;
bool g_showSliceWindow = false;

charge_density::ChargeDensityController* g_chargeDensityController = nullptr;
charge_density::ChargeDensityUI* g_chargeDensityUI = nullptr;
slice::SliceController* g_sliceController = nullptr;
slice::SliceUI* g_sliceUI = nullptr;

core::io::FormatRegistry g_registry;
bool g_registryInitialized = false;

void initializeFormatRegistry() {
    if (g_registryInitialized) {
        return;
    }

    core::io::FormatRegistry::RegisterDefaults(g_registry);
    g_registry.Register(".chgcar", [](const std::string& path, core::io::ParseResult& out) {
        const core::io::ChgcarParser::ParseResult parsed = core::io::ChgcarParser::parse(path);
        out = core::io::ParseResult{};
        out.parserId = "chgcar";
        out.path = path;
        out.success = parsed.success;
        out.errorMessage = parsed.errorMessage;
        out.payload = parsed;
        return out.success;
    });

    g_registryInitialized = true;
}

} // namespace

void InitOnce(core::scene::SceneState& scene) {
    if (g_chargeDensityController != nullptr) {
        return;
    }

    static charge_density::ChargeDensityController chargeDensityController(scene);
    static slice::SliceController sliceController(scene);
    static charge_density::ChargeDensityUI chargeDensityUI(chargeDensityController, &sliceController);
    static slice::SliceUI sliceUI(sliceController);

    g_chargeDensityController = &chargeDensityController;
    g_chargeDensityUI = &chargeDensityUI;
    g_sliceController = &sliceController;
    g_sliceUI = &sliceUI;

    initializeFormatRegistry();
}

void DrawMenu() {
    if (ImGui::BeginMenu("Data")) {
        if (ImGui::MenuItem("Isosurface")) {
            if (g_chargeDensityController != nullptr) {
                g_chargeDensityController->Show(charge_density::Mode::Isosurface);
            }
            g_showChargeDensityWindow = true;
        }
        if (ImGui::MenuItem("Surface")) {
            if (g_chargeDensityController != nullptr) {
                g_chargeDensityController->Show(charge_density::Mode::Surface);
            }
            g_showChargeDensityWindow = true;
        }
        if (ImGui::MenuItem("Volumetric")) {
            if (g_chargeDensityController != nullptr) {
                g_chargeDensityController->Show(charge_density::Mode::Volumetric);
            }
            g_showChargeDensityWindow = true;
        }
        if (ImGui::MenuItem("Plane")) {
            if (g_sliceController != nullptr) {
                g_sliceController->Show();
            }
            g_showSliceWindow = true;
        }
        ImGui::EndMenu();
    }
}

void HandleRequest(const Request& request) {
    switch (request.type) {
    case Request::Type::ShowIsosurface:
        if (g_chargeDensityController != nullptr) {
            g_chargeDensityController->Show(charge_density::Mode::Isosurface);
        }
        g_showChargeDensityWindow = true;
        break;
    case Request::Type::ShowSurface:
        if (g_chargeDensityController != nullptr) {
            g_chargeDensityController->Show(charge_density::Mode::Surface);
        }
        g_showChargeDensityWindow = true;
        break;
    case Request::Type::ShowVolumetric:
        if (g_chargeDensityController != nullptr) {
            g_chargeDensityController->Show(charge_density::Mode::Volumetric);
        }
        g_showChargeDensityWindow = true;
        break;
    case Request::Type::ShowPlane:
        if (g_sliceController != nullptr) {
            g_sliceController->Show();
        }
        g_showSliceWindow = true;
        break;
    default:
        break;
    }
}

void RenderWindows() {
    if (g_showChargeDensityWindow && g_chargeDensityUI != nullptr) {
        g_chargeDensityUI->Render(&g_showChargeDensityWindow);
    }
    if (g_showSliceWindow && g_sliceUI != nullptr) {
        g_sliceUI->Render(&g_showSliceWindow);
    }
}

void Tick(float /*dt*/) {
}

void Shutdown() {
    if (g_chargeDensityController != nullptr) {
        g_chargeDensityController->Clear();
    }
    if (g_sliceController != nullptr) {
        g_sliceController->Clear();
    }
}

} // namespace features::data
