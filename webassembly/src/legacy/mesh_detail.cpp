#include "mesh_detail.h"
#include "app.h"
#include "font_manager.h"
#include "mesh_manager.h"

// ImGui
#include <imgui.h>

// Standard library
#include <algorithm>
#include <functional>  // ✅ 추가

// VTK
#include <vtkActor.h>


// ============================================================================
// Static Member Initialization
// ============================================================================
bool MeshDetail::s_HasEdgeMeshColorChanged = false;
bool MeshDetail::s_HasEdgeMeshOpacityChanged = false;
bool MeshDetail::s_HasFaceMeshColorChanged = false;
bool MeshDetail::s_HasFaceMeshOpacityChanged = false;
bool MeshDetail::s_HasFaceMeshEdgeColorChanged = false;
bool MeshDetail::s_HasVolumeMeshColorChanged = false;
bool MeshDetail::s_HasVolumeMeshOpacityChanged = false;
bool MeshDetail::s_HasVolumeMeshEdgeColorChanged = false;

// ✅ 추가: Volume Rendering Static Flags
bool MeshDetail::s_HasVolumeRenderOpacityMinChanged = false;
bool MeshDetail::s_HasVolumeRenderOpacityMaxChanged = false;
bool MeshDetail::s_HasVolumeWindowLevelChanged = false;
bool MeshDetail::s_HasVolumeColorCurveChanged = false;
bool MeshDetail::s_HasVolumeSampleDistanceChanged = false;

// ============================================================================
// ✅ 추가: Helper Function for Volume Mesh Iteration
// ============================================================================
namespace {
void forEachVolumeMesh(const std::function<void(Mesh*)>& func) {
    MeshManager& meshManager = MeshManager::Instance();
    meshManager.GetMeshTree()->TraverseTree([&](const TreeNode* node, void*) {
        Mesh* mesh = meshManager.GetMeshByIdMutable(node->GetId());
        if (!mesh || !mesh->GetVolumeMeshActor()) {
            return;
        }
        func(mesh);
    });
}

float calcVolumeActionButtonWidth() {
    const ImGuiStyle& style = ImGui::GetStyle();
    const float maxLabelWidth = std::max(ImGui::CalcTextSize("Apply").x, ImGui::CalcTextSize("Reset").x);
    const float extraPadding = 8.0f * App::UiScale();
    return maxLabelWidth + style.FramePadding.x * 2.0f + extraPadding;
}

float calcVolumeActionButtonHeight() {
    return std::max(ImGui::GetFrameHeight(), App::TextBaseHeight() * 1.05f);
}

ImVec2 calcVolumeActionButtonPadding() {
    const ImGuiStyle& style = ImGui::GetStyle();
    return ImVec2(style.FramePadding.x, std::max(style.FramePadding.y, style.FramePadding.y * App::UiScale()));
}
}  // namespace

// ============================================================================
// Constructor / Destructor
// ============================================================================
MeshDetail::MeshDetail() {
}

MeshDetail::~MeshDetail() {
}

// ============================================================================
// Main Render Function
// ============================================================================
void MeshDetail::Render(int32_t meshId, bool* openWindow) {
    if (meshId == -1) {
        return;
    }

    ImGui::Begin(ICON_FA6_FOLDER_OPEN"  Mesh Detail", openWindow);

    MeshManager& meshManager = MeshManager::Instance();
    const Mesh* mesh = meshManager.GetMeshById(meshId);
    if (mesh == nullptr) {
        ImGui::End();
        return;
    }
    m_VolumeUseSharedSettings = true;

    // ✅ 추가: Volume Mesh가 있으면 공유 설정 동기화
    if (mesh->GetVolumeMeshActor()) {
        if (!meshManager.HasSharedVolumeDisplaySettings()) {
            meshManager.EnsureSharedVolumeDisplaySettingsFromMesh(*mesh);
        }
        syncSharedVolumeUiFromSettings(meshManager.GetSharedVolumeDisplaySettings());
    }

    const int32_t tableColumeCount = 2;

    ImGuiTableFlags tableFlags = 
        ImGuiTableFlags_Resizable | 
        ImGuiTableFlags_PadOuterX | 
        ImGuiTableFlags_Borders | 
        ImGuiTableFlags_RowBg;

    // ========================================================================
    // Basic Information Section
    // ========================================================================
    bool openBasic = ImGui::CollapsingHeader("Basic Infomation", ImGuiTreeNodeFlags_DefaultOpen);
    if (openBasic && ImGui::BeginTable("Basic Infomation", tableColumeCount, tableFlags)) {
        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, App::TextBaseWidth() * 10.0f);
        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch, App::TextBaseWidth() * 15.0f);

        renderTableRow("Id", mesh->GetId());
        renderTableRow("Name", mesh->GetName());

        ImGui::EndTable();
    }

    // ========================================================================
    // Edge Mesh Display Section
    // ========================================================================
    if (mesh->GetEdgeMeshActor()) {
        bool openDisplay = ImGui::CollapsingHeader("Edge Mesh Display", ImGuiTreeNodeFlags_None);
        if (openDisplay && ImGui::BeginTable("EdgeMeshDisplay", tableColumeCount, tableFlags)) {
            ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, App::TextBaseWidth() * 10.0f);
            ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch, App::TextBaseWidth() * 20.0f);

            renderTableRowEdgeMeshVisibility(mesh->GetId(), mesh->GetEdgeMeshVisibility());
            renderTableRowEdgeMeshColor(mesh->GetId(), mesh->GetEdgeMeshColor());
            renderTableRowEdgeMeshOpacity(mesh->GetId(), mesh->GetEdgeMeshOpacity());

            ImGui::EndTable();
        }
    }

    // ========================================================================
    // Face Mesh Display Section
    // ========================================================================
    if (mesh->GetFaceMeshActor()) {
        bool openDisplay = ImGui::CollapsingHeader("Face Mesh Display", ImGuiTreeNodeFlags_None);
        if (openDisplay && ImGui::BeginTable("FaceMeshDisplay", tableColumeCount, tableFlags)) {
            ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, App::TextBaseWidth() * 10.0f);
            ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch, App::TextBaseWidth() * 20.0f);

            renderTableRowFaceMeshVisibility(mesh->GetId(), mesh->GetFaceMeshVisibility());
            renderTableRowFaceMeshColor(mesh->GetId(), mesh->GetFaceMeshColor());
            renderTableRowFaceMeshOpacity(mesh->GetId(), mesh->GetFaceMeshOpacity());
            renderTableRowFaceMeshEdgeColor(mesh->GetId(), mesh->GetFaceMeshEdgeColor());

            ImGui::EndTable();
        }
    }

    // ========================================================================
    // Volume Mesh Display Section (✅ 수정: Volume Rendering 옵션 추가)
    // ========================================================================
    if (mesh->GetVolumeMeshActor()) {
        bool openDisplay = ImGui::CollapsingHeader("Volume Mesh Display", ImGuiTreeNodeFlags_DefaultOpen);
        if (openDisplay && ImGui::BeginTable("VolumeMeshDisplay", tableColumeCount, tableFlags)) {
            ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, App::TextBaseWidth() * 10.0f);
            ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch, App::TextBaseWidth() * 20.0f);

            // ✅ 추가: Volume Rendering 관련 UI
            renderTableRowVolumeRenderMode(mesh->GetId(), mesh->IsVolumeRenderingAvailable());
            renderTableRowVolumeMeshVisibility(mesh->GetId(), mesh->GetVolumeMeshVisibility(), false);
            renderTableRowVolumeDirectApply();
            renderTableRowVolumeColorPreset(mesh->GetId(), mesh->IsVolumeRenderingAvailable());
            renderTableRowVolumeColorCurve(mesh->GetId());
            renderTableRowVolumeWindowLevel(mesh->GetId());
            renderTableRowVolumeQuality(mesh->GetId());
            
            // ✅ 추가: Render Mode에 따라 다른 옵션 표시
            if (m_UiVolumeRenderMode == static_cast<int>(Mesh::VolumeRenderMode::Volume)) {
                renderTableRowVolumeSampleDistance(mesh->GetId());
                renderTableRowVolumeRenderOpacityMin(mesh->GetId(), mesh->GetVolumeRenderOpacityMin());
                renderTableRowVolumeRenderOpacity(mesh->GetId(), mesh->GetVolumeRenderOpacity());
            } else {
                renderTableRowVolumeMeshColor(mesh->GetId(), mesh->GetVolumeMeshColor());
                renderTableRowVolumeMeshOpacity(mesh->GetId(), mesh->GetVolumeMeshOpacity());
                renderTableRowVolumeMeshEdgeColor(mesh->GetId(), mesh->GetVolumeMeshEdgeColor());
            }

            ImGui::EndTable();
        }
    }
       
    ImGui::End();
}

void MeshDetail::RenderVolumeControls(int32_t meshId, const char* tableId, bool showRenderMode) {
    if (meshId == -1) {
        ImGui::TextDisabled("No mesh selected");
        return;
    }

    MeshManager& meshManager = MeshManager::Instance();
    const Mesh* mesh = meshManager.GetMeshById(meshId);
    if (mesh == nullptr || !mesh->GetVolumeMeshActor()) {
        ImGui::TextDisabled("No volume mesh available");
        return;
    }

    m_VolumeUseSharedSettings = false;
    if (m_LastVolumeControlMeshId != meshId) {
        m_LastVolumeControlMeshId = meshId;
        s_HasVolumeMeshColorChanged = false;
        s_HasVolumeMeshOpacityChanged = false;
        s_HasVolumeMeshEdgeColorChanged = false;
        s_HasVolumeColorCurveChanged = false;
        s_HasVolumeWindowLevelChanged = false;
        s_HasVolumeSampleDistanceChanged = false;
        s_HasVolumeRenderOpacityMinChanged = false;
        s_HasVolumeRenderOpacityMaxChanged = false;
    }
    syncVolumeUiFromMesh(*mesh);

    const int32_t tableColumeCount = 2;
    ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_PadOuterX |
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg;

    ImGui::PushID(meshId);
    const char* tableName = (tableId && tableId[0] != '\0') ? tableId : "XsfGridVolumeDisplay";
    if (ImGui::BeginTable(tableName, tableColumeCount, tableFlags)) {
        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, App::TextBaseWidth() * 10.0f);
        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch, App::TextBaseWidth() * 20.0f);

        if (showRenderMode) {
            renderTableRowVolumeRenderMode(mesh->GetId(), mesh->IsVolumeRenderingAvailable());
        }
        renderTableRowVolumeMeshVisibility(mesh->GetId(), mesh->GetVolumeMeshVisibility(), true);
        renderTableRowVolumeDirectApply();
        renderTableRowVolumeColorPreset(mesh->GetId(), mesh->IsVolumeRenderingAvailable());
        renderTableRowVolumeColorCurve(mesh->GetId());
        renderTableRowVolumeWindowLevel(mesh->GetId());
        renderTableRowVolumeQuality(mesh->GetId());

        if (m_UiVolumeRenderMode == static_cast<int>(Mesh::VolumeRenderMode::Volume)) {
            renderTableRowVolumeSampleDistance(mesh->GetId());
            renderTableRowVolumeRenderOpacityMin(mesh->GetId(), mesh->GetVolumeRenderOpacityMin());
            renderTableRowVolumeRenderOpacity(mesh->GetId(), mesh->GetVolumeRenderOpacity());
        } else {
            renderTableRowVolumeMeshColor(mesh->GetId(), mesh->GetVolumeMeshColor());
            renderTableRowVolumeMeshOpacity(mesh->GetId(), mesh->GetVolumeMeshOpacity());
            renderTableRowVolumeMeshEdgeColor(mesh->GetId(), mesh->GetVolumeMeshEdgeColor());
        }

        ImGui::EndTable();
    }
    ImGui::PopID();
}

void MeshDetail::forEachTargetVolumeMesh(int32_t meshId, const std::function<void(Mesh*)>& func) {
    if (m_VolumeUseSharedSettings) {
        forEachVolumeMesh(func);
        return;
    }
    Mesh* mesh = MeshManager::Instance().GetMeshByIdMutable(meshId);
    if (!mesh || !mesh->GetVolumeMeshActor()) {
        return;
    }
    func(mesh);
}

void MeshDetail::syncVolumeUiFromMesh(const Mesh& mesh) {
    m_UiVolumeRenderMode = (mesh.GetVolumeRenderMode() == Mesh::VolumeRenderMode::Volume) ? 1 : 0;
    m_UiVolumeMeshVisibility = mesh.GetVolumeMeshVisibility();
    m_UiVolumeColorPreset = static_cast<int>(mesh.GetVolumeColorPreset());

    if (!s_HasVolumeColorCurveChanged) {
        m_UiVolumeColorMidpoint = static_cast<float>(mesh.GetVolumeColorMidpoint());
        m_UiVolumeColorSharpness = static_cast<float>(mesh.GetVolumeColorSharpness());
    }

    if (!s_HasVolumeWindowLevelChanged) {
        double rangeMin = 0.0;
        double rangeMax = 1.0;
        const double* range = mesh.GetVolumeDataRange();
        if (range) {
            rangeMin = range[0];
            rangeMax = range[1];
        }
        if (rangeMin == rangeMax) {
            rangeMax = rangeMin + 1.0;
        }
        double windowMax = rangeMax - rangeMin;
        double windowValue = mesh.GetVolumeWindow();
        if (windowValue < 0.0) windowValue = 0.0;
        else if (windowValue > windowMax) windowValue = windowMax;

        double levelValue = mesh.GetVolumeLevel();
        if (levelValue < rangeMin) levelValue = rangeMin;
        else if (levelValue > rangeMax) levelValue = rangeMax;

        m_UiVolumeWindow = static_cast<float>(windowValue);
        m_UiVolumeLevel = static_cast<float>(levelValue);
    }

    if (!s_HasVolumeRenderOpacityMinChanged) {
        m_UiVolumeRenderOpacityMin = static_cast<float>(mesh.GetVolumeRenderOpacityMin());
    }
    if (!s_HasVolumeRenderOpacityMaxChanged) {
        m_UiVolumeRenderOpacity = static_cast<float>(mesh.GetVolumeRenderOpacity());
    }
    if (!s_HasVolumeSampleDistanceChanged) {
        m_UiVolumeSampleDistanceIndex = mesh.GetVolumeSampleDistanceIndex();
    }

    m_UiVolumeQuality = static_cast<int>(mesh.GetVolumeQuality());

    if (!s_HasVolumeMeshColorChanged) {
        const double* meshColor = mesh.GetVolumeMeshColor();
        if (meshColor) {
            m_UiVolumeMeshColor[0] = static_cast<float>(meshColor[0]);
            m_UiVolumeMeshColor[1] = static_cast<float>(meshColor[1]);
            m_UiVolumeMeshColor[2] = static_cast<float>(meshColor[2]);
        }
    }
    if (!s_HasVolumeMeshOpacityChanged) {
        m_UiVolumeMeshOpacity = static_cast<float>(mesh.GetVolumeMeshOpacity());
    }
    if (!s_HasVolumeMeshEdgeColorChanged) {
        const double* edgeColor = mesh.GetVolumeMeshEdgeColor();
        if (edgeColor) {
            m_UiVolumeMeshEdgeColor[0] = static_cast<float>(edgeColor[0]);
            m_UiVolumeMeshEdgeColor[1] = static_cast<float>(edgeColor[1]);
            m_UiVolumeMeshEdgeColor[2] = static_cast<float>(edgeColor[2]);
        }
    }
}

// ============================================================================
// Basic Table Row Rendering
// ============================================================================
void MeshDetail::renderTableRow(const char* item, int32_t value) {
    ImGui::TableNextColumn();
    ImGui::Text("%s", item);
    ImGui::TableNextColumn();
    ImGui::Text("%d", value);
}

void MeshDetail::renderTableRow(const char* item, const char* value) {
    ImGui::TableNextColumn();
    ImGui::Text("%s", item);
    ImGui::TableNextColumn();
    if (value) {
        ImGui::Text("%s", value);
    } else {
        ImGui::TextDisabled("--");
    }
}

void MeshDetail::renderTableRow(const char* item, double value) {
    ImGui::TableNextColumn();
    ImGui::Text("%s", item);
    ImGui::TableNextColumn();
    ImGui::Text("%.3lf", value);
}

// ============================================================================
// Edge Mesh Table Row Rendering
// ============================================================================
void MeshDetail::renderTableRowEdgeMeshVisibility(int32_t meshId, bool visibility) {
    ImGui::TableNextColumn();
    ImGui::Text("Visibility");
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isClicked = ImGui::Checkbox("##EdgeMeshVisibility", &m_UiEdgeMeshVisibility);
    if (isClicked) {
        Mesh* mesh = MeshManager::Instance().GetMeshByIdMutable(meshId);
        mesh->SetEdgeMeshVisibility(m_UiEdgeMeshVisibility);
    }
    ImGui::PopStyleVar();
}

void MeshDetail::renderTableRowEdgeMeshColor(int32_t meshId, const double* color) {
    ImGui::TableNextColumn();
    ImGui::Text("Color");
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isChanged = ImGui::ColorEdit3("##EdgeMeshColor", m_UiEdgeMeshColor, ImGuiColorEditFlags_Float);
    if (isChanged) {
        s_HasEdgeMeshColorChanged = true;
    }
    if (s_HasEdgeMeshColorChanged) {
        ImGui::SameLine();
        if (ImGui::Button("Apply", ImVec2(App::TextBaseWidth() * 6.0f, App::TextBaseHeight()))) {
            Mesh* mesh = MeshManager::Instance().GetMeshByIdMutable(meshId);
            mesh->SetEdgeMeshColor(m_UiEdgeMeshColor[0], m_UiEdgeMeshColor[1], m_UiEdgeMeshColor[2]);
            s_HasEdgeMeshColorChanged = false;
        }
    }
    ImGui::PopStyleVar();
}

void MeshDetail::renderTableRowEdgeMeshOpacity(int32_t meshId, double opacity) {
    ImGui::TableNextColumn();
    ImGui::Text("Opacity");
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isChanged = ImGui::DragFloat("##EdgeMeshOpacity", &m_UiEdgeMeshOpacity, 0.005f, 0.0f, 1.0f);
    if (isChanged) {
        s_HasEdgeMeshOpacityChanged = true;
    }
    if (s_HasEdgeMeshOpacityChanged) {
        ImGui::SameLine();
        if (ImGui::Button("Apply", ImVec2(App::TextBaseWidth() * 6.0f, App::TextBaseHeight()))) {
            Mesh* mesh = MeshManager::Instance().GetMeshByIdMutable(meshId);
            mesh->SetEdgeMeshOpacity(m_UiEdgeMeshOpacity);
            s_HasEdgeMeshOpacityChanged = false;
        }
    }
    ImGui::PopStyleVar();
}

// ============================================================================
// Face Mesh Table Row Rendering
// ============================================================================
void MeshDetail::renderTableRowFaceMeshVisibility(int32_t meshId, bool visibility) {
    ImGui::TableNextColumn();
    ImGui::Text("Visibility");
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isClicked = ImGui::Checkbox("##FaceMeshVisibility", &m_UiFaceMeshVisibility);
    if (isClicked) {
        Mesh* mesh = MeshManager::Instance().GetMeshByIdMutable(meshId);
        mesh->SetFaceMeshVisibility(m_UiFaceMeshVisibility);
    }
    ImGui::PopStyleVar();
}

void MeshDetail::renderTableRowFaceMeshColor(int32_t meshId, const double* color) {
    ImGui::TableNextColumn();
    ImGui::Text("Color");
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isChanged = ImGui::ColorEdit3("##FaceMeshColor", m_UiFaceMeshColor, ImGuiColorEditFlags_Float);
    if (isChanged) {
        s_HasFaceMeshColorChanged = true;
    }
    if (s_HasFaceMeshColorChanged) {
        ImGui::SameLine();
        if (ImGui::Button("Apply", ImVec2(App::TextBaseWidth() * 6.0f, App::TextBaseHeight()))) {
            Mesh* mesh = MeshManager::Instance().GetMeshByIdMutable(meshId);
            mesh->SetFaceMeshColor(m_UiFaceMeshColor[0], m_UiFaceMeshColor[1], m_UiFaceMeshColor[2]);
            s_HasFaceMeshColorChanged = false;
        }
    }
    ImGui::PopStyleVar();
}

void MeshDetail::renderTableRowFaceMeshOpacity(int32_t meshId, double opacity) {
    ImGui::TableNextColumn();
    ImGui::Text("Opacity");
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isChanged = ImGui::DragFloat("##FaceMeshOpacity", &m_UiFaceMeshOpacity, 0.005f, 0.0f, 1.0f);
    if (isChanged) {
        s_HasFaceMeshOpacityChanged = true;
    }
    if (s_HasFaceMeshOpacityChanged) {
        ImGui::SameLine();
        if (ImGui::Button("Apply", ImVec2(App::TextBaseWidth() * 6.0f, App::TextBaseHeight()))) {
            Mesh* mesh = MeshManager::Instance().GetMeshByIdMutable(meshId);
            mesh->SetFaceMeshOpacity(m_UiFaceMeshOpacity);
            s_HasFaceMeshOpacityChanged = false;
        }
    }
    ImGui::PopStyleVar();
}

void MeshDetail::renderTableRowFaceMeshEdgeColor(int32_t meshId, const double* color) {
    ImGui::TableNextColumn();
    ImGui::Text("Edge Color");
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isChanged = ImGui::ColorEdit3("##FaceMeshEdgeColor", m_UiFaceMeshEdgeColor, ImGuiColorEditFlags_Float);
    if (isChanged) {
        s_HasFaceMeshEdgeColorChanged = true;
    }
    if (s_HasFaceMeshEdgeColorChanged) {
        ImGui::SameLine();
        if (ImGui::Button("Apply", ImVec2(App::TextBaseWidth() * 6.0f, App::TextBaseHeight()))) {
            Mesh* mesh = MeshManager::Instance().GetMeshByIdMutable(meshId);
            mesh->SetFaceMeshEdgeColor(m_UiFaceMeshEdgeColor[0], m_UiFaceMeshEdgeColor[1], m_UiFaceMeshEdgeColor[2]);
            s_HasFaceMeshEdgeColorChanged = false;
        }
    }
    ImGui::PopStyleVar();
}

// ============================================================================
// Volume Mesh Table Row Rendering (기존)
// ============================================================================
void MeshDetail::renderTableRowVolumeMeshVisibility(int32_t meshId, bool visibility, bool linkToTree) {
    (void)visibility;
    ImGui::TableNextColumn();
    ImGui::Text("Visibility");
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isClicked = ImGui::Checkbox("##VolumeMeshVisibility", &m_UiVolumeMeshVisibility);
    if (isClicked) {
        MeshManager& meshManager = MeshManager::Instance();
        if (linkToTree) {
            if (m_UiVolumeMeshVisibility) {
                meshManager.ShowMesh(meshId);
            } else {
                meshManager.HideMesh(meshId);
            }
        } else if (m_VolumeUseSharedSettings) {
            MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
            settings.volumeVisibility = m_UiVolumeMeshVisibility;
            meshManager.SetSharedVolumeDisplaySettings(settings);
            forEachVolumeMesh([&](Mesh* mesh) {
                mesh->SetVolumeMeshVisibility(m_UiVolumeMeshVisibility);
            });
        } else {
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeMeshVisibility(m_UiVolumeMeshVisibility);
            });
        }
    }
    ImGui::PopStyleVar();
}

void MeshDetail::renderTableRowVolumeDirectApply() {
    ImGui::TableNextColumn();
    ImGui::Text("Direct Apply");
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isClicked = ImGui::Checkbox("##VolumeDirectApply", &m_UiVolumeDirectApply);
    if (isClicked && m_UiVolumeDirectApply) {
        s_HasVolumeMeshColorChanged = false;
        s_HasVolumeMeshOpacityChanged = false;
        s_HasVolumeMeshEdgeColorChanged = false;
        s_HasVolumeColorCurveChanged = false;
        s_HasVolumeWindowLevelChanged = false;
        s_HasVolumeSampleDistanceChanged = false;
        s_HasVolumeRenderOpacityMinChanged = false;
        s_HasVolumeRenderOpacityMaxChanged = false;
    }
    ImGui::PopStyleVar();
}
void MeshDetail::renderTableRowVolumeMeshColor(int32_t meshId, const double* color) {
    ImGui::TableNextColumn();
    ImGui::Text("Color");
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isChanged = ImGui::ColorEdit3("##VolumeMeshColor", m_UiVolumeMeshColor, ImGuiColorEditFlags_Float);
    if (isChanged) {
        if (m_UiVolumeDirectApply) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.meshColor[0] = m_UiVolumeMeshColor[0];
                settings.meshColor[1] = m_UiVolumeMeshColor[1];
                settings.meshColor[2] = m_UiVolumeMeshColor[2];
                meshManager.SetSharedVolumeDisplaySettings(settings);
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeMeshColor(m_UiVolumeMeshColor[0], m_UiVolumeMeshColor[1], m_UiVolumeMeshColor[2]);
            });
            s_HasVolumeMeshColorChanged = false;
        } else {
            s_HasVolumeMeshColorChanged = true;
        }
    }
    if (!m_UiVolumeDirectApply && s_HasVolumeMeshColorChanged) {
        ImGui::SameLine();
        if (ImGui::Button("Apply", ImVec2(App::TextBaseWidth() * 6.0f, App::TextBaseHeight()))) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.meshColor[0] = m_UiVolumeMeshColor[0];
                settings.meshColor[1] = m_UiVolumeMeshColor[1];
                settings.meshColor[2] = m_UiVolumeMeshColor[2];
                meshManager.SetSharedVolumeDisplaySettings(settings);
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeMeshColor(m_UiVolumeMeshColor[0], m_UiVolumeMeshColor[1], m_UiVolumeMeshColor[2]);
            });
            s_HasVolumeMeshColorChanged = false;
        }
    }
    ImGui::PopStyleVar();
}

void MeshDetail::renderTableRowVolumeMeshOpacity(int32_t meshId, double opacity) {
    ImGui::TableNextColumn();
    ImGui::Text("Opacity");
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isChanged = ImGui::DragFloat("##VolumeMeshOpacity", &m_UiVolumeMeshOpacity, 0.005f, 0.0f, 1.0f);
    if (isChanged) {
        if (m_UiVolumeDirectApply) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.meshOpacity = m_UiVolumeMeshOpacity;
                meshManager.SetSharedVolumeDisplaySettings(settings);
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeMeshOpacity(m_UiVolumeMeshOpacity);
            });
            s_HasVolumeMeshOpacityChanged = false;
        } else {
            s_HasVolumeMeshOpacityChanged = true;
        }
    }
    if (!m_UiVolumeDirectApply && s_HasVolumeMeshOpacityChanged) {
        ImGui::SameLine();
        if (ImGui::Button("Apply", ImVec2(App::TextBaseWidth() * 6.0f, App::TextBaseHeight()))) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.meshOpacity = m_UiVolumeMeshOpacity;
                meshManager.SetSharedVolumeDisplaySettings(settings);
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeMeshOpacity(m_UiVolumeMeshOpacity);
            });
            s_HasVolumeMeshOpacityChanged = false;
        }
    }
    ImGui::PopStyleVar();
}

void MeshDetail::renderTableRowVolumeMeshEdgeColor(int32_t meshId, const double* color) {
    ImGui::TableNextColumn();
    ImGui::Text("Edge Color");
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isChanged = ImGui::ColorEdit3("##VolumeMeshEdgeColor", m_UiVolumeMeshEdgeColor, ImGuiColorEditFlags_Float);
    if (isChanged) {
        if (m_UiVolumeDirectApply) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.meshEdgeColor[0] = m_UiVolumeMeshEdgeColor[0];
                settings.meshEdgeColor[1] = m_UiVolumeMeshEdgeColor[1];
                settings.meshEdgeColor[2] = m_UiVolumeMeshEdgeColor[2];
                meshManager.SetSharedVolumeDisplaySettings(settings);
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeMeshEdgeColor(m_UiVolumeMeshEdgeColor[0], m_UiVolumeMeshEdgeColor[1], m_UiVolumeMeshEdgeColor[2]);
            });
            s_HasVolumeMeshEdgeColorChanged = false;
        } else {
            s_HasVolumeMeshEdgeColorChanged = true;
        }
    }
    if (!m_UiVolumeDirectApply && s_HasVolumeMeshEdgeColorChanged) {
        ImGui::SameLine();
        if (ImGui::Button("Apply", ImVec2(App::TextBaseWidth() * 6.0f, App::TextBaseHeight()))) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.meshEdgeColor[0] = m_UiVolumeMeshEdgeColor[0];
                settings.meshEdgeColor[1] = m_UiVolumeMeshEdgeColor[1];
                settings.meshEdgeColor[2] = m_UiVolumeMeshEdgeColor[2];
                meshManager.SetSharedVolumeDisplaySettings(settings);
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeMeshEdgeColor(m_UiVolumeMeshEdgeColor[0], m_UiVolumeMeshEdgeColor[1], m_UiVolumeMeshEdgeColor[2]);
            });
            s_HasVolumeMeshEdgeColorChanged = false;
        }
    }
    ImGui::PopStyleVar();
}

// ============================================================================
// ✅ 추가: Volume Rendering Table Row Functions
// ============================================================================
void MeshDetail::renderTableRowVolumeRenderMode(int32_t meshId, bool volumeAvailable) {
    ImGui::TableNextColumn();
    ImGui::Text("Render Mode");
    ImGui::TableNextColumn();
    
    const char* modeLabels[] = { "Surface", "Volume" };
    int modeIndex = m_UiVolumeRenderMode;
    
    if (!volumeAvailable) {
        ImGui::BeginDisabled();
    }
    bool isChanged = ImGui::Combo("##VolumeRenderMode", &modeIndex, modeLabels, IM_ARRAYSIZE(modeLabels));
    if (!volumeAvailable) {
        ImGui::EndDisabled();
    }

    if (isChanged) {
        Mesh::VolumeRenderMode mode = (modeIndex == 1)
            ? Mesh::VolumeRenderMode::Volume
            : Mesh::VolumeRenderMode::Surface;
        if (m_VolumeUseSharedSettings) {
            MeshManager& meshManager = MeshManager::Instance();
            MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
            settings.renderMode = mode;
            meshManager.SetSharedVolumeDisplaySettings(settings);
        }
        forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
            mesh->SetVolumeRenderMode(mode);
        });
        m_UiVolumeRenderMode = modeIndex;
        s_HasVolumeRenderOpacityMinChanged = false;
        s_HasVolumeRenderOpacityMaxChanged = false;
        s_HasVolumeSampleDistanceChanged = false;
    }
}

void MeshDetail::renderTableRowVolumeColorPreset(int32_t meshId, bool volumeAvailable) {
    ImGui::TableNextColumn();
    ImGui::Text("Colormap");
    ImGui::TableNextColumn();
    
    const char* const* presetLabels = common::GetColorMapPresetLabels();
    int presetIndex = common::ClampColorMapPresetIndex(
        m_UiVolumeColorPreset,
        Mesh::VolumeColorPreset::Grayscale);
    
    if (!volumeAvailable) {
        ImGui::BeginDisabled();
    }
    bool isChanged = ImGui::Combo(
        "##VolumeColorPreset",
        &presetIndex,
        presetLabels,
        common::kColorMapPresetCount);
    if (!volumeAvailable) {
        ImGui::EndDisabled();
    }
    
    if (isChanged) {
        const Mesh::VolumeColorPreset preset = common::ColorMapPresetFromIndex(
            presetIndex,
            Mesh::VolumeColorPreset::Grayscale);
        if (m_VolumeUseSharedSettings) {
            MeshManager& meshManager = MeshManager::Instance();
            MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
            settings.colorPreset = preset;
            meshManager.SetSharedVolumeDisplaySettings(settings);
        }
        forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
            mesh->SetVolumeColorPreset(preset);
        });
        m_UiVolumeColorPreset = common::ColorMapPresetToIndex(preset);
    }
}

void MeshDetail::renderTableRowVolumeColorCurve(int32_t meshId) {
    ImGui::TableNextColumn();
    ImGui::Text("Color Curve");
    ImGui::TableNextColumn();
    
    ImGui::PushID("ColorCurve");
    bool midpointChanged = ImGui::SliderFloat("Midpoint##VolumeColorMidpoint", &m_UiVolumeColorMidpoint, 0.0f, 1.0f, "%.2f");
    bool sharpnessChanged = ImGui::SliderFloat("Sharpness##VolumeColorSharpness", &m_UiVolumeColorSharpness, 0.0f, 1.0f, "%.2f");
    
    if (midpointChanged || sharpnessChanged) {
        if (m_UiVolumeDirectApply) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.colorMidpoint = m_UiVolumeColorMidpoint;
                settings.colorSharpness = m_UiVolumeColorSharpness;
                meshManager.SetSharedVolumeDisplaySettings(settings);
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeColorCurve(m_UiVolumeColorMidpoint, m_UiVolumeColorSharpness);
            });
            s_HasVolumeColorCurveChanged = false;
        } else {
            s_HasVolumeColorCurveChanged = true;
        }
    }
    
    const ImGuiStyle& style = ImGui::GetStyle();
    const float actionButtonWidth = calcVolumeActionButtonWidth();
    const float actionButtonHeight = calcVolumeActionButtonHeight();
    const ImVec2 actionButtonPadding = calcVolumeActionButtonPadding();
    const bool showApplyButton = !m_UiVolumeDirectApply && s_HasVolumeColorCurveChanged;
    const float actionRowWidth = ImGui::GetContentRegionAvail().x;
    const bool sameLineActionButtons =
        showApplyButton && actionRowWidth >= (actionButtonWidth * 2.0f + style.ItemSpacing.x);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, actionButtonPadding);

    if (showApplyButton) {
        if (ImGui::Button("Apply##ColorCurve", ImVec2(actionButtonWidth, actionButtonHeight))) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.colorMidpoint = m_UiVolumeColorMidpoint;
                settings.colorSharpness = m_UiVolumeColorSharpness;
                meshManager.SetSharedVolumeDisplaySettings(settings);
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeColorCurve(m_UiVolumeColorMidpoint, m_UiVolumeColorSharpness);
            });
            s_HasVolumeColorCurveChanged = false;
        }
        if (sameLineActionButtons) {
            ImGui::SameLine();
        }
    }
    
    if (ImGui::Button("Reset##ColorCurve", ImVec2(actionButtonWidth, actionButtonHeight))) {
        if (m_VolumeUseSharedSettings) {
            MeshManager& meshManager = MeshManager::Instance();
            MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
            settings.colorMidpoint = 0.5;
            settings.colorSharpness = 0.0;
            meshManager.SetSharedVolumeDisplaySettings(settings);
        }
        forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
            mesh->SetVolumeColorCurve(0.5, 0.0);
        });
        SetUiVolumeColorCurve(0.5, 0.0);
        s_HasVolumeColorCurveChanged = false;
    }
    
    ImGui::PopStyleVar();
    ImGui::PopID();
}

void MeshDetail::renderTableRowVolumeWindowLevel(int32_t meshId) {
    ImGui::TableNextColumn();
    ImGui::Text("Level/Window");
    ImGui::TableNextColumn();
    
    ImGui::PushID("WindowLevel");
    const ImGuiStyle& baseStyle = ImGui::GetStyle();
    const float actionButtonWidth = calcVolumeActionButtonWidth();
    const float actionButtonHeight = calcVolumeActionButtonHeight();
    const ImVec2 actionButtonPadding = calcVolumeActionButtonPadding();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    
    MeshManager& meshManager = MeshManager::Instance();
    Mesh* mesh = meshManager.GetMeshByIdMutable(meshId);
    
    double rangeMin = 0.0;
    double rangeMax = 1.0;
    if (m_VolumeUseSharedSettings) {
        meshManager.GetGlobalVolumeDataRange(rangeMin, rangeMax);
    } else if (mesh) {
        const double* range = mesh->GetVolumeDataRange();
        if (range) {
            rangeMin = range[0];
            rangeMax = range[1];
        }
    }
    if (rangeMin == rangeMax) {
        rangeMax = rangeMin + 1.0;
    }
    float windowMax = static_cast<float>(rangeMax - rangeMin);
    if (windowMax <= 0.0f) {
        windowMax = 1.0f;
    }

    bool levelChanged = ImGui::SliderFloat("Level##VolumeLevel", &m_UiVolumeLevel, static_cast<float>(rangeMin), static_cast<float>(rangeMax), "%.3f");
    bool windowChanged = ImGui::SliderFloat("Window##VolumeWindow", &m_UiVolumeWindow, 0.0f, windowMax, "%.3f");
    
    if (windowChanged || levelChanged) {
        if (m_UiVolumeDirectApply) {
            if (m_VolumeUseSharedSettings) {
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.window = m_UiVolumeWindow;
                settings.level = m_UiVolumeLevel;
                meshManager.SetSharedVolumeDisplaySettings(settings);
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* meshItem) {
                meshItem->SetVolumeWindowLevel(m_UiVolumeWindow, m_UiVolumeLevel);
            });
            s_HasVolumeWindowLevelChanged = false;
        } else {
            s_HasVolumeWindowLevelChanged = true;
        }
    }
    
    const bool showApplyButton = !m_UiVolumeDirectApply && s_HasVolumeWindowLevelChanged;
    const float actionRowWidth = ImGui::GetContentRegionAvail().x;
    const bool sameLineActionButtons =
        showApplyButton && actionRowWidth >= (actionButtonWidth * 2.0f + baseStyle.ItemSpacing.x);

    if (showApplyButton) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, actionButtonPadding);
        if (ImGui::Button("Apply##WindowLevel", ImVec2(actionButtonWidth, actionButtonHeight))) {
            if (m_VolumeUseSharedSettings) {
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.window = m_UiVolumeWindow;
                settings.level = m_UiVolumeLevel;
                meshManager.SetSharedVolumeDisplaySettings(settings);
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* meshItem) {
                meshItem->SetVolumeWindowLevel(m_UiVolumeWindow, m_UiVolumeLevel);
            });
            s_HasVolumeWindowLevelChanged = false;
        }
        ImGui::PopStyleVar();
        if (sameLineActionButtons) {
            ImGui::SameLine();
        }
    }
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, actionButtonPadding);
    if (ImGui::Button("Reset##WindowLevel", ImVec2(actionButtonWidth, actionButtonHeight))) {
        double resetMin = rangeMin;
        double resetMax = rangeMax;
        if (resetMin == resetMax) {
            resetMax = resetMin + 1.0;
        }
        double resetWindow = resetMax - resetMin;
        double resetLevel = (resetMin + resetMax) * 0.5;
        if (m_VolumeUseSharedSettings) {
            MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
            settings.window = resetWindow;
            settings.level = resetLevel;
            meshManager.SetSharedVolumeDisplaySettings(settings);
        }
        forEachTargetVolumeMesh(meshId, [&](Mesh* meshItem) {
            meshItem->SetVolumeWindowLevel(resetWindow, resetLevel);
        });
        SetUiVolumeWindowLevel(resetWindow, resetLevel);
        s_HasVolumeWindowLevelChanged = false;
    }
    ImGui::PopStyleVar();
    
    ImGui::PopStyleVar();
    ImGui::PopID();
}

void MeshDetail::renderTableRowVolumeQuality(int32_t meshId) {
    ImGui::TableNextColumn();
    ImGui::Text("Quality");
    ImGui::TableNextColumn();
    
    static const char* qualityLabels[] = { "Low", "Medium", "High" };
    constexpr int qualityCount = static_cast<int>(IM_ARRAYSIZE(qualityLabels));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    int qualityIndex = m_UiVolumeQuality;
    if (qualityIndex < 0) qualityIndex = 0;
    else if (qualityIndex >= qualityCount) qualityIndex = qualityCount - 1;
    
    bool isChanged = ImGui::SliderInt("##VolumeQuality", &qualityIndex, 0, qualityCount - 1);
    ImGui::SameLine();
    ImGui::Text("%s", qualityLabels[qualityIndex]);
    ImGui::PopStyleVar();

    if (isChanged) {
        m_UiVolumeQuality = qualityIndex;
        Mesh::VolumeQuality quality = static_cast<Mesh::VolumeQuality>(qualityIndex);
        if (m_VolumeUseSharedSettings) {
            MeshManager& meshManager = MeshManager::Instance();
            MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
            settings.quality = quality;
            meshManager.SetSharedVolumeDisplaySettings(settings);
        }
        forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
            mesh->SetVolumeQuality(quality);
        });
    }
}

void MeshDetail::renderTableRowVolumeSampleDistance(int32_t meshId) {
    ImGui::TableNextColumn();
    ImGui::Text("Sample Distance");
    ImGui::TableNextColumn();
    
    static const char* ratioLabels[] = { "0.1x", "0.5x", "1.0x", "2.0x", "5.0x", "10.0x" };
    constexpr int ratioCount = static_cast<int>(IM_ARRAYSIZE(ratioLabels));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    int index = m_UiVolumeSampleDistanceIndex;
    if (index < 0) index = 0;
    else if (index >= ratioCount) index = ratioCount - 1;
    
    bool isChanged = ImGui::SliderInt("##VolumeSampleDistance", &index, 0, ratioCount - 1);
    ImGui::SameLine();
    ImGui::Text("%s", ratioLabels[index]);
    ImGui::PopStyleVar();

    if (isChanged) {
        m_UiVolumeSampleDistanceIndex = index;
        if (m_UiVolumeDirectApply) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.sampleDistanceIndex = m_UiVolumeSampleDistanceIndex;
                meshManager.SetSharedVolumeDisplaySettings(settings);
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeSampleDistanceIndex(m_UiVolumeSampleDistanceIndex);
            });
            s_HasVolumeSampleDistanceChanged = false;
        } else {
            s_HasVolumeSampleDistanceChanged = true;
        }
    }

    if (!m_UiVolumeDirectApply && s_HasVolumeSampleDistanceChanged) {
        float buttonHeight = ImGui::GetTextLineHeightWithSpacing() + (ImGui::GetStyle().FramePadding.y * 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, App::TextBaseHeight() * 0.2f));
        if (ImGui::Button("Apply##SampleDistance", ImVec2(App::TextBaseWidth() * 6.0f, buttonHeight))) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.sampleDistanceIndex = m_UiVolumeSampleDistanceIndex;
                meshManager.SetSharedVolumeDisplaySettings(settings);
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeSampleDistanceIndex(m_UiVolumeSampleDistanceIndex);
            });
            s_HasVolumeSampleDistanceChanged = false;
        }
        ImGui::PopStyleVar();
    }
}

void MeshDetail::renderTableRowVolumeRenderOpacityMin(int32_t meshId, double opacity) {
    ImGui::TableNextColumn();
    ImGui::Text("Opacity (Min)");
    ImGui::TableNextColumn();
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isChanged = ImGui::DragFloat("##VolumeRenderOpacityMin", &m_UiVolumeRenderOpacityMin, 0.005f, 0.0f, 0.8f);
    if (isChanged) {
        if (m_UiVolumeDirectApply) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.opacityMin = m_UiVolumeRenderOpacityMin;
                if (settings.opacityMin > settings.opacityMax) {
                    settings.opacityMin = settings.opacityMax;
                    m_UiVolumeRenderOpacityMin = static_cast<float>(settings.opacityMin);
                }
                meshManager.SetSharedVolumeDisplaySettings(settings);
            } else if (m_UiVolumeRenderOpacityMin > m_UiVolumeRenderOpacity) {
                m_UiVolumeRenderOpacityMin = m_UiVolumeRenderOpacity;
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeRenderOpacityMin(m_UiVolumeRenderOpacityMin);
            });
            s_HasVolumeRenderOpacityMinChanged = false;
        } else {
            s_HasVolumeRenderOpacityMinChanged = true;
        }
    }
    if (!m_UiVolumeDirectApply && s_HasVolumeRenderOpacityMinChanged) {
        ImGui::SameLine();
        if (ImGui::Button("Apply", ImVec2(App::TextBaseWidth() * 6.0f, App::TextBaseHeight()))) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.opacityMin = m_UiVolumeRenderOpacityMin;
                if (settings.opacityMin > settings.opacityMax) {
                    settings.opacityMin = settings.opacityMax;
                    m_UiVolumeRenderOpacityMin = static_cast<float>(settings.opacityMin);
                }
                meshManager.SetSharedVolumeDisplaySettings(settings);
            } else if (m_UiVolumeRenderOpacityMin > m_UiVolumeRenderOpacity) {
                m_UiVolumeRenderOpacityMin = m_UiVolumeRenderOpacity;
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeRenderOpacityMin(m_UiVolumeRenderOpacityMin);
            });
            s_HasVolumeRenderOpacityMinChanged = false;
        }
    }
    ImGui::PopStyleVar();
}

void MeshDetail::renderTableRowVolumeRenderOpacity(int32_t meshId, double opacity) {
    ImGui::TableNextColumn();
    ImGui::Text("Opacity (Max)");
    ImGui::TableNextColumn();
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isChanged = ImGui::DragFloat("##VolumeRenderOpacity", &m_UiVolumeRenderOpacity, 0.005f, 0.0f, 0.8f);
    if (isChanged) {
        if (m_UiVolumeDirectApply) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.opacityMax = m_UiVolumeRenderOpacity;
                if (settings.opacityMax < settings.opacityMin) {
                    settings.opacityMin = settings.opacityMax;
                    m_UiVolumeRenderOpacityMin = static_cast<float>(settings.opacityMin);
                }
                meshManager.SetSharedVolumeDisplaySettings(settings);
            } else if (m_UiVolumeRenderOpacity < m_UiVolumeRenderOpacityMin) {
                m_UiVolumeRenderOpacityMin = m_UiVolumeRenderOpacity;
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeRenderOpacity(m_UiVolumeRenderOpacity);
            });
            s_HasVolumeRenderOpacityMaxChanged = false;
        } else {
            s_HasVolumeRenderOpacityMaxChanged = true;
        }
    }
    if (!m_UiVolumeDirectApply && s_HasVolumeRenderOpacityMaxChanged) {
        ImGui::SameLine();
        if (ImGui::Button("Apply", ImVec2(App::TextBaseWidth() * 6.0f, App::TextBaseHeight()))) {
            if (m_VolumeUseSharedSettings) {
                MeshManager& meshManager = MeshManager::Instance();
                MeshManager::VolumeDisplaySettings settings = meshManager.GetSharedVolumeDisplaySettings();
                settings.opacityMax = m_UiVolumeRenderOpacity;
                if (settings.opacityMax < settings.opacityMin) {
                    settings.opacityMin = settings.opacityMax;
                    m_UiVolumeRenderOpacityMin = static_cast<float>(settings.opacityMin);
                }
                meshManager.SetSharedVolumeDisplaySettings(settings);
            } else if (m_UiVolumeRenderOpacity < m_UiVolumeRenderOpacityMin) {
                m_UiVolumeRenderOpacityMin = m_UiVolumeRenderOpacity;
            }
            forEachTargetVolumeMesh(meshId, [&](Mesh* mesh) {
                mesh->SetVolumeRenderOpacity(m_UiVolumeRenderOpacity);
            });
            s_HasVolumeRenderOpacityMaxChanged = false;
        }
    }
    ImGui::PopStyleVar();
}

// ============================================================================
// ✅ 추가: Shared Volume Settings Sync
// ============================================================================
void MeshDetail::syncSharedVolumeUiFromSettings(const MeshManager::VolumeDisplaySettings& settings) {
    m_UiVolumeRenderMode = static_cast<int>(settings.renderMode);
    m_UiVolumeMeshVisibility = settings.volumeVisibility;
    m_UiVolumeColorPreset = static_cast<int>(settings.colorPreset);
    
    if (!s_HasVolumeColorCurveChanged) {
        m_UiVolumeColorMidpoint = static_cast<float>(settings.colorMidpoint);
        m_UiVolumeColorSharpness = static_cast<float>(settings.colorSharpness);
    }
    
    if (!s_HasVolumeWindowLevelChanged) {
        double rangeMin = 0.0;
        double rangeMax = 1.0;
        MeshManager::Instance().GetGlobalVolumeDataRange(rangeMin, rangeMax);
        if (rangeMin == rangeMax) {
            rangeMax = rangeMin + 1.0;
        }
        double windowMax = rangeMax - rangeMin;
        double windowValue = settings.window;
        if (windowValue < 0.0) windowValue = 0.0;
        else if (windowValue > windowMax) windowValue = windowMax;
        
        double levelValue = settings.level;
        if (levelValue < rangeMin) levelValue = rangeMin;
        else if (levelValue > rangeMax) levelValue = rangeMax;
        
        m_UiVolumeWindow = static_cast<float>(windowValue);
        m_UiVolumeLevel = static_cast<float>(levelValue);
    }
    
    if (!s_HasVolumeRenderOpacityMinChanged) {
        m_UiVolumeRenderOpacityMin = static_cast<float>(settings.opacityMin);
    }
    if (!s_HasVolumeRenderOpacityMaxChanged) {
        m_UiVolumeRenderOpacity = static_cast<float>(settings.opacityMax);
    }
    if (!s_HasVolumeSampleDistanceChanged) {
        m_UiVolumeSampleDistanceIndex = settings.sampleDistanceIndex;
    }
    
    m_UiVolumeQuality = static_cast<int>(settings.quality);
    
    if (!s_HasVolumeMeshColorChanged) {
        m_UiVolumeMeshColor[0] = static_cast<float>(settings.meshColor[0]);
        m_UiVolumeMeshColor[1] = static_cast<float>(settings.meshColor[1]);
        m_UiVolumeMeshColor[2] = static_cast<float>(settings.meshColor[2]);
    }
    if (!s_HasVolumeMeshOpacityChanged) {
        m_UiVolumeMeshOpacity = static_cast<float>(settings.meshOpacity);
    }
    if (!s_HasVolumeMeshEdgeColorChanged) {
        m_UiVolumeMeshEdgeColor[0] = static_cast<float>(settings.meshEdgeColor[0]);
        m_UiVolumeMeshEdgeColor[1] = static_cast<float>(settings.meshEdgeColor[1]);
        m_UiVolumeMeshEdgeColor[2] = static_cast<float>(settings.meshEdgeColor[2]);
    }
}

// ============================================================================
// Setters
// ============================================================================
void MeshDetail::SetUiEdgeMeshColor(const double* color) {
    if (color == nullptr) return;
    m_UiEdgeMeshColor[0] = static_cast<float>(color[0]);
    m_UiEdgeMeshColor[1] = static_cast<float>(color[1]);
    m_UiEdgeMeshColor[2] = static_cast<float>(color[2]);
}

void MeshDetail::SetUiEdgeMeshOpacity(double opacity) {
    m_UiEdgeMeshOpacity = static_cast<float>(opacity);
}

void MeshDetail::SetUiFaceMeshColor(const double* color) {
    if (color == nullptr) return;
    m_UiFaceMeshColor[0] = static_cast<float>(color[0]);
    m_UiFaceMeshColor[1] = static_cast<float>(color[1]);
    m_UiFaceMeshColor[2] = static_cast<float>(color[2]);
}

void MeshDetail::SetUiFaceMeshOpacity(double opacity) {
    m_UiFaceMeshOpacity = static_cast<float>(opacity);
}

void MeshDetail::SetUiFaceMeshEdgeColor(const double* color) {
    if (color == nullptr) return;
    m_UiFaceMeshEdgeColor[0] = static_cast<float>(color[0]);
    m_UiFaceMeshEdgeColor[1] = static_cast<float>(color[1]);
    m_UiFaceMeshEdgeColor[2] = static_cast<float>(color[2]);
}

void MeshDetail::SetUiVolumeMeshColor(const double* color) {
    if (color == nullptr) return;
    m_UiVolumeMeshColor[0] = static_cast<float>(color[0]);
    m_UiVolumeMeshColor[1] = static_cast<float>(color[1]);
    m_UiVolumeMeshColor[2] = static_cast<float>(color[2]);
}

void MeshDetail::SetUiVolumeMeshOpacity(double opacity) {
    m_UiVolumeMeshOpacity = static_cast<float>(opacity);
}

void MeshDetail::SetUiVolumeMeshEdgeColor(const double* color) {
    if (color == nullptr) return;
    m_UiVolumeMeshEdgeColor[0] = static_cast<float>(color[0]);
    m_UiVolumeMeshEdgeColor[1] = static_cast<float>(color[1]);
    m_UiVolumeMeshEdgeColor[2] = static_cast<float>(color[2]);
}

void MeshDetail::SetUiVolumeWindowLevel(double window, double level) {
    m_UiVolumeWindow = static_cast<float>(window);
    m_UiVolumeLevel = static_cast<float>(level);
}

void MeshDetail::SetUiVolumeRenderOpacity(double opacity) {
    m_UiVolumeRenderOpacity = static_cast<float>(opacity);
}

void MeshDetail::SetUiVolumeRenderOpacityMin(double opacity) {
    m_UiVolumeRenderOpacityMin = static_cast<float>(opacity);
}

void MeshDetail::SetUiVolumeColorCurve(double midpoint, double sharpness) {
    m_UiVolumeColorMidpoint = static_cast<float>(midpoint);
    m_UiVolumeColorSharpness = static_cast<float>(sharpness);
}
