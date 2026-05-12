#include "file_menu.h"

#include "structure_import_controller.h"
#include "structure_import_ui.h"

#include "core/io/file_dialog.h"
#include "core/scene/scene_state.h"

#include <imgui.h>

namespace features::file {
namespace {

StructureImportController* g_controller = nullptr;
StructureImportUI* g_ui = nullptr;

StructureImportController* controller() {
    return g_controller;
}

} // namespace

void InitOnce(core::scene::SceneState& scene) {
    if (g_controller != nullptr) {
        return;
    }

    static StructureImportController controllerInstance(scene);
    static StructureImportUI uiInstance;
    g_controller = &controllerInstance;
    g_ui = &uiInstance;
}

void DrawMenu() {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open Structure File")) {
            OpenStructure();
        }
        ImGui::MenuItem("Open Recent", nullptr, false, false);
        ImGui::EndMenu();
    }
}

void HandleRequest(const Request& request) {
    switch (request.type) {
    case Request::Type::OpenStructure:
        OpenStructure();
        break;
    default:
        break;
    }
}

void RenderWindows() {
    if (g_controller != nullptr && g_ui != nullptr) {
        g_ui->Render(*g_controller);
    }
}

void Tick(float /*dt*/) {
}

void Shutdown() {
}

void OpenStructure() {
    if (StructureImportController* c = controller(); c != nullptr) {
        c->RequestOpenStructureImport();
    }
}

void LoadArrayBuffer(const std::string& fileName, bool deleteFile) {
    if (StructureImportController* c = controller(); c != nullptr) {
        c->LoadArrayBuffer(fileName, deleteFile);
    }
}

void LoadChgcarFile(const std::string& fileName) {
    if (StructureImportController* c = controller(); c != nullptr) {
        c->LoadChgcarFile(fileName);
    }
}

void HandleXsfGridFile(const std::string& fileName) {
    if (StructureImportController* c = controller(); c != nullptr) {
        c->HandleXsfGridFile(fileName);
    }
}

void HandleStructureFile(const std::string& fileName) {
    if (StructureImportController* c = controller(); c != nullptr) {
        c->HandleStructureFile(fileName);
    }
}

void WriteChunk(const std::string& fileName, int32_t offset, uintptr_t data, int32_t length) {
    core::io::FileDialog::WriteChunk(fileName, offset, data, length);
}

void CloseFile(const std::string& fileName) {
    core::io::FileDialog::CloseFile(fileName);
}

void ProcessFileInBackground(const std::string& fileName, bool deleteFile) {
    if (StructureImportController* c = controller(); c != nullptr) {
        c->ProcessFileInBackground(fileName, deleteFile);
    }
}

void ShowProgressPopup(bool show) {
    if (StructureImportController* c = controller(); c != nullptr) {
        c->ShowProgressPopup(show);
    }
}

void SetProgress(float progress) {
    if (StructureImportController* c = controller(); c != nullptr) {
        c->SetProgress(progress);
    }
}

void SetProgressPopupText(const std::string& title, const std::string& text) {
    if (StructureImportController* c = controller(); c != nullptr) {
        c->SetProgressPopupText(title, text);
    }
}

} // namespace features::file
