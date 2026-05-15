#include "edit_menu.h"

#include "atoms/atom_editor_ui.h"
#include "atoms/atoms_controller.h"
#include "bonds/bond_ui.h"
#include "bonds/bonds_controller.h"
#include "cell/cell_controller.h"
#include "cell/cell_info_ui.h"

#include "core/scene/scene_state.h"
#include "core/vtk/vtk_viewer.h"

#include <imgui.h>

namespace features::edit {
namespace {

bool g_showCellInfoWindow = false;
bool g_showCreatedAtomsWindow = false;
bool g_showBondsManagementWindow = false;

core::scene::SceneState* g_scene = nullptr;
cell::CellController* g_cellController = nullptr;
cell::CellInfoUI* g_cellInfoUI = nullptr;
atoms::AtomsController* g_atomsController = nullptr;
atoms::AtomEditorUI* g_atomEditorUI = nullptr;
bonds::BondsController* g_bondsController = nullptr;
bonds::BondUI* g_bondUI = nullptr;

} // namespace

void InitOnce(core::scene::SceneState& scene, core::vtk::MouseInteractor& mouseInteractor) {
    if (g_cellController != nullptr) {
        return;
    }

    static cell::CellController cellController(scene);
    static cell::CellInfoUI cellInfoUI(cellController);
    static atoms::AtomsController atomsController(scene, cellController.Manager());
    static atoms::AtomEditorUI atomEditorUI(atomsController);
    static bonds::BondsController bondsController(scene);
    static bonds::BondUI bondUI(bondsController);

    cellController.Subscribe();
    atomsController.SubscribeEvents();
    atomsController.Subscribe(mouseInteractor);
    bondsController.Subscribe();
    bondsController.SubscribeAtomChanges();

    g_cellController = &cellController;
    g_cellInfoUI = &cellInfoUI;
    g_atomsController = &atomsController;
    g_atomEditorUI = &atomEditorUI;
    g_bondsController = &bondsController;
    g_bondUI = &bondUI;
    g_scene = &scene;
}

void DrawMenu() {
    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Atoms")) {
            g_showCreatedAtomsWindow = true;
        }
        if (ImGui::MenuItem("Bonds")) {
            g_showBondsManagementWindow = true;
        }
        if (ImGui::MenuItem("Cell")) {
            g_showCellInfoWindow = true;
        }
        ImGui::EndMenu();
    }
}

void HandleRequest(const Request& request) {
    switch (request.type) {
    case Request::Type::ShowCellInformation:
        g_showCellInfoWindow = true;
        break;
    case Request::Type::ShowCreatedAtoms:
        g_showCreatedAtomsWindow = true;
        break;
    case Request::Type::ShowBondsManagement:
        g_showBondsManagementWindow = true;
        break;
    default:
        break;
    }
}

void RenderWindows() {
    if (g_showCreatedAtomsWindow && g_atomEditorUI != nullptr) {
        g_atomEditorUI->Render(&g_showCreatedAtomsWindow);
    }

    if (g_showBondsManagementWindow && g_bondUI != nullptr) {
        g_bondUI->Render(&g_showBondsManagementWindow);
    }

    if (g_showCellInfoWindow && g_cellInfoUI != nullptr) {
        g_cellInfoUI->Render(&g_showCellInfoWindow);
    }
}

void Tick(float /*dt*/) {
}

void Shutdown() {
    g_scene = nullptr;
    g_cellController = nullptr;
    g_cellInfoUI = nullptr;
    g_atomsController = nullptr;
    g_atomEditorUI = nullptr;
    g_bondsController = nullptr;
    g_bondUI = nullptr;
    g_showCellInfoWindow = false;
    g_showCreatedAtomsWindow = false;
    g_showBondsManagementWindow = false;
}

bool IsBoundaryAtomsEnabled() {
    return g_atomsController != nullptr && g_atomsController->BoundaryAtomsEnabled();
}

void SetBoundaryAtomsEnabled(bool enabled) {
    if (g_atomsController == nullptr) {
        return;
    }
    g_atomsController->SetBoundaryAtomsEnabled(enabled);
    core::vtk::VtkViewer::Instance().RequestRender();
}

bool AlignCameraToCurrentCellAxis(int axisIndex) {
    if (g_scene == nullptr || g_scene->currentStructureId < 0) {
        return false;
    }

    auto recordIt = g_scene->structureRecords.find(g_scene->currentStructureId);
    if (recordIt == g_scene->structureRecords.end() || !recordIt->second.cell.hasCell) {
        return false;
    }

    return core::vtk::VtkViewer::Instance().AlignCameraToCellAxis(recordIt->second.cell.matrix, axisIndex);
}

} // namespace features::edit
