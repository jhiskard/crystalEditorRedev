#include "model_tree.h"
#include "app.h"
#include "font_manager.h"
#include "mesh_manager.h"
#include "lcrs_tree.h"
#include "mesh_detail.h"

// ImGui
#include <imgui.h>
#include "atoms/atoms_template.h"
#include "atoms/ui/charge_density_ui.h"
#include "atoms/domain/bond_manager.h"
#include "atoms/domain/atom_manager.h"
using atoms::domain::bondGroups;
using atoms::domain::createdAtoms;
using atoms::domain::createdBonds;
using atoms::domain::surroundingAtoms;
using atoms::domain::surroundingBonds;

int32_t ModelTree::s_DeleteMeshId = -1;
int32_t ModelTree::s_SelectedMeshId = -1;
int32_t ModelTree::s_PendingDeleteMeshId = -1;
bool ModelTree::s_ShowDeleteConfirmPopup = false;
int32_t ModelTree::s_PendingClearMeasurementsStructureId = -1;
bool ModelTree::s_ShowClearMeasurementsConfirmPopup = false;

ModelTree::ModelTree() {
}

ModelTree::~ModelTree() {
}

// 헬퍼 함수
void TextColoredCentered(const ImVec4& color, const char* text) {
    float columnWidth = ImGui::GetColumnWidth();
    float textWidth = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f);
    ImGui::TextColored(color, "%s", text);
}

void TextCentered(const char* text) {
    float columnWidth = ImGui::GetColumnWidth();
    float textWidth = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f);
    ImGui::Text("%s", text);
}

void TextCenteredInt(int value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", value);
    float columnWidth = ImGui::GetColumnWidth();
    float textWidth = ImGui::CalcTextSize(buf).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f);
    ImGui::Text("%s", buf);
}

void TextCenteredSizeT(size_t value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%zu", value);
    float columnWidth = ImGui::GetColumnWidth();
    float textWidth = ImGui::CalcTextSize(buf).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f);
    ImGui::Text("%s", buf);
}

void ModelTree::Render(bool* openWindow) {
    ImGui::Begin(ICON_FA6_FOLDER_TREE"  Model Tree", openWindow, ImGuiWindowFlags_NoScrollbar);
    
    static ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_NoBordersInBodyUntilResize |
        ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_PadOuterX |
        ImGuiTableFlags_BordersOuter;

    // Mesh Tree 렌더링
    renderMeshTable(tableFlags);
    
    // Crystal Structure 섹션
    renderXsfStructureTable(tableFlags);
    
    // 삭제 확인 팝업 렌더링
    renderDeleteConfirmPopup();
    renderClearMeasurementsConfirmPopup();

    ImGui::End();
}

void ModelTree::renderDeleteConfirmPopup() {
    if (s_ShowDeleteConfirmPopup) {
        ImGui::OpenPopup("Delete Confirmation");
    }
    
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("Delete Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        std::string meshName = "Unknown";
        bool isXsfStructure = false;
        if (s_PendingDeleteMeshId != -1) {
            const Mesh* mesh = MeshManager::Instance().GetMeshById(s_PendingDeleteMeshId);
            if (mesh) {
                meshName = mesh->GetName();
                isXsfStructure = mesh->IsXsfStructure();
            }
        }
        
        ImGui::Text("%s will be removed.", meshName.c_str());
        ImGui::Text("Are you sure?");
        ImGui::Separator();
        
        float buttonWidth = 120.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalWidth = buttonWidth * 2 + spacing;
        float startX = (ImGui::GetWindowSize().x - totalWidth) * 0.5f;
        
        ImGui::SetCursorPosX(startX);

        if (ImGui::Button("Yes", ImVec2(buttonWidth, 0))) {
            if (s_PendingDeleteMeshId != -1) {
                if (isXsfStructure) {
                    if (s_SelectedMeshId == s_PendingDeleteMeshId) {
                        s_SelectedMeshId = -1;
                    }
                    AtomsTemplate::Instance().RemoveStructure(s_PendingDeleteMeshId);
                    MeshManager::Instance().DeleteXsfStructure(s_PendingDeleteMeshId);
                } else {
                    if (s_SelectedMeshId == s_PendingDeleteMeshId) {
                        s_SelectedMeshId = -1;
                    }
                    MeshManager::Instance().DeleteMesh(s_PendingDeleteMeshId);
                }
                s_PendingDeleteMeshId = -1;
            }
            
            s_ShowDeleteConfirmPopup = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("No", ImVec2(buttonWidth, 0))) {
            s_PendingDeleteMeshId = -1;
            s_ShowDeleteConfirmPopup = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void ModelTree::renderClearMeasurementsConfirmPopup() {
    if (s_ShowClearMeasurementsConfirmPopup) {
        ImGui::OpenPopup("Clear Measurements Confirmation");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Clear Measurements Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("All Measurement entries will be removed.");
        ImGui::Text("Are you sure?");
        ImGui::Separator();

        float buttonWidth = 120.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalWidth = buttonWidth * 2 + spacing;
        float startX = (ImGui::GetWindowSize().x - totalWidth) * 0.5f;

        ImGui::SetCursorPosX(startX);
        if (ImGui::Button("Yes", ImVec2(buttonWidth, 0))) {
            if (s_PendingClearMeasurementsStructureId != -1) {
                AtomsTemplate::Instance().RemoveMeasurementsByStructure(
                    s_PendingClearMeasurementsStructureId);
            }
            s_PendingClearMeasurementsStructureId = -1;
            s_ShowClearMeasurementsConfirmPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(buttonWidth, 0))) {
            s_PendingClearMeasurementsStructureId = -1;
            s_ShowClearMeasurementsConfirmPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ModelTree::renderXsfStructureTable(ImGuiTableFlags tableFlags) {
    AtomsTemplate& atomsTemplate = AtomsTemplate::Instance();
    auto structures = atomsTemplate.GetStructures();

    if (structures.empty() && !atomsTemplate.HasChargeDensity()) {
        return;
    }
    
    constexpr int32_t TABLE_COLUMN_COUNT = 5;
    static const char* tableHeaders[TABLE_COLUMN_COUNT] = { "Name", "Show", "Label", "Count", "Remove" };
    static ImGuiTableColumnFlags tableColumnFlags[TABLE_COLUMN_COUNT] = {
        ImGuiTableColumnFlags_WidthStretch,
        ImGuiTableColumnFlags_WidthFixed,
        ImGuiTableColumnFlags_WidthFixed,
        ImGuiTableColumnFlags_WidthFixed,
        ImGuiTableColumnFlags_WidthFixed
    };
    const ImGuiTableFlags xsfTableFlags = tableFlags | ImGuiTableFlags_SizingFixedFit;
    bool hasChargeDensityVisibilityChange = false;
    auto setChargeDensityContext = [&](int32_t structureId) {
        atomsTemplate.SetCurrentStructureId(structureId);
        atomsTemplate.SetChargeDensityStructureId(structureId);
    };
    auto openChargeDensityViewer = [&](int32_t structureId, AtomsTemplate::DataMenuRequest request) {
        setChargeDensityContext(structureId);
        App::ShowAdvancedViewWindow(true);
        atomsTemplate.RequestDataMenu(request);
        hasChargeDensityVisibilityChange = true;
    };
    auto openSliceViewer = [&](int32_t structureId) {
        setChargeDensityContext(structureId);
        App::ShowSliceViewerWindow(true);
        atomsTemplate.RequestDataMenu(AtomsTemplate::DataMenuRequest::Plane);
        hasChargeDensityVisibilityChange = true;
    };
    auto renderLabelState = [&](bool allVisible, bool anyVisible, auto&& onToggle) {
        ImVec4 curColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        if (allVisible) {
            TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                onToggle(false);
            }
        } else if (anyVisible) {
            TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.65f), ICON_FA6_EYE);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                onToggle(true);
            }
        } else {
            TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                onToggle(true);
            }
        }
    };
    bool isOpen = ImGui::CollapsingHeader("Crystal Structure", ImGuiTreeNodeFlags_DefaultOpen);
    if (isOpen && ImGui::BeginTable("XsfStructure_Table", TABLE_COLUMN_COUNT, xsfTableFlags)) {
        for (int iCol = 0; iCol < TABLE_COLUMN_COUNT; iCol++) {
            ImGui::TableSetupColumn(tableHeaders[iCol], tableColumnFlags[iCol], 0.0f);
        }
        
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        for (int iCol = 0; iCol < TABLE_COLUMN_COUNT; iCol++) {
            ImGui::TableSetColumnIndex(iCol);
            ImGui::PushID(iCol + 100);
            ImGui::TableHeader(tableHeaders[iCol]);
            ImGui::PopID();
        }

        for (const auto& entry : structures) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            bool structureVisible = atomsTemplate.IsStructureVisible(entry.id);
            ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                           ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                           ImGuiTreeNodeFlags_DefaultOpen |
                                           ImGuiTreeNodeFlags_SpanFullWidth;
            std::string treeId = "##XsfRoot" + std::to_string(entry.id);
            bool isTreeOpen = ImGui::TreeNodeEx(treeId.c_str(), rootFlags,
                                                ICON_FA6_ATOM"  %s", entry.name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImVec4 curColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
            if (structureVisible) {
                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    atomsTemplate.SetStructureVisible(entry.id, false);
                    MeshManager::Instance().HideMesh(entry.id);
                    if (atomsTemplate.HasChargeDensity() &&
                        atomsTemplate.GetChargeDensityStructureId() == entry.id) {
                        hasChargeDensityVisibilityChange = true;
                    }
                }
            } else {
                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    atomsTemplate.SetStructureVisible(entry.id, true);
                    MeshDetail::Instance().SetUiVolumeMeshVisibility(true);
                    MeshManager::Instance().ShowMesh(entry.id);
                    atomsTemplate.ApplyChargeDensityAdvancedGridVisibilityForStructure(entry.id);
                    if (atomsTemplate.HasChargeDensity() &&
                        atomsTemplate.GetChargeDensityStructureId() == entry.id) {
                        hasChargeDensityVisibilityChange = true;
                    }
                }
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::TextDisabled("--");

            ImGui::TableSetColumnIndex(3);
            size_t totalAtoms = atomsTemplate.GetAtomCountForStructure(entry.id);
            TextCenteredSizeT(totalAtoms);

            ImGui::TableSetColumnIndex(4);
            ImGui::TextDisabled("--");

            if (isTreeOpen) {
                ImGui::BeginDisabled(!structureVisible);
                struct AtomRowEntry {
                    const atoms::domain::AtomInfo* atom = nullptr;
                    size_t serial = 0;
                    std::vector<const atoms::domain::AtomInfo*> surroundingChildren;
                };

                std::map<std::string, std::vector<AtomRowEntry>> atomsBySymbol;
                std::map<uint32_t, size_t> originalSerialByAtomId;
                std::map<uint32_t, std::vector<const atoms::domain::AtomInfo*>> surroundingByOriginalAtomId;
                std::vector<uint32_t> structureAtomIds;
                for (size_t atomIndex = 0; atomIndex < createdAtoms.size(); ++atomIndex) {
                    const auto& atom = createdAtoms[atomIndex];
                    if (atom.structureId != entry.id) {
                        continue;
                    }
                    atomsBySymbol[atom.symbol].push_back(AtomRowEntry{ &atom, atomIndex + 1, {} });
                    if (atom.id != 0) {
                        originalSerialByAtomId[atom.id] = atomIndex + 1;
                        structureAtomIds.push_back(atom.id);
                    }
                }
                for (const auto& surroundingAtom : surroundingAtoms) {
                    if (surroundingAtom.structureId != entry.id) {
                        continue;
                    }
                    if (surroundingAtom.id != 0) {
                        structureAtomIds.push_back(surroundingAtom.id);
                    }
                    if (surroundingAtom.originalAtomId != 0) {
                        surroundingByOriginalAtomId[surroundingAtom.originalAtomId].push_back(&surroundingAtom);
                    }
                }

                for (auto& [symbol, atomEntries] : atomsBySymbol) {
                    (void)symbol;
                    for (auto& atomEntry : atomEntries) {
                        if (!atomEntry.atom || atomEntry.atom->id == 0) {
                            continue;
                        }
                        auto it = surroundingByOriginalAtomId.find(atomEntry.atom->id);
                        if (it != surroundingByOriginalAtomId.end()) {
                            atomEntry.surroundingChildren = it->second;
                        }
                    }
                }

                if (!structureAtomIds.empty()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGuiTreeNodeFlags atomsRootFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                                        ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                        ImGuiTreeNodeFlags_DefaultOpen |
                                                        ImGuiTreeNodeFlags_SpanFullWidth;
                    const std::string atomsRootId = "##AtomsRoot_" + std::to_string(entry.id);
                    const bool atomsRootOpen = ImGui::TreeNodeEx(
                        atomsRootId.c_str(), atomsRootFlags, ICON_FA6_ATOM"  Atoms");

                    bool anyAtomVisible = false;
                    bool allAtomVisible = true;
                    for (uint32_t atomId : structureAtomIds) {
                        const bool visible = atomsTemplate.IsAtomVisibleById(atomId);
                        anyAtomVisible = anyAtomVisible || visible;
                        allAtomVisible = allAtomVisible && visible;
                    }

                    ImGui::TableSetColumnIndex(1);
                    if (allAtomVisible) {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            atomsTemplate.SetAtomVisibilityForIds(structureAtomIds, false);
                        }
                    } else if (anyAtomVisible) {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.65f), ICON_FA6_EYE);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            atomsTemplate.SetAtomVisibilityForIds(structureAtomIds, true);
                        }
                    } else {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            atomsTemplate.SetAtomVisibilityForIds(structureAtomIds, true);
                        }
                    }

                    ImGui::TableSetColumnIndex(2);
                    bool anyAtomLabelVisible = false;
                    bool allAtomLabelVisible = !structureAtomIds.empty();
                    for (uint32_t atomId : structureAtomIds) {
                        const bool labelVisible = atomsTemplate.IsAtomLabelVisibleById(atomId);
                        anyAtomLabelVisible = anyAtomLabelVisible || labelVisible;
                        allAtomLabelVisible = allAtomLabelVisible && labelVisible;
                    }
                    renderLabelState(allAtomLabelVisible, anyAtomLabelVisible, [&](bool visible) {
                        atomsTemplate.SetAtomLabelVisibilityForIds(structureAtomIds, visible);
                    });

                    ImGui::TableSetColumnIndex(3);
                    TextCenteredSizeT(structureAtomIds.size());

                    if (atomsRootOpen) {
                        for (const auto& [symbol, atomEntries] : atomsBySymbol) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGuiTreeNodeFlags atomGroupFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                                                ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                                ImGuiTreeNodeFlags_SpanFullWidth;
                            const std::string atomGroupId = "##Atom_" + symbol + std::to_string(entry.id);
                            const bool atomGroupOpen = ImGui::TreeNodeEx(
                                atomGroupId.c_str(), atomGroupFlags, ICON_FA6_CIRCLE"  %s", symbol.c_str());

                            std::vector<uint32_t> symbolAtomIds;
                            size_t symbolDisplayCount = 0;
                            for (const auto& atomEntry : atomEntries) {
                                symbolDisplayCount += 1;
                                symbolDisplayCount += atomEntry.surroundingChildren.size();
                            }
                            symbolAtomIds.reserve(symbolDisplayCount);
                            bool anySymbolVisible = false;
                            bool allSymbolVisible = symbolDisplayCount > 0;
                            for (const auto& atomEntry : atomEntries) {
                                if (!atomEntry.atom || atomEntry.atom->id == 0) {
                                    continue;
                                }
                                symbolAtomIds.push_back(atomEntry.atom->id);
                                const bool visible = atomsTemplate.IsAtomVisibleById(atomEntry.atom->id);
                                anySymbolVisible = anySymbolVisible || visible;
                                allSymbolVisible = allSymbolVisible && visible;

                                for (const auto* surroundingChild : atomEntry.surroundingChildren) {
                                    if (!surroundingChild || surroundingChild->id == 0) {
                                        continue;
                                    }
                                    symbolAtomIds.push_back(surroundingChild->id);
                                    const bool childVisible = atomsTemplate.IsAtomVisibleById(surroundingChild->id);
                                    anySymbolVisible = anySymbolVisible || childVisible;
                                    allSymbolVisible = allSymbolVisible && childVisible;
                                }
                            }

                            ImGui::TableSetColumnIndex(1);
                            if (allSymbolVisible) {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    atomsTemplate.SetAtomVisibilityForIds(symbolAtomIds, false);
                                }
                            } else if (anySymbolVisible) {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.65f), ICON_FA6_EYE);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    atomsTemplate.SetAtomVisibilityForIds(symbolAtomIds, true);
                                }
                            } else {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    atomsTemplate.SetAtomVisibilityForIds(symbolAtomIds, true);
                                }
                            }

                            ImGui::TableSetColumnIndex(2);
                            bool anySymbolLabelVisible = false;
                            bool allSymbolLabelVisible = !symbolAtomIds.empty();
                            for (uint32_t atomId : symbolAtomIds) {
                                const bool labelVisible = atomsTemplate.IsAtomLabelVisibleById(atomId);
                                anySymbolLabelVisible = anySymbolLabelVisible || labelVisible;
                                allSymbolLabelVisible = allSymbolLabelVisible && labelVisible;
                            }
                            renderLabelState(allSymbolLabelVisible, anySymbolLabelVisible, [&](bool visible) {
                                atomsTemplate.SetAtomLabelVisibilityForIds(symbolAtomIds, visible);
                            });

                            ImGui::TableSetColumnIndex(3);
                            TextCenteredSizeT(symbolDisplayCount);

                            if (atomGroupOpen) {
                                for (const auto& atomEntry : atomEntries) {
                                    if (!atomEntry.atom) {
                                        continue;
                                    }
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    const bool hasSurroundingChildren = !atomEntry.surroundingChildren.empty();
                                    ImGuiTreeNodeFlags atomLeafFlags = ImGuiTreeNodeFlags_SpanFullWidth |
                                                                       ImGuiTreeNodeFlags_OpenOnArrow |
                                                                       ImGuiTreeNodeFlags_OpenOnDoubleClick;
                                    if (!hasSurroundingChildren) {
                                        atomLeafFlags |= ImGuiTreeNodeFlags_Leaf |
                                                         ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                                         ImGuiTreeNodeFlags_Bullet;
                                    }
                                    const std::string atomNodeId = "##AtomInstance_" + symbol + "_" +
                                                                   std::to_string(atomEntry.serial) + "_" +
                                                                   std::to_string(entry.id);
                                    const std::string atomLabel =
                                        symbol + "#" + std::to_string(atomEntry.serial);
                                    const bool atomNodeOpen = ImGui::TreeNodeEx(
                                        atomNodeId.c_str(),
                                        atomLeafFlags,
                                        ICON_FA6_CIRCLE"  %s",
                                        atomLabel.c_str());

                                    ImGui::TableSetColumnIndex(1);
                                    const bool atomVisible = atomsTemplate.IsAtomVisibleById(atomEntry.atom->id);
                                    if (atomVisible) {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f),
                                                            ICON_FA6_EYE);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                            atomsTemplate.SetAtomVisibleById(atomEntry.atom->id, false);
                                        }
                                    } else {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f),
                                                            ICON_FA6_EYE_SLASH);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                            atomsTemplate.SetAtomVisibleById(atomEntry.atom->id, true);
                                        }
                                    }

                                    ImGui::TableSetColumnIndex(2);
                                    const bool atomLabelVisible = atomsTemplate.IsAtomLabelVisibleById(atomEntry.atom->id);
                                    if (atomLabelVisible) {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f),
                                                            ICON_FA6_EYE);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                            atomsTemplate.SetAtomLabelVisibleById(atomEntry.atom->id, false);
                                        }
                                    } else {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f),
                                                            ICON_FA6_EYE_SLASH);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                            atomsTemplate.SetAtomLabelVisibleById(atomEntry.atom->id, true);
                                        }
                                    }

                                    ImGui::TableSetColumnIndex(3);
                                    if (hasSurroundingChildren) {
                                        TextCenteredSizeT(atomEntry.surroundingChildren.size());
                                    } else {
                                        ImGui::TextDisabled("--");
                                    }

                                    if (hasSurroundingChildren && atomNodeOpen) {
                                        for (const auto* surroundingChild : atomEntry.surroundingChildren) {
                                            if (!surroundingChild) {
                                                continue;
                                            }

                                            ImGui::TableNextRow();
                                            ImGui::TableSetColumnIndex(0);
                                            ImGuiTreeNodeFlags surroundingLeafFlags = ImGuiTreeNodeFlags_Leaf |
                                                                                     ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                                                                     ImGuiTreeNodeFlags_Bullet |
                                                                                     ImGuiTreeNodeFlags_SpanFullWidth;

                                            size_t originSerial = atomEntry.serial;
                                            bool hasOriginSerial = true;
                                            if (surroundingChild->originalAtomId != 0) {
                                                auto itSerial = originalSerialByAtomId.find(surroundingChild->originalAtomId);
                                                if (itSerial != originalSerialByAtomId.end()) {
                                                    originSerial = itSerial->second;
                                                } else {
                                                    hasOriginSerial = false;
                                                }
                                            }

                                            const std::string surroundingNodeId =
                                                "##SurroundingAtom_" + symbol + "_" +
                                                std::to_string(atomEntry.serial) + "_" +
                                                std::to_string(entry.id) + "_" +
                                                std::to_string(surroundingChild->id);
                                            const std::string surroundingLabel = hasOriginSerial
                                                ? (surroundingChild->symbol + "(#" + std::to_string(originSerial) + ")")
                                                : (surroundingChild->symbol + "(#?)");
                                            ImGui::TreeNodeEx(surroundingNodeId.c_str(),
                                                              surroundingLeafFlags,
                                                              ICON_FA6_CIRCLE"  %s",
                                                              surroundingLabel.c_str());

                                            ImGui::TableSetColumnIndex(1);
                                            const bool surroundingVisible =
                                                atomsTemplate.IsAtomVisibleById(surroundingChild->id);
                                            if (surroundingVisible) {
                                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f),
                                                                    ICON_FA6_EYE);
                                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                                    atomsTemplate.SetAtomVisibleById(surroundingChild->id, false);
                                                }
                                            } else {
                                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f),
                                                                    ICON_FA6_EYE_SLASH);
                                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                                    atomsTemplate.SetAtomVisibleById(surroundingChild->id, true);
                                                }
                                            }

                                            ImGui::TableSetColumnIndex(2);
                                            const bool surroundingLabelVisible =
                                                atomsTemplate.IsAtomLabelVisibleById(surroundingChild->id);
                                            if (surroundingLabelVisible) {
                                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f),
                                                                    ICON_FA6_EYE);
                                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                                    atomsTemplate.SetAtomLabelVisibleById(surroundingChild->id, false);
                                                }
                                            } else {
                                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f),
                                                                    ICON_FA6_EYE_SLASH);
                                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                                    atomsTemplate.SetAtomLabelVisibleById(surroundingChild->id, true);
                                                }
                                            }

                                            ImGui::TableSetColumnIndex(3);
                                            ImGui::TextDisabled("--");
                                        }
                                        ImGui::TreePop();
                                    }
                                }
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                }

                auto resolveBondAtomByIndex = [&](const atoms::domain::BondInfo& bond, int atomIndex, uint32_t atomId)
                    -> const atoms::domain::AtomInfo* {
                    if (atomId != 0) {
                        for (const auto& atom : createdAtoms) {
                            if (atom.id == atomId) {
                                return &atom;
                            }
                        }
                        for (const auto& atom : surroundingAtoms) {
                            if (atom.id == atomId) {
                                return &atom;
                            }
                        }
                    }
                    if (atomIndex >= 0) {
                        if (bond.bondType == atoms::domain::BondType::SURROUNDING) {
                            if (atomIndex < static_cast<int>(surroundingAtoms.size())) {
                                return &surroundingAtoms[atomIndex];
                            }
                            if (atomIndex < static_cast<int>(createdAtoms.size())) {
                                return &createdAtoms[atomIndex];
                            }
                        } else {
                            if (atomIndex < static_cast<int>(createdAtoms.size())) {
                                return &createdAtoms[atomIndex];
                            }
                            if (atomIndex < static_cast<int>(surroundingAtoms.size())) {
                                return &surroundingAtoms[atomIndex];
                            }
                        }
                    } else {
                        const int surroundingIndex = -atomIndex - 1;
                        if (surroundingIndex >= 0 &&
                            surroundingIndex < static_cast<int>(surroundingAtoms.size())) {
                            return &surroundingAtoms[surroundingIndex];
                        }
                    }
                    return nullptr;
                };

                auto buildAtomDisplayLabel = [&](const atoms::domain::AtomInfo* atom) {
                    if (!atom) {
                        return std::string("?(#?)");
                    }
                    auto originalIt = originalSerialByAtomId.find(atom->id);
                    if (originalIt != originalSerialByAtomId.end()) {
                        return atom->symbol + "#" + std::to_string(originalIt->second);
                    }
                    if (atom->originalAtomId != 0) {
                        auto itSerial = originalSerialByAtomId.find(atom->originalAtomId);
                        if (itSerial != originalSerialByAtomId.end()) {
                            return atom->symbol + "(#" + std::to_string(itSerial->second) + ")";
                        }
                    }
                    if (atom->atomType == atoms::domain::AtomType::ORIGINAL) {
                        return atom->symbol + "#?";
                    }
                    return atom->symbol + "(#?)";
                };

                std::map<std::string, std::vector<const atoms::domain::BondInfo*>> bondsByType;
                std::vector<uint32_t> structureBondIds;
                for (const auto& bond : createdBonds) {
                    if (bond.structureId != entry.id) {
                        continue;
                    }
                    const std::string bondType = bond.bondGroupKey.empty() ? "Bond" : bond.bondGroupKey;
                    bondsByType[bondType].push_back(&bond);
                    if (bond.id != 0) {
                        structureBondIds.push_back(bond.id);
                    }
                }
                for (const auto& bond : surroundingBonds) {
                    if (bond.structureId != entry.id) {
                        continue;
                    }
                    const std::string bondType = bond.bondGroupKey.empty() ? "Bond" : bond.bondGroupKey;
                    bondsByType[bondType].push_back(&bond);
                    if (bond.id != 0) {
                        structureBondIds.push_back(bond.id);
                    }
                }
                if (!structureBondIds.empty()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGuiTreeNodeFlags bondFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                                   ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                   ImGuiTreeNodeFlags_DefaultOpen |
                                                   ImGuiTreeNodeFlags_SpanFullWidth;
                    const std::string bondsRootId = "##Bonds" + std::to_string(entry.id);
                    const bool bondsOpen = ImGui::TreeNodeEx(
                        bondsRootId.c_str(), bondFlags, ICON_FA6_LINK"  Bonds");

                    const bool bondsStructureEnabled = atomsTemplate.IsBondsVisible(entry.id);
                    bool anyBondVisible = false;
                    bool allBondVisible = true;
                    for (uint32_t bondId : structureBondIds) {
                        const bool visible = bondsStructureEnabled && atomsTemplate.IsBondVisibleById(bondId);
                        anyBondVisible = anyBondVisible || visible;
                        allBondVisible = allBondVisible && visible;
                    }

                    ImGui::TableSetColumnIndex(1);
                    if (allBondVisible) {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            atomsTemplate.SetBondVisibilityForIds(structureBondIds, false);
                        }
                    } else if (anyBondVisible) {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.65f), ICON_FA6_EYE);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            atomsTemplate.SetBondsVisible(entry.id, true);
                            atomsTemplate.SetBondVisibilityForIds(structureBondIds, true);
                        }
                    } else {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            atomsTemplate.SetBondsVisible(entry.id, true);
                            atomsTemplate.SetBondVisibilityForIds(structureBondIds, true);
                        }
                    }

                    ImGui::TableSetColumnIndex(2);
                    bool anyBondLabelVisible = false;
                    bool allBondLabelVisible = !structureBondIds.empty();
                    for (uint32_t bondId : structureBondIds) {
                        const bool labelVisible = atomsTemplate.IsBondLabelVisibleById(bondId);
                        anyBondLabelVisible = anyBondLabelVisible || labelVisible;
                        allBondLabelVisible = allBondLabelVisible && labelVisible;
                    }
                    renderLabelState(allBondLabelVisible, anyBondLabelVisible, [&](bool visible) {
                        atomsTemplate.SetBondLabelVisibilityForIds(structureBondIds, visible);
                    });

                    ImGui::TableSetColumnIndex(3);
                    TextCenteredSizeT(structureBondIds.size());

                    if (bondsOpen) {
                        for (const auto& [bondType, bonds] : bondsByType) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGuiTreeNodeFlags bondTypeFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                                               ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                               ImGuiTreeNodeFlags_SpanFullWidth;
                            const std::string bondTypeNodeId =
                                "##BondType_" + std::to_string(entry.id) + "_" + bondType;
                            const bool bondTypeOpen = ImGui::TreeNodeEx(
                                bondTypeNodeId.c_str(), bondTypeFlags, ICON_FA6_LINK"  %s", bondType.c_str());

                            std::vector<uint32_t> bondIdsByType;
                            bondIdsByType.reserve(bonds.size());
                            bool anyTypeVisible = false;
                            bool allTypeVisible = !bonds.empty();
                            for (const auto* bond : bonds) {
                                if (!bond || bond->id == 0) {
                                    continue;
                                }
                                bondIdsByType.push_back(bond->id);
                                const bool visible = atomsTemplate.IsBondsVisible(entry.id) &&
                                                     atomsTemplate.IsBondVisibleById(bond->id);
                                anyTypeVisible = anyTypeVisible || visible;
                                allTypeVisible = allTypeVisible && visible;
                            }

                            ImGui::TableSetColumnIndex(1);
                            if (allTypeVisible) {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    atomsTemplate.SetBondVisibilityForIds(bondIdsByType, false);
                                }
                            } else if (anyTypeVisible) {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.65f), ICON_FA6_EYE);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    atomsTemplate.SetBondsVisible(entry.id, true);
                                    atomsTemplate.SetBondVisibilityForIds(bondIdsByType, true);
                                }
                            } else {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    atomsTemplate.SetBondsVisible(entry.id, true);
                                    atomsTemplate.SetBondVisibilityForIds(bondIdsByType, true);
                                }
                            }

                            ImGui::TableSetColumnIndex(2);
                            bool anyTypeLabelVisible = false;
                            bool allTypeLabelVisible = !bondIdsByType.empty();
                            for (uint32_t bondId : bondIdsByType) {
                                const bool labelVisible = atomsTemplate.IsBondLabelVisibleById(bondId);
                                anyTypeLabelVisible = anyTypeLabelVisible || labelVisible;
                                allTypeLabelVisible = allTypeLabelVisible && labelVisible;
                            }
                            renderLabelState(allTypeLabelVisible, anyTypeLabelVisible, [&](bool visible) {
                                atomsTemplate.SetBondLabelVisibilityForIds(bondIdsByType, visible);
                            });

                            ImGui::TableSetColumnIndex(3);
                            TextCenteredSizeT(bonds.size());

                            if (bondTypeOpen) {
                                for (const auto* bond : bonds) {
                                    if (!bond) {
                                        continue;
                                    }
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGuiTreeNodeFlags bondLeafFlags = ImGuiTreeNodeFlags_Leaf |
                                                                       ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                                                       ImGuiTreeNodeFlags_Bullet |
                                                                       ImGuiTreeNodeFlags_SpanFullWidth;
                                    const atoms::domain::AtomInfo* atom1 =
                                        resolveBondAtomByIndex(*bond, bond->atom1Index, bond->atom1Id);
                                    const atoms::domain::AtomInfo* atom2 =
                                        resolveBondAtomByIndex(*bond, bond->atom2Index, bond->atom2Id);

                                    std::string atomLabel1 = buildAtomDisplayLabel(atom1);
                                    std::string atomLabel2 = buildAtomDisplayLabel(atom2);

                                    auto endpointRank = [&](const atoms::domain::AtomInfo* atom) {
                                        if (!atom) {
                                            return 2;
                                        }
                                        auto itOriginal = originalSerialByAtomId.find(atom->id);
                                        if (itOriginal != originalSerialByAtomId.end()) {
                                            return 0;
                                        }
                                        return 1;
                                    };

                                    auto endpointSerial = [&](const atoms::domain::AtomInfo* atom) {
                                        if (!atom) {
                                            return static_cast<size_t>(-1);
                                        }
                                        auto itOriginal = originalSerialByAtomId.find(atom->id);
                                        if (itOriginal != originalSerialByAtomId.end()) {
                                            return itOriginal->second;
                                        }
                                        if (atom->originalAtomId != 0) {
                                            auto itOrigin = originalSerialByAtomId.find(atom->originalAtomId);
                                            if (itOrigin != originalSerialByAtomId.end()) {
                                                return itOrigin->second;
                                            }
                                        }
                                        return static_cast<size_t>(-1);
                                    };

                                    const int rank1 = endpointRank(atom1);
                                    const int rank2 = endpointRank(atom2);
                                    const size_t serial1 = endpointSerial(atom1);
                                    const size_t serial2 = endpointSerial(atom2);
                                    if (rank1 > rank2 ||
                                        (rank1 == rank2 && serial1 > serial2) ||
                                        (rank1 == rank2 && serial1 == serial2 && atomLabel1 > atomLabel2)) {
                                        std::string temp = atomLabel1;
                                        atomLabel1 = atomLabel2;
                                        atomLabel2 = temp;
                                    }

                                    const std::string bondLabel =
                                        atomLabel1 + "-" + atomLabel2 + "/#" + std::to_string(bond->id);
                                    const std::string bondNodeId = "##BondInstance_" +
                                                                   std::to_string(entry.id) + "_" +
                                                                   std::to_string(bond->id);
                                    ImGui::TreeNodeEx(bondNodeId.c_str(), bondLeafFlags, ICON_FA6_LINK"  %s",
                                                      bondLabel.c_str());

                                    ImGui::TableSetColumnIndex(1);
                                    const bool bondVisible = atomsTemplate.IsBondsVisible(entry.id) &&
                                                             atomsTemplate.IsBondVisibleById(bond->id);
                                    if (bondVisible) {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f),
                                                            ICON_FA6_EYE);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                            atomsTemplate.SetBondVisibleById(bond->id, false);
                                        }
                                    } else {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f),
                                                            ICON_FA6_EYE_SLASH);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                            atomsTemplate.SetBondsVisible(entry.id, true);
                                            atomsTemplate.SetBondVisibleById(bond->id, true);
                                        }
                                    }

                                    ImGui::TableSetColumnIndex(2);
                                    const bool bondLabelVisible = atomsTemplate.IsBondLabelVisibleById(bond->id);
                                    if (bondLabelVisible) {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f),
                                                            ICON_FA6_EYE);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                            atomsTemplate.SetBondLabelVisibleById(bond->id, false);
                                        }
                                    } else {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f),
                                                            ICON_FA6_EYE_SLASH);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                            atomsTemplate.SetBondLabelVisibleById(bond->id, true);
                                        }
                                    }

                                    ImGui::TableSetColumnIndex(3);
                                    ImGui::TextDisabled("--");
                                }
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                }

                if (atomsTemplate.HasUnitCell(entry.id)) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGuiTreeNodeFlags cellFlags = ImGuiTreeNodeFlags_Leaf |
                                                   ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                                   ImGuiTreeNodeFlags_Bullet |
                                                   ImGuiTreeNodeFlags_SpanFullWidth;
                    ImGui::TreeNodeEx(("##UnitCell" + std::to_string(entry.id)).c_str(), cellFlags, ICON_FA6_CUBE"  Unit Cell");
                    
                    ImGui::TableSetColumnIndex(1);
                    bool cellVisible = atomsTemplate.IsUnitCellVisible(entry.id);
                    if (cellVisible) {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            atomsTemplate.SetUnitCellVisible(entry.id, false);
                        }
                    } else {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            atomsTemplate.SetUnitCellVisible(entry.id, true);
                        }
                    }
                    
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextDisabled("--");

                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextDisabled("--");
                }

                auto measurementEntries = atomsTemplate.GetMeasurementsForStructure(entry.id);
                if (!measurementEntries.empty()) {
                    auto measurementTypeLabel = [](AtomsTemplate::MeasurementType type) -> const char* {
                        switch (type) {
                        case AtomsTemplate::MeasurementType::Distance:
                            return "Distance";
                        case AtomsTemplate::MeasurementType::Angle:
                            return "Angle";
                        case AtomsTemplate::MeasurementType::Dihedral:
                            return "Dihedral";
                        case AtomsTemplate::MeasurementType::GeometricCenter:
                            return "Geometric Center";
                        case AtomsTemplate::MeasurementType::CenterOfMass:
                            return "Center of Mass";
                        }
                        return "Measurement";
                    };

                    const AtomsTemplate::MeasurementType measurementTypeOrder[] = {
                        AtomsTemplate::MeasurementType::Distance,
                        AtomsTemplate::MeasurementType::Angle,
                        AtomsTemplate::MeasurementType::Dihedral,
                        AtomsTemplate::MeasurementType::GeometricCenter,
                        AtomsTemplate::MeasurementType::CenterOfMass
                    };

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGuiTreeNodeFlags measurementFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                                          ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                          ImGuiTreeNodeFlags_DefaultOpen |
                                                          ImGuiTreeNodeFlags_SpanFullWidth;
                    const std::string measurementRootId = "##MeasurementRoot_" + std::to_string(entry.id);
                    const bool measurementOpen = ImGui::TreeNodeEx(
                        measurementRootId.c_str(), measurementFlags, ICON_FA6_RULER"  Measurement");

                    bool anyMeasurementVisible = false;
                    bool allMeasurementVisible = !measurementEntries.empty();
                    for (const auto& measurement : measurementEntries) {
                        anyMeasurementVisible = anyMeasurementVisible || measurement.visible;
                        allMeasurementVisible = allMeasurementVisible && measurement.visible;
                    }

                    ImGui::TableSetColumnIndex(1);
                    if (allMeasurementVisible) {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            for (const auto& measurement : measurementEntries) {
                                atomsTemplate.SetMeasurementVisible(measurement.id, false);
                            }
                        }
                    } else if (anyMeasurementVisible) {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.65f), ICON_FA6_EYE);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            for (const auto& measurement : measurementEntries) {
                                atomsTemplate.SetMeasurementVisible(measurement.id, true);
                            }
                        }
                    } else {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            for (const auto& measurement : measurementEntries) {
                                atomsTemplate.SetMeasurementVisible(measurement.id, true);
                            }
                        }
                    }

                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextDisabled("--");

                    ImGui::TableSetColumnIndex(3);
                    TextCenteredSizeT(measurementEntries.size());

                    ImGui::TableSetColumnIndex(4);
                    TextColoredCentered(ImVec4(0.7f, 0.0f, 0.0f, 1.0f), ICON_FA6_TRASH_CAN);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Remove all Measurement entries");
                    }
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                        s_PendingClearMeasurementsStructureId = entry.id;
                        s_ShowClearMeasurementsConfirmPopup = true;
                    }

                    if (measurementOpen) {
                        for (const auto measurementType : measurementTypeOrder) {
                            std::vector<const AtomsTemplate::MeasurementListItem*> typedMeasurements;
                            typedMeasurements.reserve(measurementEntries.size());
                            for (const auto& measurement : measurementEntries) {
                                if (measurement.type == measurementType) {
                                    typedMeasurements.push_back(&measurement);
                                }
                            }
                            if (typedMeasurements.empty()) {
                                continue;
                            }

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGuiTreeNodeFlags typeFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                                           ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                           ImGuiTreeNodeFlags_DefaultOpen |
                                                           ImGuiTreeNodeFlags_SpanFullWidth;
                            const std::string measurementTypeNodeId = "##MeasurementType_" +
                                                                      std::to_string(entry.id) + "_" +
                                                                      std::to_string(static_cast<int>(measurementType));
                            const bool typeOpen = ImGui::TreeNodeEx(
                                measurementTypeNodeId.c_str(),
                                typeFlags,
                                ICON_FA6_RULER"  %s",
                                measurementTypeLabel(measurementType));

                            bool anyTypeVisible = false;
                            bool allTypeVisible = true;
                            for (const auto* measurement : typedMeasurements) {
                                anyTypeVisible = anyTypeVisible || measurement->visible;
                                allTypeVisible = allTypeVisible && measurement->visible;
                            }

                            ImGui::TableSetColumnIndex(1);
                            if (allTypeVisible) {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    for (const auto* measurement : typedMeasurements) {
                                        atomsTemplate.SetMeasurementVisible(measurement->id, false);
                                    }
                                }
                            } else if (anyTypeVisible) {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.65f), ICON_FA6_EYE);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    for (const auto* measurement : typedMeasurements) {
                                        atomsTemplate.SetMeasurementVisible(measurement->id, true);
                                    }
                                }
                            } else {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    for (const auto* measurement : typedMeasurements) {
                                        atomsTemplate.SetMeasurementVisible(measurement->id, true);
                                    }
                                }
                            }

                            ImGui::TableSetColumnIndex(2);
                            ImGui::TextDisabled("--");

                            ImGui::TableSetColumnIndex(3);
                            TextCenteredInt(static_cast<int>(typedMeasurements.size()));

                            ImGui::TableSetColumnIndex(4);
                            ImGui::TextDisabled("--");

                            if (!typeOpen) {
                                continue;
                            }

                            for (const auto* measurement : typedMeasurements) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGuiTreeNodeFlags measurementItemFlags = ImGuiTreeNodeFlags_Leaf |
                                                                          ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                                                          ImGuiTreeNodeFlags_Bullet |
                                                                          ImGuiTreeNodeFlags_SpanFullWidth;
                                const std::string measurementNodeId =
                                    "##MeasurementItem_" + std::to_string(entry.id) + "_" +
                                    std::to_string(measurement->id);
                                ImGui::TreeNodeEx(
                                    measurementNodeId.c_str(),
                                    measurementItemFlags,
                                    ICON_FA6_RULER"  %s",
                                    measurement->displayName.c_str());

                                ImGui::TableSetColumnIndex(1);
                                if (measurement->visible) {
                                    TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                        atomsTemplate.SetMeasurementVisible(measurement->id, false);
                                    }
                                } else {
                                    TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                        atomsTemplate.SetMeasurementVisible(measurement->id, true);
                                    }
                                }

                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextDisabled("--");

                                ImGui::TableSetColumnIndex(3);
                                ImGui::TextDisabled("--");

                                ImGui::TableSetColumnIndex(4);
                                TextColoredCentered(ImVec4(0.7f, 0.0f, 0.0f, 1.0f), ICON_FA6_TRASH_CAN);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    atomsTemplate.RemoveMeasurement(measurement->id);
                                }
                            }
                            ImGui::TreePop();
                        }
                        ImGui::TreePop();
                    }
                }

                auto* chargeDensityUI = atomsTemplate.chargeDensityUI();
                const bool hasChargeDensityForEntry =
                    atomsTemplate.HasChargeDensity() &&
                    atomsTemplate.GetChargeDensityStructureId() == entry.id;
                const auto simpleGridEntries = chargeDensityUI
                    ? chargeDensityUI->getSimpleGridEntries()
                    : std::vector<atoms::ui::ChargeDensityUI::SimpleGridEntry>{};
                const auto sliceGridEntries = chargeDensityUI
                    ? chargeDensityUI->getSliceGridEntries()
                    : std::vector<atoms::ui::ChargeDensityUI::SliceGridEntry>{};

                MeshManager& meshManager = MeshManager::Instance();
                std::vector<const TreeNode*> gridNodes;
                const TreeNode* structureNode = meshManager.GetMeshTree()->GetTreeNodeById(entry.id);
                if (structureNode) {
                    const TreeNode* child = structureNode->GetLeftChild();
                    while (child) {
                        const Mesh* childMesh = meshManager.GetMeshById(child->GetId());
                        if (childMesh && !childMesh->IsXsfStructure()) {
                            gridNodes.push_back(child);
                        }
                        child = child->GetRightSibling();
                    }
                }

                const bool hasGridDataForEntry = hasChargeDensityForEntry || !gridNodes.empty();
                if (hasGridDataForEntry) {
                    bool anySimpleGridVisible = false;
                    bool allSimpleGridVisible = true;
                    if (hasChargeDensityForEntry) {
                        if (simpleGridEntries.empty()) {
                            anySimpleGridVisible = atomsTemplate.IsChargeDensityVisible();
                            allSimpleGridVisible = anySimpleGridVisible;
                        } else {
                            allSimpleGridVisible = true;
                            for (const auto& simpleEntry : simpleGridEntries) {
                                anySimpleGridVisible = anySimpleGridVisible || simpleEntry.visible;
                                allSimpleGridVisible = allSimpleGridVisible && simpleEntry.visible;
                            }
                        }
                    }

                    bool anySurfaceVisible = false;
                    bool allSurfaceVisible = !gridNodes.empty();
                    bool anyVolumeVisible = false;
                    bool allVolumeVisible = !gridNodes.empty();
                    for (const TreeNode* gridNode : gridNodes) {
                        const bool surfaceVisible = atomsTemplate.IsChargeDensityAdvancedGridVisible(
                            gridNode->GetId(), false);
                        anySurfaceVisible = anySurfaceVisible || surfaceVisible;
                        allSurfaceVisible = allSurfaceVisible && surfaceVisible;

                        const bool volumeVisible = atomsTemplate.IsChargeDensityAdvancedGridVisible(
                            gridNode->GetId(), true);
                        anyVolumeVisible = anyVolumeVisible || volumeVisible;
                        allVolumeVisible = allVolumeVisible && volumeVisible;
                    }

                    bool anyPlaneVisible = false;
                    bool allPlaneVisible = true;
                    if (hasChargeDensityForEntry) {
                        if (sliceGridEntries.empty()) {
                            anyPlaneVisible = chargeDensityUI && chargeDensityUI->isSliceVisible();
                            allPlaneVisible = anyPlaneVisible;
                        } else {
                            allPlaneVisible = true;
                            for (const auto& sliceEntry : sliceGridEntries) {
                                anyPlaneVisible = anyPlaneVisible || sliceEntry.visible;
                                allPlaneVisible = allPlaneVisible && sliceEntry.visible;
                            }
                        }
                    }

                    bool anyGridDataVisible = false;
                    bool allGridDataVisible = true;
                    bool hasGridVisibilitySource = false;
                    if (hasChargeDensityForEntry) {
                        anyGridDataVisible = anyGridDataVisible || anySimpleGridVisible || anyPlaneVisible;
                        allGridDataVisible = allGridDataVisible && allSimpleGridVisible && allPlaneVisible;
                        hasGridVisibilitySource = true;
                    }
                    if (!gridNodes.empty()) {
                        anyGridDataVisible = anyGridDataVisible || anySurfaceVisible || anyVolumeVisible;
                        allGridDataVisible = allGridDataVisible && allSurfaceVisible && allVolumeVisible;
                        hasGridVisibilitySource = true;
                    }
                    if (!hasGridVisibilitySource) {
                        allGridDataVisible = false;
                    }

                    size_t gridDataCount = 0;
                    if (hasChargeDensityForEntry) {
                        gridDataCount += simpleGridEntries.empty() ? 1 : simpleGridEntries.size();
                        gridDataCount += sliceGridEntries.empty() ? 1 : sliceGridEntries.size();
                    }
                    if (!gridNodes.empty()) {
                        gridDataCount += gridNodes.size() * 2;
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGuiTreeNodeFlags gridDataFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                                       ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                       ImGuiTreeNodeFlags_DefaultOpen |
                                                       ImGuiTreeNodeFlags_SpanFullWidth;
                    const std::string gridDataRootId = "##GridDataRoot_" + std::to_string(entry.id);
                    const bool gridDataOpen = ImGui::TreeNodeEx(
                        gridDataRootId.c_str(), gridDataFlags, ICON_FA6_LAYER_GROUP"  Grid data");

                    ImGui::TableSetColumnIndex(1);
                    if (allGridDataVisible) {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            setChargeDensityContext(entry.id);
                            if (hasChargeDensityForEntry) {
                                if (chargeDensityUI && !simpleGridEntries.empty()) {
                                    chargeDensityUI->setAllSimpleGridVisible(false);
                                } else {
                                    atomsTemplate.SetChargeDensityVisible(false);
                                }
                                if (chargeDensityUI) {
                                    chargeDensityUI->setAllSliceGridVisible(false);
                                }
                            }
                            if (!gridNodes.empty()) {
                                atomsTemplate.SetAllChargeDensityAdvancedGridVisible(entry.id, false, false);
                                atomsTemplate.SetAllChargeDensityAdvancedGridVisible(entry.id, true, false);
                            }
                            hasChargeDensityVisibilityChange = true;
                        }
                    } else if (anyGridDataVisible) {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.65f), ICON_FA6_EYE);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            setChargeDensityContext(entry.id);
                            if (hasChargeDensityForEntry) {
                                if (chargeDensityUI && !simpleGridEntries.empty()) {
                                    chargeDensityUI->setAllSimpleGridVisible(true);
                                } else {
                                    atomsTemplate.SetChargeDensityVisible(true);
                                }
                                if (chargeDensityUI) {
                                    chargeDensityUI->setAllSliceGridVisible(true);
                                }
                            }
                            if (!gridNodes.empty()) {
                                atomsTemplate.SetAllChargeDensityAdvancedGridVisible(entry.id, false, true);
                                atomsTemplate.SetAllChargeDensityAdvancedGridVisible(entry.id, true, true);
                            }
                            hasChargeDensityVisibilityChange = true;
                        }
                    } else {
                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            setChargeDensityContext(entry.id);
                            if (hasChargeDensityForEntry) {
                                if (chargeDensityUI && !simpleGridEntries.empty()) {
                                    chargeDensityUI->setAllSimpleGridVisible(true);
                                } else {
                                    atomsTemplate.SetChargeDensityVisible(true);
                                }
                                if (chargeDensityUI) {
                                    chargeDensityUI->setAllSliceGridVisible(true);
                                }
                            }
                            if (!gridNodes.empty()) {
                                atomsTemplate.SetAllChargeDensityAdvancedGridVisible(entry.id, false, true);
                                atomsTemplate.SetAllChargeDensityAdvancedGridVisible(entry.id, true, true);
                            }
                            hasChargeDensityVisibilityChange = true;
                        }
                    }

                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextDisabled("--");

                    ImGui::TableSetColumnIndex(3);
                    TextCenteredSizeT(gridDataCount);

                    ImGui::TableSetColumnIndex(4);
                    ImGui::TextDisabled("--");

                    if (gridDataOpen) {
                        if (hasChargeDensityForEntry) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGuiTreeNodeFlags cdFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                                         ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                         ImGuiTreeNodeFlags_DefaultOpen |
                                                         ImGuiTreeNodeFlags_SpanFullWidth;
                            if (simpleGridEntries.empty()) {
                                cdFlags |= ImGuiTreeNodeFlags_Leaf |
                                           ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                           ImGuiTreeNodeFlags_Bullet;
                            }
                            bool cdTreeOpen = ImGui::TreeNodeEx(
                                ("##ChargeDensity" + std::to_string(entry.id)).c_str(),
                                cdFlags, ICON_FA6_CLOUD"  Isosurface");

                            ImGui::TableSetColumnIndex(1);
                            if (allSimpleGridVisible) {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    setChargeDensityContext(entry.id);
                                    if (chargeDensityUI && !simpleGridEntries.empty()) {
                                        chargeDensityUI->setAllSimpleGridVisible(false);
                                    } else {
                                        atomsTemplate.SetChargeDensityVisible(false);
                                    }
                                    hasChargeDensityVisibilityChange = true;
                                }
                            } else if (anySimpleGridVisible) {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.65f), ICON_FA6_EYE);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    setChargeDensityContext(entry.id);
                                    if (chargeDensityUI && !simpleGridEntries.empty()) {
                                        chargeDensityUI->setAllSimpleGridVisible(true);
                                    } else {
                                        atomsTemplate.SetChargeDensityVisible(true);
                                    }
                                    hasChargeDensityVisibilityChange = true;
                                }
                            } else {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    setChargeDensityContext(entry.id);
                                    if (chargeDensityUI && !simpleGridEntries.empty()) {
                                        chargeDensityUI->setAllSimpleGridVisible(true);
                                    } else {
                                        atomsTemplate.SetChargeDensityVisible(true);
                                    }
                                    hasChargeDensityVisibilityChange = true;
                                }
                            }

                            ImGui::TableSetColumnIndex(2);
                            ImGui::TextDisabled("--");

                            ImGui::TableSetColumnIndex(3);
                            if (simpleGridEntries.empty()) {
                                ImGui::TextDisabled("--");
                            } else {
                                TextCenteredInt(static_cast<int>(simpleGridEntries.size()));
                            }

                            ImGui::TableSetColumnIndex(4);
                            ImGui::TextDisabled("--");

                            if (cdTreeOpen && !simpleGridEntries.empty()) {
                                for (const auto& simpleEntry : simpleGridEntries) {
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGuiTreeNodeFlags simpleFlags = ImGuiTreeNodeFlags_Leaf |
                                                                     ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                                                     ImGuiTreeNodeFlags_Bullet |
                                                                     ImGuiTreeNodeFlags_SpanFullWidth;
                                    ImGui::TreeNodeEx(
                                        ("##SimpleGrid_" + std::to_string(entry.id) + "_" + simpleEntry.name).c_str(),
                                        simpleFlags, ICON_FA6_LAYER_GROUP"  %s", simpleEntry.name.c_str());
                                    if (ImGui::IsItemHovered() &&
                                        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
                                        chargeDensityUI) {
                                        openChargeDensityViewer(entry.id, AtomsTemplate::DataMenuRequest::Isosurface);
                                        chargeDensityUI->setSimpleGridVisible(simpleEntry.name, true);
                                        chargeDensityUI->selectSimpleGridByName(simpleEntry.name);
                                    }

                                    ImGui::TableSetColumnIndex(1);
                                    if (simpleEntry.visible) {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && chargeDensityUI) {
                                            setChargeDensityContext(entry.id);
                                            chargeDensityUI->setSimpleGridVisible(simpleEntry.name, false);
                                            hasChargeDensityVisibilityChange = true;
                                        }
                                    } else {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && chargeDensityUI) {
                                            setChargeDensityContext(entry.id);
                                            chargeDensityUI->setSimpleGridVisible(simpleEntry.name, true);
                                            hasChargeDensityVisibilityChange = true;
                                        }
                                    }

                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::TextDisabled("--");

                                    ImGui::TableSetColumnIndex(3);
                                    ImGui::TextDisabled("--");

                                    ImGui::TableSetColumnIndex(4);
                                    ImGui::TextDisabled("--");
                                }
                                ImGui::TreePop();
                            }
                        }

                        if (!gridNodes.empty()) {
                            auto renderAdvancedGridGroup = [&](const char* groupLabel,
                                                               const char* groupSuffix,
                                                               bool volumeMode,
                                                               bool anyGridVisible,
                                                               bool allGridVisible) {
                                const AtomsTemplate::DataMenuRequest request = volumeMode
                                    ? AtomsTemplate::DataMenuRequest::Volumetric
                                    : AtomsTemplate::DataMenuRequest::Surface;
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGuiTreeNodeFlags groupFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                                                ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                                ImGuiTreeNodeFlags_DefaultOpen |
                                                                ImGuiTreeNodeFlags_SpanFullWidth;
                                std::string groupId = "##GridRoot_" + std::to_string(entry.id) + "_" + groupSuffix;
                                bool groupOpen = ImGui::TreeNodeEx(groupId.c_str(), groupFlags,
                                                                   ICON_FA6_LAYER_GROUP"  %s", groupLabel);

                                ImGui::TableSetColumnIndex(1);
                                if (allGridVisible) {
                                    TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                        setChargeDensityContext(entry.id);
                                        atomsTemplate.SetAllChargeDensityAdvancedGridVisible(
                                            entry.id, volumeMode, false);
                                        hasChargeDensityVisibilityChange = true;
                                    }
                                } else if (anyGridVisible) {
                                    TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.65f), ICON_FA6_EYE);
                                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                        setChargeDensityContext(entry.id);
                                        atomsTemplate.SetAllChargeDensityAdvancedGridVisible(
                                            entry.id, volumeMode, true);
                                        hasChargeDensityVisibilityChange = true;
                                    }
                                } else {
                                    TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                        setChargeDensityContext(entry.id);
                                        atomsTemplate.SetAllChargeDensityAdvancedGridVisible(
                                            entry.id, volumeMode, true);
                                        hasChargeDensityVisibilityChange = true;
                                    }
                                }

                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextDisabled("--");

                                ImGui::TableSetColumnIndex(3);
                                TextCenteredInt(static_cast<int>(gridNodes.size()));

                                ImGui::TableSetColumnIndex(4);
                                ImGui::TextDisabled("--");

                                if (!groupOpen) {
                                    return;
                                }

                                for (const TreeNode* gridNode : gridNodes) {
                                    const Mesh* gridMesh = meshManager.GetMeshById(gridNode->GetId());
                                    const char* gridName = gridMesh ? gridMesh->GetName() : "Grid";

                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGuiTreeNodeFlags gridFlags = ImGuiTreeNodeFlags_Leaf |
                                                                   ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                                                   ImGuiTreeNodeFlags_Bullet |
                                                                   ImGuiTreeNodeFlags_SpanFullWidth;
                                    std::string nodeId = "##GridNode_" + std::to_string(gridNode->GetId()) +
                                                         "_" + groupSuffix;
                                    ImGui::TreeNodeEx(nodeId.c_str(), gridFlags, ICON_FA6_LAYER_GROUP"  %s", gridName);
                                    if (ImGui::IsItemHovered() &&
                                        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                        s_SelectedMeshId = gridNode->GetId();
                                        openChargeDensityViewer(entry.id, request);
                                        atomsTemplate.SetChargeDensityAdvancedGridVisible(
                                            gridNode->GetId(), volumeMode, true);
                                        hasChargeDensityVisibilityChange = true;
                                    }

                                    ImGui::TableSetColumnIndex(1);
                                    const bool visible = atomsTemplate.IsChargeDensityAdvancedGridVisible(
                                        gridNode->GetId(), volumeMode);
                                    if (visible) {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                            setChargeDensityContext(entry.id);
                                            atomsTemplate.SetChargeDensityAdvancedGridVisible(
                                                gridNode->GetId(), volumeMode, false);
                                            hasChargeDensityVisibilityChange = true;
                                        }
                                    } else {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f),
                                                            ICON_FA6_EYE_SLASH);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                            setChargeDensityContext(entry.id);
                                            atomsTemplate.SetChargeDensityAdvancedGridVisible(
                                                gridNode->GetId(), volumeMode, true);
                                            hasChargeDensityVisibilityChange = true;
                                        }
                                    }

                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::TextDisabled("--");

                                    ImGui::TableSetColumnIndex(3);
                                    ImGui::TextDisabled("--");

                                    ImGui::TableSetColumnIndex(4);
                                    ImGui::TextDisabled("--");
                                }
                                ImGui::TreePop();
                            };

                            renderAdvancedGridGroup("Surface", "Surface", false, anySurfaceVisible, allSurfaceVisible);
                            renderAdvancedGridGroup("Volume", "Volume", true, anyVolumeVisible, allVolumeVisible);
                        }

                        if (hasChargeDensityForEntry) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGuiTreeNodeFlags planeFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                                            ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                            ImGuiTreeNodeFlags_DefaultOpen |
                                                            ImGuiTreeNodeFlags_SpanFullWidth;
                            if (sliceGridEntries.empty()) {
                                planeFlags |= ImGuiTreeNodeFlags_Leaf |
                                              ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                              ImGuiTreeNodeFlags_Bullet;
                            }
                            const std::string planeGroupId = "##PlaneGroup_" + std::to_string(entry.id);
                            const bool planeOpen = ImGui::TreeNodeEx(
                                planeGroupId.c_str(), planeFlags, ICON_FA6_LAYER_GROUP"  Plane");
                            if (ImGui::IsItemHovered() &&
                                ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
                                chargeDensityUI) {
                                openSliceViewer(entry.id);
                            }

                            ImGui::TableSetColumnIndex(1);
                            ImGui::BeginDisabled(chargeDensityUI == nullptr);
                            if (allPlaneVisible) {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && chargeDensityUI) {
                                    setChargeDensityContext(entry.id);
                                    chargeDensityUI->setAllSliceGridVisible(false);
                                    hasChargeDensityVisibilityChange = true;
                                }
                            } else if (anyPlaneVisible) {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.65f), ICON_FA6_EYE);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && chargeDensityUI) {
                                    setChargeDensityContext(entry.id);
                                    chargeDensityUI->setAllSliceGridVisible(true);
                                    hasChargeDensityVisibilityChange = true;
                                }
                            } else {
                                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && chargeDensityUI) {
                                    setChargeDensityContext(entry.id);
                                    chargeDensityUI->setAllSliceGridVisible(true);
                                    hasChargeDensityVisibilityChange = true;
                                }
                            }
                            ImGui::EndDisabled();

                            ImGui::TableSetColumnIndex(2);
                            ImGui::TextDisabled("--");

                            ImGui::TableSetColumnIndex(3);
                            if (sliceGridEntries.empty()) {
                                ImGui::TextDisabled("--");
                            } else {
                                TextCenteredInt(static_cast<int>(sliceGridEntries.size()));
                            }

                            ImGui::TableSetColumnIndex(4);
                            ImGui::TextDisabled("--");

                            if (planeOpen && !sliceGridEntries.empty()) {
                                for (const auto& sliceEntry : sliceGridEntries) {
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGuiTreeNodeFlags sliceGridFlags = ImGuiTreeNodeFlags_Leaf |
                                                                        ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                                                        ImGuiTreeNodeFlags_Bullet |
                                                                        ImGuiTreeNodeFlags_SpanFullWidth;
                                    const std::string sliceNodeId =
                                        "##PlaneGrid_" + std::to_string(entry.id) + "_" + sliceEntry.name;
                                    ImGui::TreeNodeEx(sliceNodeId.c_str(), sliceGridFlags,
                                                      ICON_FA6_LAYER_GROUP"  %s", sliceEntry.name.c_str());
                                    if (ImGui::IsItemHovered() &&
                                        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
                                        chargeDensityUI) {
                                        openSliceViewer(entry.id);
                                        setChargeDensityContext(entry.id);
                                        chargeDensityUI->selectSliceGridByName(sliceEntry.name);
                                        hasChargeDensityVisibilityChange = true;
                                    }

                                    ImGui::TableSetColumnIndex(1);
                                    if (sliceEntry.visible) {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && chargeDensityUI) {
                                            setChargeDensityContext(entry.id);
                                            chargeDensityUI->setSliceGridVisible(sliceEntry.name, false);
                                            hasChargeDensityVisibilityChange = true;
                                        }
                                    } else {
                                        TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f),
                                                            ICON_FA6_EYE_SLASH);
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && chargeDensityUI) {
                                            setChargeDensityContext(entry.id);
                                            chargeDensityUI->setSliceGridVisible(sliceEntry.name, true);
                                            hasChargeDensityVisibilityChange = true;
                                        }
                                    }

                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::TextDisabled("--");

                                    ImGui::TableSetColumnIndex(3);
                                    ImGui::TextDisabled("--");

                                    ImGui::TableSetColumnIndex(4);
                                    ImGui::TextDisabled("--");
                                }
                                ImGui::TreePop();
                            }
                        }

                        ImGui::TreePop();
                    }
                }
                ImGui::EndDisabled();
                ImGui::TreePop();
            }
        }

        ImGui::EndTable();
        if (hasChargeDensityVisibilityChange) {
            atomsTemplate.SyncChargeDensityViewTypeState();
        }
    }
}

void ModelTree::renderMeshTable(ImGuiTableFlags tableFlags) {
    constexpr int32_t TABLE_COLUMN_COUNT = 4;
    static const char* tableHeaders[TABLE_COLUMN_COUNT] = { "Name", "Id", "Show", "Remove" };
    static ImGuiTableColumnFlags tableColumnFlags[TABLE_COLUMN_COUNT] = {
        ImGuiTableColumnFlags_WidthStretch,
        ImGuiTableColumnFlags_WidthFixed,
        ImGuiTableColumnFlags_WidthFixed,
        ImGuiTableColumnFlags_WidthFixed
    };
    static float tableColumnSizes[TABLE_COLUMN_COUNT] = { 27.0f, 3.0f, 4.0f, 4.0f };

    bool isOpen = ImGui::CollapsingHeader("Mesh Tree", ImGuiTreeNodeFlags_DefaultOpen);
    if (isOpen && ImGui::BeginTable("MeshTree_Table", TABLE_COLUMN_COUNT, tableFlags)) {
        for (int iCol = 0; iCol < TABLE_COLUMN_COUNT; iCol++) {
            ImGui::TableSetupColumn(tableHeaders[iCol], tableColumnFlags[iCol], App::TextBaseWidth() * tableColumnSizes[iCol]);
        }

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        for (int iCol = 0; iCol < TABLE_COLUMN_COUNT; iCol++) {
            ImGui::TableSetColumnIndex(iCol);
            ImGui::PushID(iCol);
            ImGui::TableHeader(tableHeaders[iCol]);
            if (iCol == 2) {
                App::AddTooltip("Show/Hide Mesh");
            }
            else if (iCol == 3) {
                App::AddTooltip("Delete Mesh");
            }
            ImGui::PopID();
        }

        MeshManager& meshManager = MeshManager::Instance();
        TreeNode* rootNode = meshManager.GetMeshTree()->GetRootMutable();
        renderMeshTree(rootNode);

        ImGui::EndTable();
    }

    // Delete mesh 처리 (일반 VTK 메시)
    MeshDetail& meshDetail = MeshDetail::Instance();
    if (s_DeleteMeshId != -1) {
        if (s_DeleteMeshId != 0) {
            MeshManager::Instance().DeleteMesh(s_DeleteMeshId);
        } else {
            SPDLOG_ERROR("Cannot delete the root node.");
        }
        s_DeleteMeshId = -1;
    }
    else {
        // 더블클릭으로 선택 해제
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsWindowFocused()) {
            s_SelectedMeshId = -1;

            meshDetail.SetHasEdgeMeshColorChanged(false);
            meshDetail.SetHasEdgeMeshOpacityChanged(false);
            meshDetail.SetHasFaceMeshColorChanged(false);
            meshDetail.SetHasFaceMeshOpacityChanged(false);
            meshDetail.SetHasFaceMeshEdgeColorChanged(false);
            meshDetail.SetHasVolumeMeshColorChanged(false);
            meshDetail.SetHasVolumeMeshOpacityChanged(false);
            meshDetail.SetHasVolumeMeshEdgeColorChanged(false);
            meshDetail.SetHasVolumeRenderOpacityMinChanged(false);
            meshDetail.SetHasVolumeRenderOpacityMaxChanged(false);
            meshDetail.SetHasVolumeWindowLevelChanged(false);
            meshDetail.SetHasVolumeColorCurveChanged(false);
            meshDetail.SetHasVolumeSampleDistanceChanged(false);
        }
    }
}

void ModelTree::renderMeshTree(TreeNode* node) {
    if (node == nullptr) {
        return;
    }

    MeshManager& meshManager = MeshManager::Instance();
    Mesh* mesh = meshManager.GetMeshByIdMutable(node->GetId());
    if (mesh == nullptr) {
        return;
    }
    if (node->GetParent() && node->GetParent()->GetId() != 0) {
        Mesh* parentMesh = meshManager.GetMeshByIdMutable(node->GetParent()->GetId());
        if (parentMesh && parentMesh->IsXsfStructure() && !mesh->IsXsfStructure()) {
            if (node->GetRightSibling()) {
                renderMeshTree(node->GetRightSiblingMutable());
            }
            return;
        }
    }

    ImGuiTreeNodeFlags flagOpen = 
        ImGuiTreeNodeFlags_OpenOnArrow | 
        ImGuiTreeNodeFlags_OpenOnDoubleClick | 
        ImGuiTreeNodeFlags_DefaultOpen | 
        ImGuiTreeNodeFlags_SpanFullWidth;
    ImGuiTreeNodeFlags flagNoOpen = 
        ImGuiTreeNodeFlags_Leaf | 
        ImGuiTreeNodeFlags_NoTreePushOnOpen | 
        ImGuiTreeNodeFlags_Bullet | 
        ImGuiTreeNodeFlags_SpanFullWidth;

    bool isTreeOpen = false;
    bool isXsfStructure = mesh->IsXsfStructure();

    ImGui::TableNextRow();

    // Column 0: Mesh name
    ImGui::TableSetColumnIndex(0);
    std::string meshNodeId = std::to_string(node->GetId());
    ImGuiTreeNodeFlags selectFlag = s_SelectedMeshId == node->GetId() ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;
    
    if (node->GetLeftChild()) {
        isTreeOpen = ImGui::TreeNodeEx(meshNodeId.c_str(), flagOpen | selectFlag, ICON_FA6_CUBES"  %s", mesh->GetName());
    } else {
        const char* icon = isXsfStructure ? ICON_FA6_ATOM : ICON_FA6_CUBE;
        ImGui::TreeNodeEx(meshNodeId.c_str(), flagNoOpen | selectFlag, "%s  %s", icon, mesh->GetName());
    }

    // Click event
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        if (!mesh) {
            return;
        }

        MeshDetail& meshDetail = MeshDetail::Instance();

        if (s_SelectedMeshId != node->GetId()) {
            s_SelectedMeshId = node->GetId();

            if (!isXsfStructure) {
                meshDetail.SetUiEdgeMeshColor(mesh->GetEdgeMeshColor());
                meshDetail.SetUiEdgeMeshOpacity(mesh->GetEdgeMeshOpacity());
                meshDetail.SetUiEdgeMeshVisibility(mesh->GetEdgeMeshVisibility());

                meshDetail.SetUiFaceMeshColor(mesh->GetFaceMeshColor());
                meshDetail.SetUiFaceMeshOpacity(mesh->GetFaceMeshOpacity());
                meshDetail.SetUiFaceMeshVisibility(mesh->GetFaceMeshVisibility());

                meshDetail.SetUiVolumeMeshColor(mesh->GetVolumeMeshColor());
                meshDetail.SetUiVolumeMeshOpacity(mesh->GetVolumeMeshOpacity());
                meshDetail.SetUiVolumeMeshEdgeColor(mesh->GetVolumeMeshEdgeColor());
                meshDetail.SetUiVolumeMeshVisibility(mesh->GetVolumeMeshVisibility());

                // Volume rendering settings (다른 사람 코드에서 추가)
                meshDetail.SetUiVolumeWindowLevel(mesh->GetVolumeWindow(), mesh->GetVolumeLevel());
                meshDetail.SetUiVolumeRenderOpacityMin(mesh->GetVolumeRenderOpacityMin());
                meshDetail.SetUiVolumeRenderOpacity(mesh->GetVolumeRenderOpacity());
                meshDetail.SetUiVolumeRenderMode(
                    mesh->GetVolumeRenderMode() == Mesh::VolumeRenderMode::Volume ? 1 : 0);
                meshDetail.SetUiVolumeColorPreset(static_cast<int>(mesh->GetVolumeColorPreset()));
                meshDetail.SetUiVolumeColorCurve(mesh->GetVolumeColorMidpoint(), mesh->GetVolumeColorSharpness());
                meshDetail.SetUiVolumeSampleDistanceIndex(mesh->GetVolumeSampleDistanceIndex());
                meshDetail.SetUiVolumeQuality(static_cast<int>(mesh->GetVolumeQuality()));
            }
        }
        else {
            s_SelectedMeshId = -1;

            meshDetail.SetHasEdgeMeshColorChanged(false);
            meshDetail.SetHasEdgeMeshOpacityChanged(false);
            meshDetail.SetHasFaceMeshColorChanged(false);
            meshDetail.SetHasFaceMeshOpacityChanged(false);
            meshDetail.SetHasFaceMeshEdgeColorChanged(false);
            meshDetail.SetHasVolumeMeshColorChanged(false);
            meshDetail.SetHasVolumeMeshOpacityChanged(false);
            meshDetail.SetHasVolumeMeshEdgeColorChanged(false);
            meshDetail.SetHasVolumeRenderOpacityMinChanged(false);
            meshDetail.SetHasVolumeRenderOpacityMaxChanged(false);
            meshDetail.SetHasVolumeWindowLevelChanged(false);
            meshDetail.SetHasVolumeColorCurveChanged(false);
            meshDetail.SetHasVolumeSampleDistanceChanged(false);
        }
    }

    // Column 1: Mesh id
    ImGui::TableSetColumnIndex(1);
    TextCenteredInt(node->GetId());

    // Column 2: Show/Hide mesh icon
    ImGui::TableSetColumnIndex(2);
    if (node->GetId() != 0) {
        ImVec4 curColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        
        if (isXsfStructure) {
            AtomsTemplate& atomsTemplate = AtomsTemplate::Instance();
            bool isVisible = atomsTemplate.IsStructureVisible(node->GetId());
            
            if (isVisible) {
                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    atomsTemplate.SetStructureVisible(node->GetId(), false);
                    meshManager.HideMesh(node->GetId());
                }
            } else {
                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    atomsTemplate.SetStructureVisible(node->GetId(), true);
                    MeshDetail::Instance().SetUiVolumeMeshVisibility(true);
                    meshManager.ShowMesh(node->GetId());
                }
            }
        } else {
            if (node->GetIconState() == IconState::VISIBLE) {
                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ICON_FA6_EYE);
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    meshManager.HideMesh(node->GetId());
                }
            } else if (node->GetIconState() == IconState::PARTIAL) {
                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE);
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    meshManager.ShowMesh(node->GetId());
                }
            } else if (node->GetIconState() == IconState::HIDDEN) {
                TextColoredCentered(ImVec4(curColor.x, curColor.y, curColor.z, 0.4f), ICON_FA6_EYE_SLASH);
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    meshManager.ShowMesh(node->GetId());
                }
            }
        }
    }

    // Column 3: Delete mesh icon
    ImGui::TableSetColumnIndex(3);
    if (node->GetId() != 0) {
        TextColoredCentered(ImVec4(0.7f, 0.0f, 0.0f, 1.0f), ICON_FA6_TRASH_CAN);
        
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            s_PendingDeleteMeshId = node->GetId();
            s_ShowDeleteConfirmPopup = true;
        }
    }

    if (isTreeOpen) {
        renderMeshTree(node->GetLeftChildMutable());
        ImGui::TreePop();
    }

    if (node->GetRightSibling()) {
        renderMeshTree(node->GetRightSiblingMutable());
    }
}
