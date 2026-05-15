#include "data_menu.h"

#include "charge_density/charge_density_controller.h"
#include "charge_density/charge_density_ui.h"
#include "slice/slice_controller.h"
#include "slice/slice_ui.h"

#include "core/io/chgcar_parser.h"
#include "core/io/format_registry.h"
#include "core/io/xsf_parser.h"
#include "core/scene/scene_state.h"
#include "core/vtk/vtk_viewer.h"

#include <imgui.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace features::data {
namespace {

bool g_showChargeDensityWindow = false;
bool g_showSliceWindow = false;

charge_density::ChargeDensityController* g_chargeDensityController = nullptr;
charge_density::ChargeDensityUI* g_chargeDensityUI = nullptr;
slice::SliceController* g_sliceController = nullptr;
slice::SliceUI* g_sliceUI = nullptr;

bool g_quickAnimationActive = false;
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
    g_quickAnimationActive = false;
}

bool LoadChgcarParseResult(const std::string& name, const core::io::ChgcarParser::ParseResult& parsed) {
    if (g_chargeDensityController == nullptr) {
        return false;
    }

    std::unique_ptr<charge_density::ChargeDensity> data =
        charge_density::ChargeDensity::FromChgcarParseResult(parsed);
    if (data == nullptr) {
        return false;
    }

    std::unique_ptr<charge_density::ChargeDensity> sliceData =
        std::make_unique<charge_density::ChargeDensity>(*data);
    g_chargeDensityController->SetNamedData(name.empty() ? std::string("CHGCAR") : name, std::move(data));
    if (g_sliceController != nullptr) {
        g_sliceController->SetData(std::move(sliceData));
    }
    return true;
}

bool LoadXsfGridResult(const std::string& name, const core::io::XsfGridParseResult& parsed) {
    (void)name;
    if (g_chargeDensityController == nullptr) {
        return false;
    }

    std::vector<std::pair<std::string, std::unique_ptr<charge_density::ChargeDensity>>> entries;
    entries.reserve(parsed.grids.size());
    int unnamedIndex = 1;
    for (const core::io::XsfGridData& grid : parsed.grids) {
        std::unique_ptr<charge_density::ChargeDensity> data =
            charge_density::ChargeDensity::FromXsfGridData(parsed, grid);
        if (data == nullptr) {
            continue;
        }

        std::string gridName = grid.label;
        if (gridName.empty()) {
            gridName = "noname_";
            if (unnamedIndex < 10) {
                gridName += "0";
            }
            gridName += std::to_string(unnamedIndex);
        }
        ++unnamedIndex;
        entries.emplace_back(std::move(gridName), std::move(data));
    }

    if (entries.empty()) {
        return false;
    }

    std::unique_ptr<charge_density::ChargeDensity> sliceData =
        std::make_unique<charge_density::ChargeDensity>(*entries.front().second);
    g_chargeDensityController->SetNamedDataEntries(std::move(entries));
    if (g_sliceController != nullptr) {
        g_sliceController->SetData(std::move(sliceData));
    }
    return true;
}

bool HasChargeDensity() {
    return g_chargeDensityController != nullptr && g_chargeDensityController->HasData();
}

std::string GetActiveChargeDensityName() {
    if (!HasChargeDensity()) {
        return "Charge Density";
    }

    const std::vector<std::string> names = g_chargeDensityController->DataNames();
    const int activeIndex = g_chargeDensityController->ActiveDataIndex();
    if (activeIndex >= 0 && activeIndex < static_cast<int>(names.size())) {
        return names[static_cast<size_t>(activeIndex)];
    }
    return "Charge Density";
}

bool SetChargeDensityLevelPercent(float percent) {
    if (!HasChargeDensity()) {
        return false;
    }

    percent = std::clamp(percent, 0.0f, 100.0f);
    const float minValue = g_chargeDensityController->DataMin();
    const float maxValue = g_chargeDensityController->DataMax();
    const float isoValue = minValue + (maxValue - minValue) * (percent / 100.0f);
    if (g_chargeDensityController->CurrentMode() == charge_density::Mode::None) {
        g_chargeDensityController->Show(charge_density::Mode::Isosurface);
    }
    g_chargeDensityController->SetIsoValue(isoValue);
    core::vtk::VtkViewer::Instance().RequestRender();
    return true;
}

bool IsQuickAnimationActive() {
    return g_quickAnimationActive;
}

void StartQuickAnimation() {
    if (!HasChargeDensity()) {
        return;
    }
    g_quickAnimationActive = true;
}

void StopQuickAnimation() {
    g_quickAnimationActive = false;
}

void RestartQuickAnimation() {
    StopQuickAnimation();
    StartQuickAnimation();
}

} // namespace features::data
