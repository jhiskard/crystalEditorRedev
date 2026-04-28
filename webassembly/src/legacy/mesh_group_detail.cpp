#include "mesh_group_detail.h"
#include "mesh.h"
#include "app.h"
#include "font_manager.h"
#include "mesh_manager.h"

// ImGui
#include <imgui.h>


int32_t MeshGroupDetail::s_SelectedMeshGroupId = -1;
int32_t MeshGroupDetail::s_DeleteMeshGroupId = -1;
bool MeshGroupDetail::s_HasPointSizeChanged = false;
bool MeshGroupDetail::s_HasGroupColorChanged = false;

MeshGroupDetail::MeshGroupDetail() {
}

MeshGroupDetail::~MeshGroupDetail() {
}

void MeshGroupDetail::Render(int32_t meshId) {
    if (meshId == -1) {
        return;
    }

    Mesh* mesh = MeshManager::Instance().GetMeshByIdMutable(meshId);
    if (mesh == nullptr) {
        return;
    }


    if (mesh->GetMeshGroupCount() > 0) {
        ImGui::Begin(ICON_FA6_FOLDER_OPEN"  Mesh Group");        

        static ImGuiTableFlags tableFlags =
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_NoBordersInBodyUntilResize |
            ImGuiTableFlags_Reorderable |  // Reorder columns by dragging
            ImGuiTableFlags_PadOuterX |
            ImGuiTableFlags_BordersOuter;

        constexpr int32_t TABLE_COLUMN_COUNT = 5;
        static const char* tableHeaders[TABLE_COLUMN_COUNT] = { "Name", "Id", "Type", ICON_FA6_EYE, ICON_FA6_TRASH_CAN };
        static ImGuiTableColumnFlags tableColumnFlags[TABLE_COLUMN_COUNT] = {
            ImGuiTableColumnFlags_WidthStretch,
            ImGuiTableColumnFlags_WidthFixed,
            ImGuiTableColumnFlags_WidthFixed,
            ImGuiTableColumnFlags_WidthFixed,
            ImGuiTableColumnFlags_WidthFixed
        };
        static float tableColumnSizes[TABLE_COLUMN_COUNT] = { 30.0f, 3.0f, 7.0f, 2.5f, 2.5f };

        static ImGuiTreeNodeFlags flagNoOpen = 
            ImGuiTreeNodeFlags_Leaf | 
            ImGuiTreeNodeFlags_NoTreePushOnOpen | 
            ImGuiTreeNodeFlags_Bullet | 
            ImGuiTreeNodeFlags_SpanFullWidth;

        bool openDisplay = ImGui::CollapsingHeader("Mesh Groups", ImGuiTreeNodeFlags_DefaultOpen);
        if (openDisplay && ImGui::BeginTable("MeshGroupDisplay", TABLE_COLUMN_COUNT, tableFlags)) {
            // Column headers
            for (int iCol = 0; iCol < TABLE_COLUMN_COUNT; iCol++) {
                ImGui::TableSetupColumn(tableHeaders[iCol], tableColumnFlags[iCol], App::TextBaseWidth() * tableColumnSizes[iCol]);
            }

            // Render the headers with the custom style
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            for (int iCol = 0; iCol < TABLE_COLUMN_COUNT; iCol++) {
                ImGui::TableSetColumnIndex(iCol);
                ImGui::PushID(iCol);
                ImGui::TableHeader(tableHeaders[iCol]);
                if (iCol == 3) {
                    App::AddTooltip("Show/Hide Mesh Group");
                }
                else if (iCol == 4) {
                    App::AddTooltip("Delete Mesh Group", "Double-click to delete the mesh group");
                }
                ImGui::PopID();
            }

            for (const MeshGroupUPtr& meshGroup: mesh->GetMeshGroups()) {
                ImGuiTreeNodeFlags selectFlag = s_SelectedMeshGroupId == meshGroup->GetId() ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;
                
                ImGui::TableNextRow();

                // Column 0: Mesh group name
                ImGui::TableSetColumnIndex(0);
                std::string meshGroupId = std::to_string(meshGroup->GetId());
                switch (meshGroup->GetGroupType()) {
                    case MeshGroupType::NODE:
                        ImGui::TreeNodeEx(meshGroupId.c_str(), flagNoOpen | selectFlag, ICON_FA6_GRIP_VERTICAL"  %s", meshGroup->GetName());
                        break;
                    case MeshGroupType::EDGE:
                        ImGui::TreeNodeEx(meshGroupId.c_str(), flagNoOpen | selectFlag, ICON_FA6_GRIP_LINES"  %s", meshGroup->GetName());
                        break;
                    case MeshGroupType::FACE:
                        ImGui::TreeNodeEx(meshGroupId.c_str(), flagNoOpen | selectFlag, ICON_FA6_CIRCLE_NODES"  %s", meshGroup->GetName());
                        break;
                    case MeshGroupType::VOLUME:
                        ImGui::TreeNodeEx(meshGroupId.c_str(), flagNoOpen | selectFlag, ICON_FA6_DICE_D6"  %s", meshGroup->GetName());
                        break;
                    default:
                        assert("Invalid mesh group type!" && false);
                        break;
                }

                // Add click event for the mesh group
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    if (s_SelectedMeshGroupId != meshGroup->GetId()) {
                        s_SelectedMeshGroupId = meshGroup->GetId();

                        SetUiPointSize(meshGroup->GetPointSize());
                        SetUiGroupColor(meshGroup->GetGroupColor());
                    }
                    else {
                        // If the mesh group is already selected, deselect it.
                        s_SelectedMeshGroupId = -1;
                    }
                }

                // Column 1: Mesh group id
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d", meshGroup->GetId());

                // Column 2: Mesh group type
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", MeshGroup::GetMeshGroupTypeStr(meshGroup->GetGroupType()));

                // Column 3: Show/Hide mesh group icon
                ImGui::TableSetColumnIndex(3);
                ImVec4 curColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);  // Get current text color
                if (meshGroup->GetVisibility()) {
                    ImGui::TextColored(curColor, ICON_FA6_EYE);
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                        meshGroup->SetVisibility(false);
                    }
                }
                else {
                    ImGui::TextColored(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                        meshGroup->SetVisibility(true);
                    }
                }

                // Column 4: Delete mesh group icon
                ImGui::TableSetColumnIndex(4);
                ImGui::TextColored(ImVec4(0.7f, 0.0f, 0.0f, 1.0f), ICON_FA6_TRASH_CAN);
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (s_SelectedMeshGroupId == meshGroup->GetId()) {
                        // If the selected mesh is the same as the one to be deleted, deselect it.
                        // This is to prevent the mesh detail window from showing the deleted mesh.
                        s_SelectedMeshGroupId = -1;
                    }
                    s_DeleteMeshGroupId = meshGroup->GetId();
                }
            }

            ImGui::EndTable();
        }

        if (s_SelectedMeshGroupId != -1) {
            MeshGroup* selectedMeshGroup = mesh->GetMeshGroupByIdMutable(s_SelectedMeshGroupId);
            if (selectedMeshGroup) {
                const int32_t tableColumeCount = 2;

                ImGuiTableFlags tableFlags =
                    ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_PadOuterX |
                    ImGuiTableFlags_Borders |
                    ImGuiTableFlags_RowBg;

                bool openBasic = ImGui::CollapsingHeader("Basic Infomation", ImGuiTreeNodeFlags_DefaultOpen);
                if (openBasic && ImGui::BeginTable("Basic Infomation", tableColumeCount, tableFlags)) {
                    // Header Setup (Not displayed. Only set the column widths)
                    ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, App::TextBaseWidth() * 10.0f);
                    ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch, App::TextBaseWidth() * 15.0f);

                    renderTableRow("Id", selectedMeshGroup->GetId());
                    renderTableRow("Name", selectedMeshGroup->GetName());
                    renderTableRow("Type", MeshGroup::GetMeshGroupTypeStr(selectedMeshGroup->GetGroupType()));

                    ImGui::EndTable();
                }

                bool openDisplay = ImGui::CollapsingHeader("Group Display", ImGuiTreeNodeFlags_DefaultOpen);
                if (openDisplay && ImGui::BeginTable("FaceMeshDisplay", tableColumeCount, tableFlags)) {
                    // Header Setup (Not displayed. Only set the column widths)
                    ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, App::TextBaseWidth() * 10.0f);
                    ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch, App::TextBaseWidth() * 20.0f);

                    if (selectedMeshGroup->GetGroupType() == MeshGroupType::NODE) {
                        renderTableRowPointSize(selectedMeshGroup);
                        renderTableRowGroupColor(selectedMeshGroup);
                    }
                    // TODO: Add other mesh group types

                    ImGui::EndTable();
                }
            }
        }

        if (s_DeleteMeshGroupId != -1) {
            mesh->DeleteMeshGroup(s_DeleteMeshGroupId);
            s_DeleteMeshGroupId = -1;  // Reset the delete mesh group id
        }
        else {
            // Deselect when the mouse is double-clicked in the mesh detail window
            // Only works when the mouse double-click is not for deleting the mesh group (s_DeleteMeshGroupId == -1)
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsWindowFocused()) {
                s_SelectedMeshGroupId = -1;
            }
        }

        ImGui::End();
    }


}

void MeshGroupDetail::renderTableRow(const char* item, int32_t value) {
    ImGui::TableNextColumn();
    ImGui::Text("%s", item);

    ImGui::TableNextColumn();
    ImGui::Text("%d", value);
}

void MeshGroupDetail::renderTableRow(const char* item, const char* value) {
    ImGui::TableNextColumn();
    ImGui::Text("%s", item);

    ImGui::TableNextColumn();
    if (value) {
        ImGui::Text("%s", value);
    }
    else {
        ImGui::TextDisabled("--");
    }
}

void MeshGroupDetail::renderTableRow(const char* item, double value) {
    ImGui::TableNextColumn();
    ImGui::Text("%s", item);

    ImGui::TableNextColumn();
    ImGui::Text("%.3lf", value);
}

void MeshGroupDetail::SetUiGroupColor(const double* color) {
    if (color == nullptr) {
        return;
    }
    m_UiGroupColor[0] = static_cast<float>(color[0]);
    m_UiGroupColor[1] = static_cast<float>(color[1]);
    m_UiGroupColor[2] = static_cast<float>(color[2]);
}

void MeshGroupDetail::renderTableRowPointSize(MeshGroup* group) {
    ImGui::TableNextColumn();
    ImGui::Text("Point Size");

    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isChanged = ImGui::DragFloat("##PointSize", &m_UiPointSize, 0.1f, 1.0f, 20.0f);
    if (isChanged) {
        s_HasPointSizeChanged = true;
    }
    if (s_HasPointSizeChanged) {
        ImGui::SameLine();
        if (ImGui::Button("Apply", ImVec2(App::TextBaseWidth() * 6.0f, App::TextBaseHeight()))) {
            group->SetPointSize(m_UiPointSize);
            s_HasPointSizeChanged = false;
        }
    }
    ImGui::PopStyleVar();
}

void MeshGroupDetail::renderTableRowGroupColor(MeshGroup* group) {
    ImGui::TableNextColumn();
    ImGui::Text("Point Color");

    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool isChanged = ImGui::ColorEdit3("##GroupColor", m_UiGroupColor, ImGuiColorEditFlags_Float);
    if (isChanged) {
        s_HasGroupColorChanged = true;
    }
    if (s_HasGroupColorChanged) {
        ImGui::SameLine();
        if (ImGui::Button("Apply", ImVec2(App::TextBaseWidth() * 6.0f, App::TextBaseHeight()))) {
            group->SetGroupColor(m_UiGroupColor[0], m_UiGroupColor[1], m_UiGroupColor[2]);
            s_HasGroupColorChanged = false;
        }
    }
    ImGui::PopStyleVar();
}