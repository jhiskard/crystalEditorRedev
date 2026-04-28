// atoms/ui/charge_density_ui.cpp
#include "charge_density_ui.h"
#include "../atoms_template.h"  // ???꾩뿭 AtomsTemplate ?ы븿
#include "../infrastructure/charge_density_renderer.h"
#include "../../config/log_config.h"
#include "../../mesh_manager.h"
#include "../../mesh_detail.h"
#include "../../lcrs_tree.h"
#include <vtkColorTransferFunction.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#define GLFW_INCLUDE_ES3
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <limits>
#include <optional>

namespace atoms {
namespace ui {

#ifdef __EMSCRIPTEN__
EM_JS(void, download_slice_png, (const uint8_t* data, int width, int height, const char* filename, int nameLength), {
    const heap = (typeof HEAPU8 !== 'undefined') ? HEAPU8 :
                 ((typeof Module !== 'undefined' && Module.HEAPU8) ? Module.HEAPU8 :
                 ((typeof VtkModule !== 'undefined' && VtkModule.HEAPU8) ? VtkModule.HEAPU8 : null));
    if (!heap) {
        console.error("HEAPU8 is not available for PNG download.");
        return;
    }
    const w = Number(width);
    const h = Number(height);
    if (!Number.isFinite(w) || !Number.isFinite(h) || w <= 0 || h <= 0) {
        console.error("Invalid slice size for PNG download.");
        return;
    }
    const start = (typeof data === 'bigint') ? Number(data) : data;
    const size = w * h * 4;
    if (!start || !Number.isFinite(start) || !Number.isFinite(size) || size <= 0) {
        console.error("Invalid slice data for PNG download.");
        return;
    }
    const src = heap.subarray(start, start + size);
    const pixels = new Uint8ClampedArray(src);
    const canvas = document.createElement('canvas');
    canvas.width = w;
    canvas.height = h;
    const ctx = canvas.getContext('2d');
    if (!ctx) {
        console.error("Canvas 2D context is unavailable.");
        return;
    }
    const imageData = ctx.createImageData(w, h);
    imageData.data.set(pixels);
    ctx.putImageData(imageData, 0, 0);
    const name = (() => {
        if (!filename) {
            return 'slice.png';
        }
        const ptr = (typeof filename === 'bigint') ? Number(filename) : filename;
        if (!ptr || !Number.isFinite(ptr)) {
            return 'slice.png';
        }
        const len = Number(nameLength);
        if (!Number.isFinite(len) || len <= 0) {
            return 'slice.png';
        }
        const end = Math.min(ptr + len, heap.length);
        if (end <= ptr) {
            return 'slice.png';
        }
        try {
            return new TextDecoder('utf-8').decode(heap.subarray(ptr, end)) || 'slice.png';
        } catch (e) {
            return 'slice.png';
        }
    })();
    canvas.toBlob((blob) => {
        const link = document.createElement('a');
        link.href = URL.createObjectURL(blob);
        link.download = name || 'slice.png';
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        setTimeout(() => URL.revokeObjectURL(link.href), 1000);
    }, 'image/png');
});
#endif

namespace {
constexpr const char* kMainSliceSettingsKey = "__main__";

std::string toLowerCopy(const std::string& input) {
    std::string output = input;
    std::transform(output.begin(), output.end(), output.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return output;
}

bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    return toLowerCopy(a) == toLowerCopy(b);
}

std::string formatIntegerWithCommas(int64_t value) {
    std::string sign;
    uint64_t absValue = 0;
    if (value < 0) {
        sign = "-";
        absValue = static_cast<uint64_t>(-(value + 1)) + 1;
    } else {
        absValue = static_cast<uint64_t>(value);
    }

    std::string digits = std::to_string(absValue);
    for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3) {
        digits.insert(static_cast<size_t>(i), ",");
    }
    return sign + digits;
}

struct WindowLevelRange {
    float minValue = 0.0f;
    float maxValue = 1.0f;
};

WindowLevelRange resolveWindowLevelRange(double dataMin, double dataMax, double window, double level) {
    if (!std::isfinite(dataMin) || !std::isfinite(dataMax)) {
        return {};
    }
    if (dataMax <= dataMin) {
        dataMax = dataMin + 1.0;
    }

    if (!std::isfinite(window) || window <= 0.0) {
        window = dataMax - dataMin;
    }
    if (window <= 0.0) {
        window = 1.0;
    }

    double minValue = level - window * 0.5;
    double maxValue = level + window * 0.5;
    if (minValue < dataMin) {
        minValue = dataMin;
    }
    if (maxValue > dataMax) {
        maxValue = dataMax;
    }
    if (minValue >= maxValue) {
        minValue = dataMin;
        maxValue = dataMax;
    }

    return {
        static_cast<float>(minValue),
        static_cast<float>(maxValue)
    };
}

bool nearlyEqual(double a, double b, double epsilon = 1e-6) {
    return std::abs(a - b) <= epsilon;
}
}  // 내부 헬퍼

ChargeDensityUI::ChargeDensityUI(::AtomsTemplate* atomsTemplate)  // ??:: 異붽?
    : m_atomsTemplate(atomsTemplate) {
    SPDLOG_DEBUG("ChargeDensityUI initialized");
}

ChargeDensityUI::~ChargeDensityUI() {
    clear();
}

bool ChargeDensityUI::loadFile(const std::string& filePath) {
    SPDLOG_INFO("Loading charge density file: {}", filePath);
    
    // ?뚮뜑?ш? ?놁쑝硫??앹꽦
    if (!m_renderer) {
        m_renderer = std::make_unique<infrastructure::ChargeDensityRenderer>(
            m_atomsTemplate->vtkRenderer()
        );
    }
    
    // ?? ??? ???
    m_renderer->clear();
    for (auto& [_, renderer] : m_auxRenderers) {
        if (renderer) {
            renderer->clear();
        }
    }
    m_auxRenderers.clear();
    m_simpleGridVisibility.clear();
    m_sliceGridVisibility.clear();
    m_simpleGridIsoValues.clear();
    m_simpleGridIsoColors.clear();
    m_sliceDisplaySettings.clear();
    
    // ?뚯씪 濡쒕뱶
    if (!m_renderer->loadFromFile(filePath)) {
        SPDLOG_ERROR("Failed to load charge density file");
        m_fileLoaded = false;
        return false;
    }
    
    // ?뚯씪紐?異붿텧
    size_t lastSlash = filePath.find_last_of("/\\");
    m_loadedFileName = (lastSlash != std::string::npos) 
                       ? filePath.substr(lastSlash + 1) 
                       : filePath;
    
    // ?뺣낫 ?낅뜲?댄듃
    m_gridShape = m_renderer->getGridShape();
    m_minValue = m_renderer->getMinValue();
    m_maxValue = m_renderer->getMaxValue();
    m_fileLoaded = true;
    m_loadedFromGrid = false;
    
    // 湲곕낯 ?깆튂硫?媛?怨꾩궛
    calculateDefaultIsoValue();
    
    // ?? ?? ???
    m_displayMin = m_minValue;
    m_displayMax = m_maxValue;
    m_sliceColorMidpoint = 0.5f;
    m_sliceColorSharpness = 0.0f;
    m_hasSyncedVolumeDisplaySettings = false;
    storeSliceDisplaySettingsForLoadedGrid();
    rebuildMultipleIsosurfaces();
    
    SPDLOG_INFO("Charge density loaded: {}, grid={}x{}x{}, range=[{:.4e}, {:.4e}]",
                m_loadedFileName, m_gridShape[0], m_gridShape[1], m_gridShape[2],
                m_minValue, m_maxValue);
    
    return true;
}

void ChargeDensityUI::clear() {
    if (m_renderer) {
        m_renderer->clear();
    }
    for (auto& [_, renderer] : m_auxRenderers) {
        if (renderer) {
            renderer->clear();
        }
    }
    m_auxRenderers.clear();
    m_simpleGridVisibility.clear();
    m_sliceGridVisibility.clear();
    m_simpleGridIsoValues.clear();
    m_simpleGridIsoColors.clear();
    m_sliceDisplaySettings.clear();
    
    m_fileLoaded = false;
    m_loadedFromGrid = false;
    m_loadedFileName.clear();
    m_gridShape = {0, 0, 0};
    m_minValue = m_maxValue = 0.0f;
    m_displayMin = 0.0f;
    m_displayMax = 1.0f;
    m_autoRange = true;
    m_sliceColorMidpoint = 0.5f;
    m_sliceColorSharpness = 0.0f;
    m_hasSyncedVolumeDisplaySettings = false;
    m_showIsosurface = false;
    m_showMultipleIsosurfaces = false;
    m_showSlice = false;
    m_sliceDownloadName.clear();
    clearSlicePreview();
    
    SPDLOG_DEBUG("ChargeDensityUI cleared");
}

bool ChargeDensityUI::hasData() const {
    return m_renderer && m_renderer->hasData();
}

std::vector<ChargeDensityUI::GridMeshEntry> ChargeDensityUI::collectGridMeshes() const {
    std::vector<GridMeshEntry> entries;
    if (!m_atomsTemplate) {
        return entries;
    }

    MeshManager& meshManager = MeshManager::Instance();
    auto collectFromStructure = [&](int32_t structureId) {
        if (structureId < 0) {
            return;
        }
        const TreeNode* structureNode = meshManager.GetMeshTree()->GetTreeNodeById(structureId);
        if (!structureNode) {
            return;
        }

        const TreeNode* child = structureNode->GetLeftChild();
        while (child) {
            const Mesh* mesh = meshManager.GetMeshById(child->GetId());
            if (mesh && !mesh->IsXsfStructure() && mesh->GetVolumeMeshActor()) {
                GridMeshEntry entry;
                entry.id = child->GetId();
                entry.name = mesh->GetName();
                entry.visible = mesh->GetVolumeMeshVisibility();
                entries.push_back(std::move(entry));
            }
            child = child->GetRightSibling();
        }
    };

    int32_t structureId = m_atomsTemplate->GetCurrentStructureId();
    collectFromStructure(structureId);

    if (!entries.empty()) {
        return entries;
    }

    std::vector<int32_t> structureIds;
    meshManager.GetMeshTree()->TraverseTree([&](const TreeNode* node, void*) {
        if (!node) {
            return;
        }
        int32_t id = node->GetId();
        if (id <= 0) {
            return;
        }
        const Mesh* mesh = meshManager.GetMeshById(id);
        if (mesh && mesh->IsXsfStructure()) {
            structureIds.push_back(id);
        }
    });

    for (int32_t id : structureIds) {
        if (id == structureId) {
            continue;
        }
        collectFromStructure(id);
        if (!entries.empty()) {
            break;
        }
    }

    return entries;
}

void ChargeDensityUI::render() {
    renderFileSection();

    const auto gridMeshes = collectGridMeshes();
    const bool hasGridMeshes = !gridMeshes.empty();

    if (m_fileLoaded || hasGridMeshes) {
        ImGui::Separator();
        renderIsosurfaceSection(gridMeshes);
    }
}

void ChargeDensityUI::renderDataInformation() {
    if (!m_fileLoaded && m_gridDataEntries.empty()) {
        return;
    }
    renderInfoSection();
}

void ChargeDensityUI::renderSliceViewer() {
    if (!m_renderer || !m_renderer->hasData()) {
        ImGui::TextDisabled("No charge density data loaded");
        return;
    }
    renderSliceControls();
    renderSlicePreview();
}

void ChargeDensityUI::renderFileSection() {
    // ImGui::Text("Charge Density Visualization");
    // ImGui::Spacing();

    const auto gridMeshes = collectGridMeshes();
    const bool hasGridEntries = hasGridDataEntries();

    if (m_fileLoaded) {
        // if (m_loadedFromGrid) {
        //     ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Loaded: %s (Grid)", m_loadedFileName.c_str());
        // } else {
        //     ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Loaded: %s", m_loadedFileName.c_str());
        // }

        if (m_loadedFromGrid && hasGridEntries) {
            std::vector<const char*> gridNames;
            gridNames.reserve(m_gridDataEntries.size());
            for (const auto& entry : m_gridDataEntries) {
                gridNames.push_back(entry.name.c_str());
            }

            ImGui::Text("Mesh");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-1);
            int selectedIndex = m_selectedGridDataIndex;
            if (ImGui::Combo("##GridDataSelectLoaded", &selectedIndex,
                             gridNames.data(), static_cast<int>(gridNames.size()))) {
                m_selectedGridDataIndex = selectedIndex;
                loadFromGridEntry(m_selectedGridDataIndex);
            }
        }

        // const bool allowClear = m_loadedFromGrid || hasGridEntries;
        // if (allowClear) {
        //     if (ImGui::Button("Clear Data", ImVec2(-1, 0))) {
        //         clear();
        //     }
        // }

    } else if (hasGridEntries) {
        std::vector<const char*> gridNames;
        gridNames.reserve(m_gridDataEntries.size());
        for (const auto& entry : m_gridDataEntries) {
            gridNames.push_back(entry.name.c_str());
        }

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Grid data available");
        ImGui::Text("Grid Data");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##GridDataSelect", &m_selectedGridDataIndex,
                     gridNames.data(), static_cast<int>(gridNames.size()));
        if (ImGui::Button("Use Grid Data", ImVec2(-1, 0))) {
            loadFromGridEntry(m_selectedGridDataIndex);
        }
    } else if (!gridMeshes.empty()) {
        std::string structureName = "Grid Data";
        if (m_atomsTemplate) {
            int32_t structureId = m_atomsTemplate->GetCurrentStructureId();
            for (const auto& entry : m_atomsTemplate->GetStructures()) {
                if (entry.id == structureId) {
                    structureName = entry.name;
                    break;
                }
            }
        }
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Loaded: %s (Grid)", structureName.c_str());
    } else {
        ImGui::TextDisabled("No charge density data loaded");
        ImGui::TextDisabled("Use menu: File > Open Structure File");
    }
}

void ChargeDensityUI::renderInfoSection() {
    if (ImGui::TreeNodeEx("Data Information")) {
        const size_t gridCount = m_gridDataEntries.size();
        if (gridCount >= 2) {
            ImGui::Text("Grid Size: %d x %d x %d x %zu Grid data",
                        m_gridShape[0], m_gridShape[1], m_gridShape[2], gridCount);
        } else {
            ImGui::Text("Grid Size: %d x %d x %d", m_gridShape[0], m_gridShape[1], m_gridShape[2]);
        }
        
        const int64_t pointMultiplier = (gridCount >= 2) ? static_cast<int64_t>(gridCount) : 1LL;
        const int64_t totalPoints =
            static_cast<int64_t>(m_gridShape[0]) *
            static_cast<int64_t>(m_gridShape[1]) *
            static_cast<int64_t>(m_gridShape[2]) *
            pointMultiplier;
        if (totalPoints < 1000000LL) {
            const std::string totalPointsText = formatIntegerWithCommas(totalPoints);
            ImGui::Text("Total Points: %s", totalPointsText.c_str());
        } else if (totalPoints < 1000000000LL) {
            const int64_t totalPointsM = totalPoints / 1000000LL;
            const std::string totalPointsText = formatIntegerWithCommas(totalPointsM);
            ImGui::Text("Total Points: %sM", totalPointsText.c_str());
        } else {
            const int64_t totalPointsB = totalPoints / 1000000000LL;
            const std::string totalPointsText = formatIntegerWithCommas(totalPointsB);
            ImGui::Text("Total Points: %sB", totalPointsText.c_str());
        }
        
        ImGui::Spacing();
        ImGui::Text("Value Range:");
        ImGui::Text("  Min: %.4e", m_minValue);
        ImGui::Text("  Max: %.4e", m_maxValue);
        
        ImGui::TreePop();
    }
}

void ChargeDensityUI::renderIsosurfaceSection(const std::vector<GridMeshEntry>& gridMeshes) {
    if (ImGui::TreeNodeEx("Isosurface", ImGuiTreeNodeFlags_DefaultOpen)) {
        
        // ?? ??? ??
        if (!m_fileLoaded) {
            if (hasGridDataEntries()) {
                ImGui::TextDisabled("Grid data available. Select grid data above.");
            } else if (gridMeshes.empty()) {
                ImGui::TextDisabled("No grid data loaded");
            } else {
                bool structureVisible = true;
                if (m_atomsTemplate) {
                    int32_t structureId = m_atomsTemplate->GetCurrentStructureId();
                    structureVisible = m_atomsTemplate->IsStructureVisible(structureId);
                }

                ImGui::BeginDisabled(!structureVisible);
                MeshManager& meshManager = MeshManager::Instance();

                for (const auto& entry : gridMeshes) {
                    bool visible = entry.visible;
                    std::string displayName = entry.name;
                    if (!displayName.empty()) {
                        displayName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(displayName[0])));
                    }
                    std::string label = "Show " + displayName;
                    if (ImGui::Checkbox(label.c_str(), &visible)) {
                        if (visible) {
                            MeshDetail::Instance().SetUiVolumeMeshVisibility(true);
                            meshManager.ShowMesh(entry.id);
                        } else {
                            meshManager.HideMesh(entry.id);
                        }
                    }
                }

                ImGui::EndDisabled();
            }

            ImGui::TreePop();
            return;
        }

        if (ImGui::Checkbox("Show Isosurface", &m_showIsosurface)) {
            updateIsosurface();
        }
        
    if (m_showIsosurface && m_structureVisible) {
            ImGui::Indent();
            
            // ?먮룞 媛?怨꾩궛
            if (ImGui::Checkbox("Auto Value", &m_autoIsoValue)) {
                if (m_autoIsoValue) {
                    m_isoValueDirectInput = false;
                    m_isoValueDirectInputFocus = false;
                }
                if (m_autoIsoValue) {
                    calculateDefaultIsoValue();
                    updateIsosurface();
                }
            }
            
            // ?깆튂硫?媛??щ씪?대뜑
            ImGui::BeginDisabled(m_autoIsoValue);
            
            // 濡쒓렇 ?ㅼ????щ씪?대뜑 (媛?踰붿쐞媛 ?볦쓣 ???좎슜)
            float absMax = std::max(std::abs(m_minValue), std::abs(m_maxValue));
            if (absMax > 0) {
                // ?좏삎 ?щ씪?대뜑 (더블클릭 시 직접 입력 모드)
                if (m_isoValueDirectInput) {
                    if (m_isoValueDirectInputFocus) {
                        ImGui::SetKeyboardFocusHere();
                        m_isoValueDirectInputFocus = false;
                    }

                    float isoInput = m_isoValue;
                    if (ImGui::InputFloat("Iso Value", &isoInput, 0.0f, 0.0f, "%.4e",
                                          ImGuiInputTextFlags_CharsScientific | ImGuiInputTextFlags_EnterReturnsTrue)) {
                        isoInput = std::clamp(isoInput, m_minValue, m_maxValue);
                        if (isoInput != m_isoValue) {
                            m_isoValue = isoInput;
                            updateIsosurface();
                        }
                        m_isoValueDirectInput = false;
                    } else if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_isoValueDirectInput = false;
                    }
                } else {
                    if (ImGui::SliderFloat("Iso Value", &m_isoValue, m_minValue, m_maxValue, "%.4e")) {
                        updateIsosurface();
                    }
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        m_isoValueDirectInput = true;
                        m_isoValueDirectInputFocus = true;
                    }
                }

                // 諛깅텇???щ씪?대뜑 (??吏곴???
                const float valueSpan = (m_maxValue - m_minValue);
                if (valueSpan > 0.0f) {
                    float percentage = (m_isoValue - m_minValue) / valueSpan * 100.0f;
                    if (ImGui::SliderFloat("Level (%)", &percentage, 0.0f, 100.0f, "%.1f%%")) {
                        m_isoValue = m_minValue + valueSpan * percentage / 100.0f;
                        updateIsosurface();
                    }
                }
            }
            
            ImGui::EndDisabled();
            
            // ?됱긽 ?좏깮
            if (ImGui::ColorEdit4("Color##iso", m_isoColor, 
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview)) {
                storeIsoColorForLoadedGrid();
                updateIsosurface();
            }
            
            ImGui::Unindent();
        }
        
        ImGui::Spacing();
        
        if (ImGui::Checkbox("Show +/- Isosurfaces", &m_showMultipleIsosurfaces)) {
            rebuildMultipleIsosurfaces();
            if (m_showMultipleIsosurfaces) {
                SPDLOG_INFO("Multiple isosurfaces enabled");
            }
        }
        
        if (m_showMultipleIsosurfaces) {
            ImGui::Indent();
            
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.1f, 0.1f, 1.0f));
            if (ImGui::SliderFloat("+ Value", &m_isoValuePositive, 0.0f, m_maxValue, "%.4e")) {
                rebuildMultipleIsosurfaces();
            }
            ImGui::PopStyleColor();
            
            if (ImGui::ColorEdit4("+ Color", m_isoColorPositive, 
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview)) {
                rebuildMultipleIsosurfaces();
            }
            
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.3f, 1.0f));
            if (ImGui::SliderFloat("- Value", &m_isoValueNegative, m_minValue, 0.0f, "%.4e")) {
                rebuildMultipleIsosurfaces();
            }
            ImGui::PopStyleColor();
            
            if (ImGui::ColorEdit4("- Color", m_isoColorNegative, 
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview)) {
                rebuildMultipleIsosurfaces();
            }
            
            ImGui::Unindent();
        }
        
        ImGui::TreePop();
    }
}

void ChargeDensityUI::renderSliceControls() {
    bool previewDirty = false;
    const std::string activeSliceKey = getActiveSliceSettingsKey();

    if (ImGui::Checkbox("Show Slice", &m_showSlice)) {
        updateSlice();
        previewDirty = true;
    }
    
    ImGui::BeginDisabled(!m_showSlice);
    ImGui::Indent();
    
    const int millerPlaneValue =
        static_cast<int>(infrastructure::ChargeDensityRenderer::SlicePlane::Miller);

    // Plane preset + Miller mode
    const char* planeNames[] = {
        "XY (Z=const)",
        "XZ (Y=const)",
        "YZ (X=const)",
        "Define plane by Miller indices"
    };
    if (ImGui::Combo("Plane", &m_slicePlane, planeNames, 4)) {
        if (m_renderer) {
            m_renderer->setSlicePlane(
                static_cast<infrastructure::ChargeDensityRenderer::SlicePlane>(m_slicePlane)
            );
            m_renderer->setSliceMillerIndices(m_sliceMillerH, m_sliceMillerK, m_sliceMillerL);
        }
        previewDirty = true;
    }

    const bool isMillerMode = (m_slicePlane == millerPlaneValue);
    if (isMillerMode) {
        bool millerChanged = false;
        millerChanged |= ImGui::SliderInt("h", &m_sliceMillerH, 0, 6);
        millerChanged |= ImGui::SliderInt("k", &m_sliceMillerK, 0, 6);
        millerChanged |= ImGui::SliderInt("l", &m_sliceMillerL, 0, 6);
        if (millerChanged) {
            if (m_renderer) {
                m_renderer->setSliceMillerIndices(m_sliceMillerH, m_sliceMillerK, m_sliceMillerL);
            }
            previewDirty = true;
        }
        if (m_sliceMillerH == 0 && m_sliceMillerK == 0 && m_sliceMillerL == 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.35f, 1.0f),
                               "h, k, l cannot all be 0.");
        }
    }
    
    // Slice position
    const char* axisLabels[] = { "Z Position", "Y Position", "X Position" };
    const char* positionLabel = isMillerMode ? "Position" : axisLabels[m_slicePlane];
    if (ImGui::SliderFloat(positionLabel, &m_slicePosition, 0.0f, 1.0f, "%.3f")) {
        if (m_renderer) {
            m_renderer->setSlicePosition(m_slicePosition);
        }
        previewDirty = true;
    }
    
    if (isMillerMode) {
        const int maxDim = std::max({m_gridShape[0], m_gridShape[1], m_gridShape[2]});
        const int maxIndex = std::max(0, maxDim - 1);
        int gridIndex = static_cast<int>(std::round(m_slicePosition * static_cast<float>(maxIndex)));
        gridIndex = std::clamp(gridIndex, 0, maxIndex);
        if (ImGui::SliderInt("Grid Index", &gridIndex, 0, maxIndex)) {
            if (maxIndex > 0) {
                m_slicePosition = static_cast<float>(gridIndex) / static_cast<float>(maxIndex);
            } else {
                m_slicePosition = 0.0f;
            }
            if (m_renderer) {
                m_renderer->setSlicePosition(m_slicePosition);
            }
            previewDirty = true;
        }
    } else {
        int gridIndex = 0;
        switch (m_slicePlane) {
            case 0: gridIndex = static_cast<int>(m_slicePosition * m_gridShape[2]); break;
            case 1: gridIndex = static_cast<int>(m_slicePosition * m_gridShape[1]); break;
            case 2: gridIndex = static_cast<int>(m_slicePosition * m_gridShape[0]); break;
            default: break;
        }
        const int maxIndex = (m_slicePlane == 0) ? std::max(0, m_gridShape[2] - 1)
                           : (m_slicePlane == 1) ? std::max(0, m_gridShape[1] - 1)
                                                 : std::max(0, m_gridShape[0] - 1);
        if (ImGui::SliderInt("Grid Index", &gridIndex, 0, maxIndex)) {
            m_slicePosition = static_cast<float>(gridIndex) / static_cast<float>(maxIndex + 1);
            if (m_renderer) {
                m_renderer->setSlicePosition(m_slicePosition);
            }
            previewDirty = true;
        }
    }
    
    ImGui::Spacing();
    
    // ???????????
    if (ImGui::Checkbox("Auto Range", &m_autoRange)) {
        if (m_autoRange) {
            m_displayMin = m_minValue;
            m_displayMax = m_maxValue;
        }
        storeSliceDisplaySettingsForLoadedGrid();
        if (m_renderer) {
            syncSliceSettingsForRenderer(m_renderer.get(), activeSliceKey);
        }
        previewDirty = true;
    }
    
    ImGui::BeginDisabled(m_autoRange);
    
    const float rangeLower = std::min(m_minValue, m_maxValue);
    const float rangeUpper = std::max(m_minValue, m_maxValue);
    const ImGuiSliderFlags rangeSliderFlags = ImGuiSliderFlags_AlwaysClamp;
    bool applyRangeNow = false;

    if (ImGui::SliderFloat("Display Min", &m_displayMin,
                           rangeLower, m_displayMax, "%.3f", rangeSliderFlags)) {
        const bool isTextEditing = ImGui::IsItemActive() && !ImGui::IsMouseDown(ImGuiMouseButton_Left);
        if (!isTextEditing) {
            applyRangeNow = true;
        }
    }
    const bool minEditFinished = ImGui::IsItemDeactivatedAfterEdit();

    if (ImGui::SliderFloat("Display Max", &m_displayMax,
                           m_displayMin, rangeUpper, "%.3f", rangeSliderFlags)) {
        const bool isTextEditing = ImGui::IsItemActive() && !ImGui::IsMouseDown(ImGuiMouseButton_Left);
        if (!isTextEditing) {
            applyRangeNow = true;
        }
    }
    const bool maxEditFinished = ImGui::IsItemDeactivatedAfterEdit();

    if (minEditFinished || maxEditFinished) {
        applyRangeNow = true;
    }

    if (applyRangeNow && m_renderer) {
        if (m_displayMin > m_displayMax) {
            std::swap(m_displayMin, m_displayMax);
        }
        storeSliceDisplaySettingsForLoadedGrid();
        syncSliceSettingsForRenderer(m_renderer.get(), activeSliceKey);
        previewDirty = true;
    }
    
    ImGui::EndDisabled();
    
    ImGui::Unindent();
    ImGui::EndDisabled();

    if (previewDirty) {
        markSlicePreviewCachesDirty();
    }

    if (m_showSlice && m_slicePreviewDirty) {
        updateSlicePreview();
    }
}

void ChargeDensityUI::renderSlicePreview() {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Slice Preview");
    const auto targets = collectSliceRenderTargets();
    const bool hasSliceTargets = !targets.empty();
    const float buttonWidth = ImGui::CalcTextSize("PNG").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    float rightX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonWidth;
    ImGui::SameLine();
    ImGui::SetCursorPosX(rightX);
    ImGui::BeginDisabled(!m_showSlice || !hasSliceTargets);
    if (ImGui::SmallButton("PNG")) {
#ifdef __EMSCRIPTEN__
        infrastructure::ChargeDensityRenderer* pngRenderer = nullptr;
        std::string pngKey;
        if (!m_slicePopupKey.empty()) {
            auto popupTargetIt = std::find_if(targets.begin(), targets.end(),
                [&](const SliceRenderTarget& target) {
                    return target.key == m_slicePopupKey;
                });
            if (popupTargetIt != targets.end()) {
                pngRenderer = popupTargetIt->renderer;
                pngKey = popupTargetIt->key;
            }
        }
        if (!pngRenderer && !targets.empty()) {
            if (m_loadedFromGrid) {
                auto visibleIt = std::find_if(targets.begin(), targets.end(),
                    [&](const SliceRenderTarget& target) {
                        return isSliceGridVisibleByKey(target.key);
                    });
                if (visibleIt != targets.end()) {
                    pngRenderer = visibleIt->renderer;
                    pngKey = visibleIt->key;
                }
            }
        }
        if (!pngRenderer && !targets.empty()) {
            pngRenderer = targets.front().renderer;
            pngKey = targets.front().key;
        }
        if (pngRenderer) {
            syncSliceSettingsForRenderer(pngRenderer, pngKey);
        }
        std::string baseName;
        if (m_atomsTemplate) {
            const std::string& loadedName = m_atomsTemplate->GetLoadedFileName();
            if (!loadedName.empty()) {
                baseName = loadedName;
            }
        }
        if (baseName.empty()) {
            baseName = m_loadedFileName;
        }
        if (baseName.empty() && m_atomsTemplate) {
            int32_t structureId = m_atomsTemplate->GetCurrentStructureId();
            for (const auto& entry : m_atomsTemplate->GetStructures()) {
                if (entry.id == structureId && !entry.name.empty()) {
                    baseName = entry.name;
                    break;
                }
            }
        }
        if (baseName.empty()) {
            baseName = "slice";
        }
        m_sliceDownloadName = baseName + ".slice.png";
        bool hasDownloadImage = false;
        if (pngRenderer) {
                hasDownloadImage = pngRenderer->captureRenderedSliceImage(
                    m_sliceDownloadPixels,
                    m_sliceDownloadWidth,
                    m_sliceDownloadHeight);
            if (!hasDownloadImage) {
                hasDownloadImage = buildSliceImageForRenderer(
                    pngRenderer,
                    pngKey,
                    2000,
                    2000,
                    4096,
                    m_sliceDownloadPixels,
                    m_sliceDownloadWidth,
                    m_sliceDownloadHeight);
            }
        }
        if (hasDownloadImage) {
            download_slice_png(m_sliceDownloadPixels.data(),
                               m_sliceDownloadWidth,
                               m_sliceDownloadHeight,
                               m_sliceDownloadName.c_str(),
                               static_cast<int>(m_sliceDownloadName.size()));
        }
#endif
    }
    ImGui::EndDisabled();

    if (!m_showSlice) {
        ImGui::TextDisabled("Slice is hidden");
        return;
    }

    if (!hasSliceTargets) {
        ImGui::TextDisabled("Preview not available");
        return;
    }

    if (m_slicePreviewDirty) {
        updateSlicePreview();
    }

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float targetWidth = avail.x > 0.0f ? avail.x : 1.0f;

    for (const auto& target : targets) {
        auto it = m_slicePreviewCaches.find(target.key);
        if (it == m_slicePreviewCaches.end()) {
            continue;
        }
        SlicePreviewCache& cache = it->second;
        if (cache.dirty) {
            updateSlicePreviewForTarget(target);
        }
        if (!cache.ready || cache.texture == 0 || cache.width <= 0 || cache.height <= 0) {
            continue;
        }

        float aspect = static_cast<float>(cache.width) / static_cast<float>(cache.height);
        float width = targetWidth;
        float height = width / aspect;
        const float maxHeight = 260.0f;
        if (height > maxHeight) {
            height = maxHeight;
            width = height * aspect;
        }

        ImGui::Image((ImTextureID)(uintptr_t)cache.texture,
                     ImVec2(width, height),
                     cache.flipVerticallyWhenRendering ? ImVec2(0.0f, 1.0f) : ImVec2(0.0f, 0.0f),
                     cache.flipVerticallyWhenRendering ? ImVec2(1.0f, 0.0f) : ImVec2(1.0f, 1.0f));
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_slicePopupKey = target.key;
            ImGui::OpenPopup(("Slice Original##" + target.key).c_str());
        }
        ImGui::TextDisabled("%s", target.label.c_str());

        const std::string popupId = "Slice Original##" + target.key;
        const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        const float popupMargin = 20.0f;
        const float availableWidth = std::max(0.0f, displaySize.x - popupMargin * 2.0f);
        const float popupWidth = std::max(320.0f, availableWidth * 0.8f);
        const float popupHeight = std::max(240.0f, displaySize.y - popupMargin * 2.0f);
        const float popupX = std::max(popupMargin, (displaySize.x - popupWidth) * 0.5f);
        ImGui::SetNextWindowPos(ImVec2(popupX, popupMargin), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(popupWidth, popupHeight), ImGuiCond_Appearing);
        if (ImGui::BeginPopupModal(popupId.c_str(), nullptr)) {
            ImGui::Text("%s", target.label.c_str());
            ImGui::Separator();

            const float buttonHeight = ImGui::GetFrameHeightWithSpacing();
            ImVec2 imageArea = ImGui::GetContentRegionAvail();
            imageArea.y = std::max(80.0f, imageArea.y - buttonHeight);
            const std::string childId = "##SliceOriginalFit" + target.key;
            if (ImGui::BeginChild(childId.c_str(), imageArea, true,
                                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                const ImVec2 avail = ImGui::GetContentRegionAvail();
                const ImVec2 framebufferScale = ImGui::GetIO().DisplayFramebufferScale;
                const float dprX = std::max(1.0f, framebufferScale.x);
                const float dprY = std::max(1.0f, framebufferScale.y);
                const int requestedWidth = std::max(1, static_cast<int>(std::round(avail.x * dprX)));
                const int requestedHeight = std::max(1, static_cast<int>(std::round(avail.y * dprY)));

                SlicePreviewCache& popupCache = m_slicePopupCaches[target.key];
                const bool popupSizeChanged = popupCache.requestedWidth != requestedWidth ||
                                              popupCache.requestedHeight != requestedHeight;
                if (popupSizeChanged || popupCache.dirty || !popupCache.ready || popupCache.texture == 0) {
                    popupCache.ready = false;
                    popupCache.dirty = false;
                    popupCache.requestedWidth = requestedWidth;
                    popupCache.requestedHeight = requestedHeight;
                    if (target.renderer) {
                        syncSliceSettingsForRenderer(target.renderer, target.key);
                        bool captured = target.renderer->captureRenderedSliceImage(
                            popupCache.pixels,
                            popupCache.width,
                            popupCache.height);
                        popupCache.flipVerticallyWhenRendering = !captured;
                        if (!captured) {
                            captured = buildSliceImageForRenderer(target.renderer,
                                                                  target.key,
                                                                  requestedWidth,
                                                                  requestedHeight,
                                                                  4096,
                                                                  popupCache.pixels,
                                                                  popupCache.width,
                                                                  popupCache.height);
                            popupCache.flipVerticallyWhenRendering = true;
                        }
                        if (captured) {
                            uploadSlicePreviewTexture(popupCache);
                            popupCache.ready = (popupCache.texture != 0);
                        }
                    }
                }

                const SlicePreviewCache* displayCache = &popupCache;
                if (!popupCache.ready || popupCache.texture == 0 ||
                    popupCache.width <= 0 || popupCache.height <= 0) {
                    displayCache = &cache;
                }

                const float srcW = std::max(1.0f, static_cast<float>(displayCache->width));
                const float srcH = std::max(1.0f, static_cast<float>(displayCache->height));
                const float scaleX = avail.x / srcW;
                const float scaleY = avail.y / srcH;
                const float scale = std::max(0.01f, std::min(scaleX, scaleY));
                const float drawW = srcW * scale;
                const float drawH = srcH * scale;

                const float offsetX = std::max(0.0f, (avail.x - drawW) * 0.5f);
                const float offsetY = std::max(0.0f, (avail.y - drawH) * 0.5f);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);

                ImGui::Image((ImTextureID)(uintptr_t)displayCache->texture,
                             ImVec2(drawW, drawH),
                             displayCache->flipVerticallyWhenRendering ? ImVec2(0.0f, 1.0f) : ImVec2(0.0f, 0.0f),
                             displayCache->flipVerticallyWhenRendering ? ImVec2(1.0f, 0.0f) : ImVec2(1.0f, 1.0f));
            }
            ImGui::EndChild();
            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Spacing();
    }
}

void ChargeDensityUI::updateIsosurface() {
    if (!m_renderer) return;
    storeIsoColorForLoadedGrid();

    auto applyToRenderer = [&](infrastructure::ChargeDensityRenderer* renderer,
                               const std::string& gridKey,
                               bool gridVisible) {
        if (!renderer) {
            return;
        }
        const bool shouldShow = m_showIsosurface && m_structureVisible && gridVisible;
        renderer->setIsosurfaceVisible(shouldShow);
        if (shouldShow) {
            const std::array<float, 4> color = gridKey.empty()
                ? std::array<float, 4>{ m_isoColor[0], m_isoColor[1], m_isoColor[2], m_isoColor[3] }
                : getIsoColorForGridKey(gridKey);
            const float isoValue = gridKey.empty()
                ? m_isoValue
                : getIsoValueForGridKey(gridKey);
            renderer->setIsosurfaceValue(isoValue);
            renderer->setIsosurfaceColor(
                color[0], color[1], color[2], color[3]);
        }
    };

    if (!m_loadedFromGrid || m_gridDataEntries.empty()) {
        applyToRenderer(m_renderer.get(), std::string(), true);
        return;
    }

    const std::string activeKey = toLowerCopy(m_loadedFileName);
    applyToRenderer(m_renderer.get(), activeKey, isSimpleGridVisibleByKey(activeKey));

    for (auto& [key, renderer] : m_auxRenderers) {
        applyToRenderer(renderer.get(), key, isSimpleGridVisibleByKey(key));
    }
}

void ChargeDensityUI::rebuildMultipleIsosurfaces() {
    if (!m_renderer) return;

    auto applyToRenderer = [&](infrastructure::ChargeDensityRenderer* renderer, bool gridVisible) {
        if (!renderer) {
            return;
        }
        renderer->clearIsosurfaces();
        if (!m_showMultipleIsosurfaces || !renderer->hasData() || !m_structureVisible || !gridVisible) {
            return;
        }

        if (m_isoValuePositive >= m_minValue && m_isoValuePositive <= m_maxValue) {
            renderer->addIsosurface(
                m_isoValuePositive,
                m_isoColorPositive[0], m_isoColorPositive[1],
                m_isoColorPositive[2], m_isoColorPositive[3]);
        }

        if (m_isoValueNegative >= m_minValue && m_isoValueNegative <= m_maxValue) {
            renderer->addIsosurface(
                m_isoValueNegative,
                m_isoColorNegative[0], m_isoColorNegative[1],
                m_isoColorNegative[2], m_isoColorNegative[3]);
        }
    };

    if (!m_loadedFromGrid || m_gridDataEntries.empty()) {
        applyToRenderer(m_renderer.get(), true);
        return;
    }

    const std::string activeKey = toLowerCopy(m_loadedFileName);
    applyToRenderer(m_renderer.get(), isSimpleGridVisibleByKey(activeKey));

    for (auto& [key, renderer] : m_auxRenderers) {
        applyToRenderer(renderer.get(), isSimpleGridVisibleByKey(key));
    }
}

void ChargeDensityUI::updateAllRendererVisibility() {
    updateIsosurface();
    rebuildMultipleIsosurfaces();
    updateSlice();
}

void ChargeDensityUI::updateSlice() {
    if (!m_renderer) {
        return;
    }

    auto applyToRenderer = [&](infrastructure::ChargeDensityRenderer* renderer,
                               const std::string& key,
                               bool gridVisible) {
        if (!renderer) {
            return;
        }
        syncSliceSettingsForRenderer(renderer, key);
        const bool shouldShow = m_showSlice && m_structureVisible && gridVisible;
        renderer->setSliceVisible(shouldShow);
    };

    if (!m_loadedFromGrid || m_gridDataEntries.empty()) {
        applyToRenderer(m_renderer.get(), getActiveSliceSettingsKey(), true);
        return;
    }

    const std::string activeKey = toLowerCopy(m_loadedFileName);
    applyToRenderer(m_renderer.get(), activeKey, isSliceGridVisibleByKey(activeKey));

    for (auto& [key, renderer] : m_auxRenderers) {
        applyToRenderer(renderer.get(), key, isSliceGridVisibleByKey(key));
    }
}

void ChargeDensityUI::calculateDefaultIsoValue() {
    // 湲곕낯媛? 理쒕?媛믪쓽 10% ?먮뒗 ?됯퇏 + ?쒖??몄감
    // 媛꾨떒??理쒕?媛믪쓽 10%濡??ㅼ젙
    if (m_maxValue > 0) {
        m_isoValue = m_maxValue * 0.1f;
    } else if (m_minValue < 0) {
        m_isoValue = m_minValue * 0.1f;
    } else {
        m_isoValue = (m_minValue + m_maxValue) * 0.5f;
    }
    
    // ?? ??? ???
    m_isoValuePositive = m_maxValue * 0.1f;
    m_isoValueNegative = m_minValue * 0.1f;
    
    SPDLOG_DEBUG("Default iso value: {:.4e}", m_isoValue);
}

bool ChargeDensityUI::loadFromParseResult(
    const infrastructure::ChgcarParser::ParseResult& result) 
{
    SPDLOG_INFO("Loading charge density from parse result");
    return loadFromParseResultInternal(result, "CHGCAR", false, true);
}

void ChargeDensityUI::setGridDataEntries(std::vector<GridDataEntry>&& entries) {
    m_gridDataEntries = std::move(entries);
    m_selectedGridDataIndex = 0;
    m_showSlice = false;
    for (auto& [_, renderer] : m_auxRenderers) {
        if (renderer) {
            renderer->clear();
        }
    }
    m_auxRenderers.clear();
    m_simpleGridVisibility.clear();
    m_sliceGridVisibility.clear();
    m_simpleGridIsoValues.clear();
    m_simpleGridIsoColors.clear();
    m_sliceDisplaySettings.clear();
    const std::array<float, 4> defaultColor = { m_isoColor[0], m_isoColor[1], m_isoColor[2], m_isoColor[3] };
    for (const auto& entry : m_gridDataEntries) {
        const std::string key = toLowerCopy(entry.name);
        m_simpleGridVisibility[key] = false;
        m_sliceGridVisibility[key] = false;
        m_simpleGridIsoColors[key] = defaultColor;
    }
    if (!m_gridDataEntries.empty()) {
        m_simpleGridVisibility[toLowerCopy(m_gridDataEntries.front().name)] = true;
    }
    if (!m_gridDataEntries.empty() && (!m_fileLoaded || m_loadedFromGrid)) {
        loadFromGridEntry(0);
    }
}

bool ChargeDensityUI::hasGridDataEntries() const {
    return !m_gridDataEntries.empty();
}

bool ChargeDensityUI::isAnySimpleGridVisible() const {
    if (m_gridDataEntries.empty()) {
        return false;
    }
    for (const auto& entry : m_gridDataEntries) {
        const std::string key = toLowerCopy(entry.name);
        auto it = m_simpleGridVisibility.find(key);
        if (it != m_simpleGridVisibility.end() && it->second) {
            return true;
        }
    }
    return false;
}

bool ChargeDensityUI::isAnySliceGridVisible() const {
    if (m_gridDataEntries.empty()) {
        return m_showSlice;
    }
    for (const auto& entry : m_gridDataEntries) {
        const std::string key = toLowerCopy(entry.name);
        auto it = m_sliceGridVisibility.find(key);
        if (it != m_sliceGridVisibility.end() && it->second) {
            return true;
        }
    }
    return false;
}

bool ChargeDensityUI::isAnyIsosurfaceVisible() const {
    const bool hasVisibleMode = m_showIsosurface || m_showMultipleIsosurfaces;
    if (!hasVisibleMode || !m_structureVisible) {
        return false;
    }
    if (!m_loadedFromGrid || m_gridDataEntries.empty()) {
        return true;
    }
    return isAnySimpleGridVisible();
}

std::vector<ChargeDensityUI::SimpleGridEntry> ChargeDensityUI::getSimpleGridEntries() const {
    std::vector<SimpleGridEntry> entries;
    entries.reserve(m_gridDataEntries.size());
    for (const auto& entry : m_gridDataEntries) {
        SimpleGridEntry item;
        item.name = entry.name;
        item.visible = isSimpleGridVisibleByKey(toLowerCopy(entry.name));
        entries.push_back(std::move(item));
    }
    return entries;
}

std::vector<ChargeDensityUI::SliceGridEntry> ChargeDensityUI::getSliceGridEntries() const {
    std::vector<SliceGridEntry> entries;
    if (m_loadedFromGrid && !m_gridDataEntries.empty()) {
        entries.reserve(m_gridDataEntries.size());
        for (const auto& entry : m_gridDataEntries) {
            SliceGridEntry item;
            item.name = entry.name;
            item.visible = isSliceGridVisibleByKey(toLowerCopy(entry.name));
            entries.push_back(std::move(item));
        }
        return entries;
    }
    if (m_renderer && m_renderer->hasData()) {
        SliceGridEntry item;
        item.name = m_loadedFileName.empty() ? "Charge Density" : m_loadedFileName;
        item.visible = m_showSlice;
        entries.push_back(std::move(item));
    }
    return entries;
}

bool ChargeDensityUI::setSimpleGridVisible(const std::string& gridName, bool visible) {
    return handleGridMeshVisibilityChange(gridName, visible);
}

bool ChargeDensityUI::setSliceGridVisible(const std::string& gridName, bool visible) {
    if (!m_loadedFromGrid || m_gridDataEntries.empty()) {
        setSliceVisible(visible);
        return true;
    }
    if (gridName.empty()) {
        return false;
    }

    const int index = findGridEntryIndexByName(gridName);
    if (index < 0) {
        return false;
    }

    const std::string canonicalName = m_gridDataEntries[index].name;
    const std::string key = toLowerCopy(canonicalName);
    m_sliceGridVisibility[key] = visible;

    if (visible) {
        if (!m_fileLoaded || !m_loadedFromGrid) {
            m_selectedGridDataIndex = index;
            if (!loadFromGridEntry(index)) {
                return false;
            }
        } else if (!equalsIgnoreCase(m_loadedFileName, canonicalName)) {
            if (!ensureAuxRendererForGrid(canonicalName)) {
                return false;
            }
        }
        m_showSlice = true;
    } else if (!isAnySliceGridVisible()) {
        m_showSlice = false;
    }

    updateSlice();
    markSlicePreviewCachesDirty();
    return true;
}

bool ChargeDensityUI::selectSimpleGridByName(const std::string& gridName) {
    if (gridName.empty() || m_gridDataEntries.empty()) {
        return false;
    }

    const int index = findGridEntryIndexByName(gridName);
    if (index < 0) {
        return false;
    }

    m_selectedGridDataIndex = index;
    const std::string canonicalName = m_gridDataEntries[index].name;

    if (!m_fileLoaded || !m_loadedFromGrid || !equalsIgnoreCase(m_loadedFileName, canonicalName)) {
        if (!loadFromGridEntryInternal(index, false, false)) {
            return false;
        }
    }

    updateAllRendererVisibility();
    return true;
}

bool ChargeDensityUI::selectSliceGridByName(const std::string& gridName) {
    if (gridName.empty() || m_gridDataEntries.empty()) {
        return false;
    }

    const int index = findGridEntryIndexByName(gridName);
    if (index < 0) {
        return false;
    }

    const std::string canonicalName = m_gridDataEntries[index].name;
    if (!m_fileLoaded || !m_loadedFromGrid) {
        m_selectedGridDataIndex = index;
        if (!loadFromGridEntry(index)) {
            return false;
        }
    } else if (!equalsIgnoreCase(m_loadedFileName, canonicalName)) {
        if (!ensureAuxRendererForGrid(canonicalName)) {
            return false;
        }
    }

    for (const auto& entry : m_gridDataEntries) {
        m_sliceGridVisibility[toLowerCopy(entry.name)] = false;
    }
    m_sliceGridVisibility[toLowerCopy(canonicalName)] = true;
    m_showSlice = true;
    m_selectedGridDataIndex = index;

    updateSlice();
    markSlicePreviewCachesDirty();
    return true;
}

void ChargeDensityUI::setAllSimpleGridVisible(bool visible) {
    if (m_gridDataEntries.empty()) {
        if (!visible) {
            setIsosurfaceGroupVisible(false);
        }
        return;
    }

    if (visible && !m_showIsosurface && !m_showMultipleIsosurfaces) {
        m_showIsosurface = true;
    }

    for (const auto& entry : m_gridDataEntries) {
        const std::string key = toLowerCopy(entry.name);
        m_simpleGridVisibility[key] = visible;
        if (visible) {
            ensureAuxRendererForGrid(entry.name);
        }
    }

    updateAllRendererVisibility();
}

void ChargeDensityUI::setAllSliceGridVisible(bool visible) {
    if (!m_loadedFromGrid || m_gridDataEntries.empty()) {
        setSliceVisible(visible);
        return;
    }

    for (const auto& entry : m_gridDataEntries) {
        const std::string key = toLowerCopy(entry.name);
        m_sliceGridVisibility[key] = visible;
        if (visible) {
            ensureAuxRendererForGrid(entry.name);
        }
    }

    m_showSlice = visible;
    updateSlice();
    markSlicePreviewCachesDirty();
}

bool ChargeDensityUI::handleGridMeshVisibilityChange(const std::string& gridName, bool visible) {
    if (gridName.empty() || m_gridDataEntries.empty()) {
        return false;
    }

    int index = findGridEntryIndexByName(gridName);
    if (index < 0) {
        return false;
    }

    const std::string canonicalName = m_gridDataEntries[index].name;
    const std::string key = toLowerCopy(canonicalName);
    m_simpleGridVisibility[key] = visible;

    if (visible) {
        if (!m_showIsosurface && !m_showMultipleIsosurfaces) {
            m_showIsosurface = true;
        }

        if (!m_fileLoaded || !m_loadedFromGrid) {
            m_selectedGridDataIndex = index;
            if (!loadFromGridEntry(index)) {
                return false;
            }
        } else if (!equalsIgnoreCase(m_loadedFileName, canonicalName)) {
            if (!ensureAuxRendererForGrid(canonicalName)) {
                return false;
            }
        }
    }

    updateAllRendererVisibility();
    return true;
}

bool ChargeDensityUI::ensureSliceDataForGrid(const std::string& gridName) {
    if (m_renderer && m_fileLoaded && !m_loadedFromGrid && m_renderer->hasData()) {
        return true;
    }

    if (gridName.empty() || m_gridDataEntries.empty()) {
        return false;
    }

    int index = findGridEntryIndexByName(gridName);
    if (index < 0) {
        return false;
    }

    if (m_loadedFromGrid && equalsIgnoreCase(m_loadedFileName, gridName) && m_fileLoaded) {
        return true;
    }

    return loadFromGridEntryInternal(index, false, false);
}

bool ChargeDensityUI::loadFromParseResultInternal(
    const infrastructure::ChgcarParser::ParseResult& result,
    const std::string& name,
    bool loadedFromGrid,
    bool resetIsosurface)
{
    if (!result.success) {
        SPDLOG_ERROR("Invalid parse result");
        return false;
    }

    storeSliceDisplaySettingsForLoadedGrid();

    if (!m_renderer) {
        m_renderer = std::make_unique<infrastructure::ChargeDensityRenderer>(
            m_atomsTemplate->vtkRenderer()
        );
    }

    m_renderer->clear();

    if (!m_renderer->loadFromParseResult(result)) {
        SPDLOG_ERROR("Failed to load charge density from parse result");
        m_fileLoaded = false;
        m_loadedFromGrid = false;
        return false;
    }

    m_loadedFileName = name.empty() ? "CHGCAR" : name;

    m_gridShape = m_renderer->getGridShape();
    m_minValue = m_renderer->getMinValue();
    m_maxValue = m_renderer->getMaxValue();
    m_fileLoaded = true;
    m_loadedFromGrid = loadedFromGrid;
    std::string activeKey;
    if (!loadedFromGrid) {
        for (auto& [_, renderer] : m_auxRenderers) {
            if (renderer) {
                renderer->clear();
            }
        }
        m_auxRenderers.clear();
        m_simpleGridVisibility.clear();
        m_sliceGridVisibility.clear();
        m_simpleGridIsoValues.clear();
        m_simpleGridIsoColors.clear();
        m_sliceDisplaySettings.clear();
    } else {
        activeKey = toLowerCopy(m_loadedFileName);
        auto it = m_auxRenderers.find(activeKey);
        if (it != m_auxRenderers.end()) {
            if (it->second) {
                it->second->clear();
            }
            m_auxRenderers.erase(it);
        }
        if (m_simpleGridVisibility.find(activeKey) == m_simpleGridVisibility.end()) {
            m_simpleGridVisibility[activeKey] = true;
        }
        if (m_sliceGridVisibility.empty() && !m_gridDataEntries.empty()) {
            for (const auto& gridEntry : m_gridDataEntries) {
                m_sliceGridVisibility[toLowerCopy(gridEntry.name)] = false;
            }
        }
        if (m_sliceGridVisibility.find(activeKey) == m_sliceGridVisibility.end()) {
            m_sliceGridVisibility[activeKey] = false;
        }
    }

    calculateDefaultIsoValue();
    if (loadedFromGrid) {
        if (m_simpleGridIsoValues.find(activeKey) == m_simpleGridIsoValues.end()) {
            m_simpleGridIsoValues[activeKey] = m_isoValue;
        }
        if (m_simpleGridIsoColors.find(activeKey) == m_simpleGridIsoColors.end()) {
            m_simpleGridIsoColors[activeKey] = { m_isoColor[0], m_isoColor[1], m_isoColor[2], m_isoColor[3] };
        }
        syncIsoColorFromLoadedGrid();
    }

    m_displayMin = m_minValue;
    m_displayMax = m_maxValue;
    m_sliceColorMidpoint = 0.5f;
    m_sliceColorSharpness = 0.0f;
    m_hasSyncedVolumeDisplaySettings = false;
    restoreSliceDisplaySettingsForLoadedGrid();
    rebuildMultipleIsosurfaces();

    if (resetIsosurface) {
        m_showIsosurface = true;
    }
    updateIsosurface();
    if (m_showSlice) {
        updateSlice();
    }
    markSlicePreviewCachesDirty();

    if (m_atomsTemplate) {
        m_atomsTemplate->SetChargeDensityStructureId(m_atomsTemplate->GetCurrentStructureId());
    }

    SPDLOG_INFO("Charge density loaded from result: grid={}x{}x{}, range=[{:.4e}, {:.4e}]",
                m_gridShape[0], m_gridShape[1], m_gridShape[2],
                m_minValue, m_maxValue);

    return true;
}

bool ChargeDensityUI::loadFromGridEntry(int index) {
    return loadFromGridEntryInternal(index, true, true);
}

bool ChargeDensityUI::buildParseResultFromGridEntry(
    int index,
    infrastructure::ChgcarParser::ParseResult& result) const
{
    if (index < 0 || index >= static_cast<int>(m_gridDataEntries.size())) {
        return false;
    }

    const GridDataEntry& entry = m_gridDataEntries[index];
    const int nx = entry.dims[0];
    const int ny = entry.dims[1];
    const int nz = entry.dims[2];
    if (nx <= 0 || ny <= 0 || nz <= 0) {
        SPDLOG_ERROR("Invalid grid dimensions");
        return false;
    }

    const size_t total = static_cast<size_t>(nx) * static_cast<size_t>(ny) * static_cast<size_t>(nz);
    if (entry.values.size() < total) {
        SPDLOG_ERROR("Grid values size mismatch");
        return false;
    }

    result = {};
    result.success = true;
    result.gridShape = { nx, ny, nz };

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            result.lattice[i][j] = entry.vectors[i][j];
        }
    }

    result.density.resize(total);
    float minValue = std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::lowest();

    for (int ix = 0; ix < nx; ++ix) {
        for (int iy = 0; iy < ny; ++iy) {
            for (int iz = 0; iz < nz; ++iz) {
                const size_t xsfIdx = static_cast<size_t>(ix)
                    + static_cast<size_t>(iy) * static_cast<size_t>(nx)
                    + static_cast<size_t>(iz) * static_cast<size_t>(nx) * static_cast<size_t>(ny);
                const size_t vaspIdx = static_cast<size_t>(iz)
                    + static_cast<size_t>(iy) * static_cast<size_t>(nz)
                    + static_cast<size_t>(ix) * static_cast<size_t>(ny) * static_cast<size_t>(nz);

                const float value = entry.values[xsfIdx];
                result.density[vaspIdx] = value;
                if (value < minValue) minValue = value;
                if (value > maxValue) maxValue = value;
            }
        }
    }

    result.minValue = minValue;
    result.maxValue = maxValue;
    return true;
}

bool ChargeDensityUI::loadFromGridEntryInternal(int index, bool resetIsosurface, bool syncVisibility) {
    if (index < 0 || index >= static_cast<int>(m_gridDataEntries.size())) {
        return false;
    }
    storeIsoColorForLoadedGrid();
    storeSliceDisplaySettingsForLoadedGrid();
    infrastructure::ChgcarParser::ParseResult result;
    if (!buildParseResultFromGridEntry(index, result)) {
        return false;
    }

    const GridDataEntry& entry = m_gridDataEntries[index];
    std::string gridName = entry.name.empty() ? "Grid" : entry.name;
    if (resetIsosurface || syncVisibility) {
        m_simpleGridVisibility[toLowerCopy(gridName)] = true;
        if (m_sliceGridVisibility.find(toLowerCopy(gridName)) == m_sliceGridVisibility.end()) {
            m_sliceGridVisibility[toLowerCopy(gridName)] = false;
        }
    }

    if (m_atomsTemplate && m_loadedFromGrid && m_fileLoaded && !equalsIgnoreCase(m_loadedFileName, gridName)) {
        const std::string previousKey = toLowerCopy(m_loadedFileName);
        if (isSimpleGridVisibleByKey(previousKey) && m_auxRenderers.find(previousKey) == m_auxRenderers.end()) {
            const int previousIndex = findGridEntryIndexByName(m_loadedFileName);
            if (previousIndex >= 0) {
                infrastructure::ChgcarParser::ParseResult previousResult;
                if (buildParseResultFromGridEntry(previousIndex, previousResult)) {
                    auto previousRenderer = std::make_unique<infrastructure::ChargeDensityRenderer>(
                        m_atomsTemplate->vtkRenderer());
                    if (previousRenderer->loadFromParseResult(previousResult)) {
                        m_auxRenderers[previousKey] = std::move(previousRenderer);
                    }
                }
            }
        }
    }

    const bool loaded = loadFromParseResultInternal(result, gridName, true, resetIsosurface);
    if (loaded) {
        updateAllRendererVisibility();
    }
    return loaded;
}

int ChargeDensityUI::findGridEntryIndexByName(const std::string& name) const {
    if (name.empty()) {
        return -1;
    }
    const std::string target = toLowerCopy(name);
    for (size_t i = 0; i < m_gridDataEntries.size(); ++i) {
        if (toLowerCopy(m_gridDataEntries[i].name) == target) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool ChargeDensityUI::isSimpleGridVisibleByKey(const std::string& key) const {
    if (key.empty()) {
        return false;
    }
    auto it = m_simpleGridVisibility.find(key);
    if (it != m_simpleGridVisibility.end()) {
        return it->second;
    }
    if (m_loadedFromGrid && toLowerCopy(m_loadedFileName) == key) {
        return true;
    }
    return false;
}

bool ChargeDensityUI::isSliceGridVisibleByKey(const std::string& key) const {
    if (key.empty()) {
        return false;
    }
    auto it = m_sliceGridVisibility.find(key);
    if (it != m_sliceGridVisibility.end()) {
        return it->second;
    }
    if (m_loadedFromGrid && toLowerCopy(m_loadedFileName) == key) {
        return true;
    }
    return false;
}

std::array<float, 4> ChargeDensityUI::getIsoColorForGridKey(const std::string& key) const {
    if (!key.empty()) {
        auto it = m_simpleGridIsoColors.find(key);
        if (it != m_simpleGridIsoColors.end()) {
            return it->second;
        }
    }
    return { m_isoColor[0], m_isoColor[1], m_isoColor[2], m_isoColor[3] };
}

float ChargeDensityUI::getIsoValueForGridKey(const std::string& key) const {
    if (!key.empty()) {
        auto it = m_simpleGridIsoValues.find(key);
        if (it != m_simpleGridIsoValues.end()) {
            return it->second;
        }
    }
    return m_isoValue;
}

void ChargeDensityUI::syncIsoColorFromLoadedGrid() {
    if (!m_loadedFromGrid || m_loadedFileName.empty()) {
        return;
    }
    const std::string key = toLowerCopy(m_loadedFileName);
    const std::array<float, 4> color = getIsoColorForGridKey(key);
    m_isoColor[0] = color[0];
    m_isoColor[1] = color[1];
    m_isoColor[2] = color[2];
    m_isoColor[3] = color[3];
    m_isoValue = std::clamp(getIsoValueForGridKey(key), m_minValue, m_maxValue);
}

void ChargeDensityUI::storeIsoColorForLoadedGrid() {
    if (!m_loadedFromGrid || m_loadedFileName.empty()) {
        return;
    }
    const std::string key = toLowerCopy(m_loadedFileName);
    m_simpleGridIsoColors[key] =
        { m_isoColor[0], m_isoColor[1], m_isoColor[2], m_isoColor[3] };
    m_simpleGridIsoValues[key] = m_isoValue;
}

bool ChargeDensityUI::ensureAuxRendererForGrid(const std::string& gridName) {
    if (!m_atomsTemplate) {
        return false;
    }
    int index = findGridEntryIndexByName(gridName);
    if (index < 0) {
        return false;
    }

    const std::string canonicalName = m_gridDataEntries[index].name;
    const std::string key = toLowerCopy(canonicalName);
    if (m_loadedFromGrid && equalsIgnoreCase(m_loadedFileName, canonicalName)) {
        return true;
    }

    auto it = m_auxRenderers.find(key);
    if (it != m_auxRenderers.end() && it->second && it->second->hasData()) {
        return true;
    }

    infrastructure::ChgcarParser::ParseResult parseResult;
    if (!buildParseResultFromGridEntry(index, parseResult)) {
        return false;
    }

    auto renderer = std::make_unique<infrastructure::ChargeDensityRenderer>(
        m_atomsTemplate->vtkRenderer());
    if (!renderer->loadFromParseResult(parseResult)) {
        return false;
    }
    m_auxRenderers[key] = std::move(renderer);
    return true;
}

void ChargeDensityUI::syncGridMeshVisibility(const std::string& selectedName) {
    if (!m_atomsTemplate) {
        return;
    }

    const auto gridMeshes = collectGridMeshes();
    if (gridMeshes.empty()) {
        return;
    }

    MeshManager& meshManager = MeshManager::Instance();
    for (const auto& entry : gridMeshes) {
        bool shouldShow = false;
        auto it = m_simpleGridVisibility.find(toLowerCopy(entry.name));
        if (it != m_simpleGridVisibility.end()) {
            shouldShow = it->second;
        } else if (!selectedName.empty()) {
            shouldShow = equalsIgnoreCase(entry.name, selectedName);
        }
        if (shouldShow && !entry.visible) {
            MeshDetail::Instance().SetUiVolumeMeshVisibility(true);
            meshManager.ShowMesh(entry.id);
        } else if (!shouldShow && entry.visible) {
            meshManager.HideMesh(entry.id);
        }
    }
}

std::vector<ChargeDensityUI::SliceRenderTarget> ChargeDensityUI::collectSliceRenderTargets() {
    std::vector<SliceRenderTarget> targets;
    if (m_loadedFromGrid && !m_gridDataEntries.empty()) {
        targets.reserve(m_gridDataEntries.size());
        for (size_t i = 0; i < m_gridDataEntries.size(); ++i) {
            const auto& entry = m_gridDataEntries[i];
            std::string label = entry.name.empty()
                ? ("Grid " + std::to_string(i + 1))
                : entry.name;
            std::string key = toLowerCopy(label);
            if (!isSliceGridVisibleByKey(key)) {
                continue;
            }
            infrastructure::ChargeDensityRenderer* renderer = nullptr;
            if (m_renderer && m_renderer->hasData() && equalsIgnoreCase(m_loadedFileName, label)) {
                renderer = m_renderer.get();
            } else {
                ensureAuxRendererForGrid(label);
                auto it = m_auxRenderers.find(key);
                if (it != m_auxRenderers.end() && it->second && it->second->hasData()) {
                    renderer = it->second.get();
                }
            }
            targets.push_back({key, label, renderer});
        }
    } else if (m_renderer && m_renderer->hasData()) {
        const std::string label = m_loadedFileName.empty() ? "Charge Density" : m_loadedFileName;
        targets.push_back({"__main__", label, m_renderer.get()});
    }
    return targets;
}

std::string ChargeDensityUI::getActiveSliceSettingsKey() const {
    if (m_loadedFromGrid && !m_loadedFileName.empty()) {
        return toLowerCopy(m_loadedFileName);
    }
    return kMainSliceSettingsKey;
}

common::ColorMapPreset ChargeDensityUI::resolveSliceColorMapPresetForKey(
    const std::string& key) const {
    const std::string resolvedKey = key.empty() ? getActiveSliceSettingsKey() : toLowerCopy(key);
    if (resolvedKey.empty()) {
        return common::ColorMapPreset::Rainbow;
    }

    const auto gridMeshes = collectGridMeshes();
    auto findPresetForKey = [&](const std::string& meshKey) -> std::optional<common::ColorMapPreset> {
        if (meshKey.empty()) {
            return std::nullopt;
        }
        for (const auto& entry : gridMeshes) {
            if (toLowerCopy(entry.name) != meshKey) {
                continue;
            }
            const Mesh* mesh = MeshManager::Instance().GetMeshById(entry.id);
            if (mesh && mesh->GetVolumeMeshActor()) {
                return mesh->GetVolumeColorPreset();
            }
        }
        return std::nullopt;
    };

    if (auto preset = findPresetForKey(resolvedKey)) {
        return *preset;
    }

    if (resolvedKey == kMainSliceSettingsKey && !m_loadedFileName.empty()) {
        if (auto preset = findPresetForKey(toLowerCopy(m_loadedFileName))) {
            return *preset;
        }
    }

    if (resolvedKey == kMainSliceSettingsKey && gridMeshes.size() == 1) {
        const Mesh* mesh = MeshManager::Instance().GetMeshById(gridMeshes.front().id);
        if (mesh && mesh->GetVolumeMeshActor()) {
            return mesh->GetVolumeColorPreset();
        }
    }

    return common::ColorMapPreset::Rainbow;
}

ChargeDensityUI::SliceDisplaySettings& ChargeDensityUI::ensureSliceDisplaySettingsForKey(
    const std::string& key,
    const infrastructure::ChargeDensityRenderer* renderer) {
    const std::string resolvedKey = key.empty() ? getActiveSliceSettingsKey() : toLowerCopy(key);
    auto it = m_sliceDisplaySettings.find(resolvedKey);
    if (it != m_sliceDisplaySettings.end()) {
        return it->second;
    }

    SliceDisplaySettings defaults;
    float dataMin = m_minValue;
    float dataMax = m_maxValue;
    if (renderer) {
        dataMin = renderer->getMinValue();
        dataMax = renderer->getMaxValue();
    }
    if (dataMax <= dataMin) {
        dataMax = dataMin + 1.0f;
    }
    defaults.displayMin = dataMin;
    defaults.displayMax = dataMax;
    auto inserted = m_sliceDisplaySettings.emplace(resolvedKey, defaults);
    return inserted.first->second;
}

const ChargeDensityUI::SliceDisplaySettings* ChargeDensityUI::findSliceDisplaySettingsForKey(
    const std::string& key) const {
    const std::string resolvedKey = key.empty() ? getActiveSliceSettingsKey() : toLowerCopy(key);
    auto it = m_sliceDisplaySettings.find(resolvedKey);
    if (it == m_sliceDisplaySettings.end()) {
        return nullptr;
    }
    return &it->second;
}

void ChargeDensityUI::storeSliceDisplaySettingsForLoadedGrid() {
    if (!m_fileLoaded) {
        return;
    }

    SliceDisplaySettings& settings = ensureSliceDisplaySettingsForKey(
        getActiveSliceSettingsKey(),
        m_renderer.get());
    settings.autoRange = m_autoRange;
    settings.displayMin = m_displayMin;
    settings.displayMax = m_displayMax;
    settings.colorMidpoint = m_sliceColorMidpoint;
    settings.colorSharpness = m_sliceColorSharpness;
    settings.hasSyncedVolumeDisplaySettings = m_hasSyncedVolumeDisplaySettings;
    settings.lastSyncedVolumeWindow = m_lastSyncedVolumeWindow;
    settings.lastSyncedVolumeLevel = m_lastSyncedVolumeLevel;
    settings.lastSyncedColorPreset = resolveSliceColorMapPresetForKey(getActiveSliceSettingsKey());
    settings.lastSyncedColorMidpoint = m_lastSyncedColorMidpoint;
    settings.lastSyncedColorSharpness = m_lastSyncedColorSharpness;
    settings.lastSyncedDataMin = m_lastSyncedDataMin;
    settings.lastSyncedDataMax = m_lastSyncedDataMax;
}

void ChargeDensityUI::restoreSliceDisplaySettingsForLoadedGrid() {
    if (!m_fileLoaded) {
        return;
    }

    const SliceDisplaySettings& settings = ensureSliceDisplaySettingsForKey(
        getActiveSliceSettingsKey(),
        m_renderer.get());
    m_autoRange = settings.autoRange;
    m_displayMin = settings.displayMin;
    m_displayMax = settings.displayMax;
    if (m_displayMax <= m_displayMin) {
        m_displayMin = m_minValue;
        m_displayMax = (m_maxValue > m_minValue) ? m_maxValue : (m_minValue + 1.0f);
    }
    m_sliceColorMidpoint = std::clamp(settings.colorMidpoint, 0.0f, 1.0f);
    m_sliceColorSharpness = std::clamp(settings.colorSharpness, 0.0f, 1.0f);
    m_hasSyncedVolumeDisplaySettings = settings.hasSyncedVolumeDisplaySettings;
    m_lastSyncedVolumeWindow = settings.lastSyncedVolumeWindow;
    m_lastSyncedVolumeLevel = settings.lastSyncedVolumeLevel;
    m_lastSyncedColorMidpoint = settings.lastSyncedColorMidpoint;
    m_lastSyncedColorSharpness = settings.lastSyncedColorSharpness;
    m_lastSyncedDataMin = settings.lastSyncedDataMin;
    m_lastSyncedDataMax = settings.lastSyncedDataMax;
}

void ChargeDensityUI::syncSliceSettingsForRenderer(infrastructure::ChargeDensityRenderer* renderer,
                                                   const std::string& key) {
    if (!renderer) {
        return;
    }
    renderer->setSliceMillerIndices(m_sliceMillerH, m_sliceMillerK, m_sliceMillerL);
    renderer->setSlicePlane(
        static_cast<infrastructure::ChargeDensityRenderer::SlicePlane>(m_slicePlane)
    );
    renderer->setSlicePosition(m_slicePosition);
    const common::ColorMapPreset colorPreset = resolveSliceColorMapPresetForKey(key);
    renderer->setColorMap(common::ColorMapPresetFromIndex(
        common::ColorMapPresetToIndex(colorPreset),
        common::ColorMapPreset::Viridis));
    const SliceDisplaySettings& settings = ensureSliceDisplaySettingsForKey(key, renderer);
    renderer->setValueRange(settings.displayMin, settings.displayMax);
    renderer->setColorCurve(settings.colorMidpoint, settings.colorSharpness);
}

void ChargeDensityUI::markSlicePreviewCachesDirty() {
    m_slicePreviewDirty = true;
    for (auto& [_, cache] : m_slicePreviewCaches) {
        cache.dirty = true;
    }
    for (auto& [_, cache] : m_slicePopupCaches) {
        cache.dirty = true;
    }
}

void ChargeDensityUI::updateSlicePreviewForTarget(const SliceRenderTarget& target) {
    if (!target.renderer) {
        return;
    }

    syncSliceSettingsForRenderer(target.renderer, target.key);
    SlicePreviewCache& cache = m_slicePreviewCaches[target.key];
    cache.dirty = false;
    cache.ready = false;
    cache.requestedWidth = 0;
    cache.requestedHeight = 0;

    if (!buildSliceImageForRenderer(target.renderer,
                                    target.key,
                                    0,
                                    0,
                                    1024,
                                    cache.pixels,
                                    cache.width,
                                    cache.height)) {
        return;
    }

    cache.flipVerticallyWhenRendering = true;
    uploadSlicePreviewTexture(cache);
    cache.ready = (cache.texture != 0);
}

void ChargeDensityUI::updateSlicePreview() {
    if (!m_showSlice) {
        m_slicePreviewDirty = false;
        return;
    }

    const auto targets = collectSliceRenderTargets();
    std::unordered_set<std::string> activeKeys;
    activeKeys.reserve(targets.size());
    for (const auto& target : targets) {
        activeKeys.insert(target.key);
    }

    for (auto it = m_slicePreviewCaches.begin(); it != m_slicePreviewCaches.end();) {
        if (activeKeys.find(it->first) == activeKeys.end()) {
            if (it->second.texture != 0) {
                glDeleteTextures(1, &it->second.texture);
            }
            it = m_slicePreviewCaches.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_slicePopupCaches.begin(); it != m_slicePopupCaches.end();) {
        if (activeKeys.find(it->first) == activeKeys.end()) {
            if (it->second.texture != 0) {
                glDeleteTextures(1, &it->second.texture);
            }
            it = m_slicePopupCaches.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto& target : targets) {
        SlicePreviewCache& cache = m_slicePreviewCaches[target.key];
        if (m_slicePreviewDirty) {
            cache.dirty = true;
        }
        if (cache.dirty || !cache.ready || cache.texture == 0) {
            updateSlicePreviewForTarget(target);
        }
    }

    m_slicePreviewDirty = false;
}

bool ChargeDensityUI::buildSliceImage(int targetWidth,
                                      int targetHeight,
                                      int millerMaxResolution,
                                      std::vector<uint8_t>& pixels,
                                      int& width,
                                      int& height) const {
    return buildSliceImageForRenderer(m_renderer.get(),
                                      getActiveSliceSettingsKey(),
                                      targetWidth,
                                      targetHeight,
                                      millerMaxResolution,
                                      pixels,
                                      width,
                                      height);
}

bool ChargeDensityUI::buildSliceImageForRenderer(const infrastructure::ChargeDensityRenderer* renderer,
                                                 const std::string& settingsKey,
                                                 int targetWidth,
                                                 int targetHeight,
                                                 int millerMaxResolution,
                                                 std::vector<uint8_t>& pixels,
                                                 int& width,
                                                 int& height) const {
    pixels.clear();
    width = 0;
    height = 0;

    if (!renderer || !m_showSlice) {
        return false;
    }

    std::vector<float> values;
    int srcWidth = 0;
    int srcHeight = 0;
    if (!renderer->getSliceValues(values, srcWidth, srcHeight, millerMaxResolution)) {
        return false;
    }

    if (values.empty() || srcWidth <= 0 || srcHeight <= 0) {
        return false;
    }

    int outWidth = srcWidth;
    int outHeight = srcHeight;
    if (targetWidth > 0 && targetHeight > 0) {
        const double scaleX = static_cast<double>(targetWidth) / static_cast<double>(srcWidth);
        const double scaleY = static_cast<double>(targetHeight) / static_cast<double>(srcHeight);
        const double scale = std::max(1e-6, std::min(scaleX, scaleY));
        outWidth = std::max(1, static_cast<int>(std::round(static_cast<double>(srcWidth) * scale)));
        outHeight = std::max(1, static_cast<int>(std::round(static_cast<double>(srcHeight) * scale)));
    } else if (targetWidth > 0) {
        outWidth = std::max(1, targetWidth);
        outHeight = std::max(1, static_cast<int>(std::round(
            static_cast<double>(outWidth) * static_cast<double>(srcHeight) / static_cast<double>(srcWidth))));
    } else if (targetHeight > 0) {
        outHeight = std::max(1, targetHeight);
        outWidth = std::max(1, static_cast<int>(std::round(
            static_cast<double>(outHeight) * static_cast<double>(srcWidth) / static_cast<double>(srcHeight))));
    }

    width = outWidth;
    height = outHeight;
    pixels.assign(static_cast<size_t>(outWidth) * static_cast<size_t>(outHeight) * 4, 0);

    const SliceDisplaySettings* settings = findSliceDisplaySettingsForKey(settingsKey);
    const double rangeMin = static_cast<double>(settings ? settings->displayMin : renderer->getMinValue());
    double rangeMax = static_cast<double>(settings ? settings->displayMax : renderer->getMaxValue());
    const common::ColorMapPreset colorPreset = resolveSliceColorMapPresetForKey(settingsKey);
    const double colorMidpoint = static_cast<double>(settings ? settings->colorMidpoint : 0.5f);
    const double colorSharpness = static_cast<double>(settings ? settings->colorSharpness : 0.0f);
    if (rangeMax <= rangeMin) {
        rangeMax = rangeMin + 1.0;
    }
    const double midValue = (rangeMin + rangeMax) * 0.5;
    vtkSmartPointer<vtkColorTransferFunction> colorTransfer =
        vtkSmartPointer<vtkColorTransferFunction>::New();
    common::ApplyColorMapToTransferFunction(
        colorTransfer,
        colorPreset,
        rangeMin,
        midValue,
        rangeMax,
        colorMidpoint,
        colorSharpness);

    const double scaleX = static_cast<double>(srcWidth) / static_cast<double>(outWidth);
    const double scaleY = static_cast<double>(srcHeight) / static_cast<double>(outHeight);

    auto sampleValueBilinear = [&](double srcX, double srcY) -> float {
        srcX = std::clamp(srcX, 0.0, static_cast<double>(srcWidth - 1));
        srcY = std::clamp(srcY, 0.0, static_cast<double>(srcHeight - 1));

        const int x0 = static_cast<int>(std::floor(srcX));
        const int y0 = static_cast<int>(std::floor(srcY));
        const int x1 = std::min(x0 + 1, srcWidth - 1);
        const int y1 = std::min(y0 + 1, srcHeight - 1);

        const double tx = srcX - static_cast<double>(x0);
        const double ty = srcY - static_cast<double>(y0);

        const float v00 = values[static_cast<size_t>(y0) * static_cast<size_t>(srcWidth) +
                                 static_cast<size_t>(x0)];
        const float v10 = values[static_cast<size_t>(y0) * static_cast<size_t>(srcWidth) +
                                 static_cast<size_t>(x1)];
        const float v01 = values[static_cast<size_t>(y1) * static_cast<size_t>(srcWidth) +
                                 static_cast<size_t>(x0)];
        const float v11 = values[static_cast<size_t>(y1) * static_cast<size_t>(srcWidth) +
                                 static_cast<size_t>(x1)];

        const double top = static_cast<double>(v00) * (1.0 - tx) + static_cast<double>(v10) * tx;
        const double bottom = static_cast<double>(v01) * (1.0 - tx) + static_cast<double>(v11) * tx;
        return static_cast<float>(top * (1.0 - ty) + bottom * ty);
    };

    for (int y = 0; y < outHeight; ++y) {
        const double srcY = (static_cast<double>(outHeight - 1 - y) + 0.5) * scaleY - 0.5;
        for (int x = 0; x < outWidth; ++x) {
            const double srcX = (static_cast<double>(x) + 0.5) * scaleX - 0.5;
            const float value = sampleValueBilinear(srcX, srcY);
            double rgb[3] = {0.0, 0.0, 0.0};
            colorTransfer->GetColor(static_cast<double>(value), rgb);
            const uint8_t r = static_cast<uint8_t>(std::round(std::clamp(rgb[0], 0.0, 1.0) * 255.0));
            const uint8_t g = static_cast<uint8_t>(std::round(std::clamp(rgb[1], 0.0, 1.0) * 255.0));
            const uint8_t b = static_cast<uint8_t>(std::round(std::clamp(rgb[2], 0.0, 1.0) * 255.0));

            const size_t dstIndex = (static_cast<size_t>(y) * static_cast<size_t>(outWidth) +
                                     static_cast<size_t>(x)) * 4;
            pixels[dstIndex + 0] = r;
            pixels[dstIndex + 1] = g;
            pixels[dstIndex + 2] = b;
            pixels[dstIndex + 3] = 255;
        }
    }

    return true;
}

void ChargeDensityUI::uploadSlicePreviewTexture(SlicePreviewCache& cache) {
    if (cache.pixels.empty() || cache.width <= 0 || cache.height <= 0) {
        return;
    }

    if (cache.texture == 0) {
        glGenTextures(1, &cache.texture);
    }

    glBindTexture(GL_TEXTURE_2D, cache.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (cache.textureWidth != cache.width ||
        cache.textureHeight != cache.height) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cache.width, cache.height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, cache.pixels.data());
        cache.textureWidth = cache.width;
        cache.textureHeight = cache.height;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cache.width, cache.height,
                        GL_RGBA, GL_UNSIGNED_BYTE, cache.pixels.data());
    }
}

void ChargeDensityUI::clearSlicePreview() {
    for (auto& [_, cache] : m_slicePreviewCaches) {
        if (cache.texture != 0) {
            glDeleteTextures(1, &cache.texture);
            cache.texture = 0;
        }
    }
    for (auto& [_, cache] : m_slicePopupCaches) {
        if (cache.texture != 0) {
            glDeleteTextures(1, &cache.texture);
            cache.texture = 0;
        }
    }
    m_slicePreviewCaches.clear();
    m_slicePopupCaches.clear();
    m_slicePreviewDirty = false;
    m_slicePopupKey.clear();
    m_sliceDownloadPixels.clear();
    m_sliceDownloadWidth = 0;
    m_sliceDownloadHeight = 0;
}

void ChargeDensityUI::setIsosurfaceVisible(bool visible) {
    m_showIsosurface = visible;
    updateIsosurface();
}

void ChargeDensityUI::setMultipleIsosurfacesVisible(bool visible) {
    m_showMultipleIsosurfaces = visible;
    rebuildMultipleIsosurfaces();
}

void ChargeDensityUI::setIsosurfaceGroupVisible(bool visible) {
    m_showIsosurface = visible;
    m_showMultipleIsosurfaces = visible;
    if (!visible) {
        for (auto& [key, _] : m_simpleGridVisibility) {
            m_simpleGridVisibility[key] = false;
        }
    } else if (!m_gridDataEntries.empty() && !isAnySimpleGridVisible()) {
        m_simpleGridVisibility[toLowerCopy(m_gridDataEntries.front().name)] = true;
    }
    updateAllRendererVisibility();
}

void ChargeDensityUI::setSliceVisible(bool visible) {
    m_showSlice = visible;
    if (visible && m_loadedFromGrid && !m_gridDataEntries.empty() && !isAnySliceGridVisible()) {
        m_sliceGridVisibility[toLowerCopy(m_gridDataEntries.front().name)] = true;
    }
    updateSlice();
    markSlicePreviewCachesDirty();
}

void ChargeDensityUI::setStructureVisible(bool visible) {
    m_structureVisible = visible;
    updateAllRendererVisibility();
}

void ChargeDensityUI::syncSliceDisplayFromVolume(const std::string& gridName,
                                                 double window,
                                                 double level,
                                                 double colorMidpoint,
                                                 double colorSharpness,
                                                 double dataMin,
                                                 double dataMax) {
    if (!m_renderer) {
        return;
    }

    const std::string targetKey = gridName.empty()
        ? getActiveSliceSettingsKey()
        : toLowerCopy(gridName);

    infrastructure::ChargeDensityRenderer* targetRenderer = nullptr;
    if (!m_loadedFromGrid || m_gridDataEntries.empty()) {
        targetRenderer = m_renderer.get();
    } else if (!gridName.empty() && equalsIgnoreCase(m_loadedFileName, gridName)) {
        targetRenderer = m_renderer.get();
    } else {
        if (!gridName.empty()) {
            ensureAuxRendererForGrid(gridName);
        }
        auto it = m_auxRenderers.find(targetKey);
        if (it != m_auxRenderers.end() && it->second && it->second->hasData()) {
            targetRenderer = it->second.get();
        }
    }

    if (!targetRenderer) {
        return;
    }

    const common::ColorMapPreset targetColorPreset = resolveSliceColorMapPresetForKey(targetKey);
    SliceDisplaySettings& settings = ensureSliceDisplaySettingsForKey(targetKey, targetRenderer);
    if (settings.hasSyncedVolumeDisplaySettings &&
        nearlyEqual(window, settings.lastSyncedVolumeWindow) &&
        nearlyEqual(level, settings.lastSyncedVolumeLevel) &&
        targetColorPreset == settings.lastSyncedColorPreset &&
        nearlyEqual(colorMidpoint, settings.lastSyncedColorMidpoint) &&
        nearlyEqual(colorSharpness, settings.lastSyncedColorSharpness) &&
        nearlyEqual(dataMin, settings.lastSyncedDataMin) &&
        nearlyEqual(dataMax, settings.lastSyncedDataMax)) {
        return;
    }

    const WindowLevelRange resolved = resolveWindowLevelRange(dataMin, dataMax, window, level);
    settings.displayMin = resolved.minValue;
    settings.displayMax = resolved.maxValue;
    settings.colorMidpoint = std::clamp(static_cast<float>(colorMidpoint), 0.0f, 1.0f);
    settings.colorSharpness = std::clamp(static_cast<float>(colorSharpness), 0.0f, 1.0f);
    settings.autoRange = false;
    settings.hasSyncedVolumeDisplaySettings = true;
    settings.lastSyncedVolumeWindow = window;
    settings.lastSyncedVolumeLevel = level;
    settings.lastSyncedColorPreset = targetColorPreset;
    settings.lastSyncedColorMidpoint = colorMidpoint;
    settings.lastSyncedColorSharpness = colorSharpness;
    settings.lastSyncedDataMin = dataMin;
    settings.lastSyncedDataMax = dataMax;

    if (targetKey == getActiveSliceSettingsKey()) {
        m_displayMin = settings.displayMin;
        m_displayMax = settings.displayMax;
        m_sliceColorMidpoint = settings.colorMidpoint;
        m_sliceColorSharpness = settings.colorSharpness;
        m_autoRange = settings.autoRange;
        m_hasSyncedVolumeDisplaySettings = settings.hasSyncedVolumeDisplaySettings;
        m_lastSyncedVolumeWindow = settings.lastSyncedVolumeWindow;
        m_lastSyncedVolumeLevel = settings.lastSyncedVolumeLevel;
        // Colormap is dataset-owned by mesh settings and not exposed in Slice Viewer UI.
        m_lastSyncedColorMidpoint = settings.lastSyncedColorMidpoint;
        m_lastSyncedColorSharpness = settings.lastSyncedColorSharpness;
        m_lastSyncedDataMin = settings.lastSyncedDataMin;
        m_lastSyncedDataMax = settings.lastSyncedDataMax;
    }

    syncSliceSettingsForRenderer(targetRenderer, targetKey);
    markSlicePreviewCachesDirty();
    if (m_showSlice) {
        updateSlicePreview();
    }
}


// ============================================================================
// ?좊땲硫붿씠??愿??硫붿꽌??// ============================================================================

float ChargeDensityUI::getLevelPercent() const {
    if (m_maxValue <= m_minValue) return 0.0f;
    return (m_isoValue - m_minValue) / (m_maxValue - m_minValue);
}

void ChargeDensityUI::setLevelPercent(float percent, bool stopAnim) {
    if (stopAnim && m_isPlaying) {
        stopAnimation();
    }
    
    percent = std::clamp(percent, 0.0f, 1.0f);
    applyIsoValueFromPercent(percent);
}

void ChargeDensityUI::applyIsoValueFromPercent(float percent) {
    m_isoValue = m_minValue + (m_maxValue - m_minValue) * percent;
    
    // Auto value ?댁젣 (?섎룞 議곗옉?대?濡?
    m_autoIsoValue = false;
    
    updateIsosurface();
}

void ChargeDensityUI::startAnimation(float durationSeconds) {
    if (!hasData()) return;
    
    m_isPlaying = true;
    m_animationDuration = durationSeconds;
    m_animationStartTime = std::chrono::steady_clock::now();
    m_animationStartLevel = getLevelPercent();
    
    // Isosurface媛 爰쇱졇?덉쑝硫?耳쒓린
    if (!m_showIsosurface) {
        m_showIsosurface = true;
    }
    
    // Auto value ?댁젣
    m_autoIsoValue = false;
    
    SPDLOG_INFO("Charge density animation started: duration={:.1f}s, startLevel={:.1f}%", 
                durationSeconds, m_animationStartLevel * 100.0f);
}

void ChargeDensityUI::stopAnimation() {
    if (m_isPlaying) {
        m_isPlaying = false;
        SPDLOG_INFO("Charge density animation stopped at level={:.1f}%", 
                    getLevelPercent() * 100.0f);
    }
}

void ChargeDensityUI::updateAnimation() {
    if (!m_isPlaying || !hasData()) return;
    
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - m_animationStartTime).count();
    
    // 吏꾪뻾瑜?怨꾩궛 (0.0 ~ 1.0)
    float progress = elapsed / m_animationDuration;
    
    if (progress >= 1.0f) {
        // ?좊땲硫붿씠???꾨즺
        progress = 1.0f;
        m_isPlaying = false;
        SPDLOG_INFO("Charge density animation completed");
    }
    
    // ?덈꺼 怨꾩궛: startLevel?먯꽌 ?쒖옉?섏뿬 100%源뚯? 吏꾪뻾
    // 留뚯빟 泥섏쓬遺???쒖옉?섍퀬 ?띠쑝硫?startLevel??0?쇰줈 ?ㅼ젙
    float targetLevel = m_animationStartLevel + (1.0f - m_animationStartLevel) * progress;
    
    // ?먮뒗 ??긽 0%?먯꽌 100%源뚯?:
    // float targetLevel = progress;
    
    applyIsoValueFromPercent(targetLevel);
}


} // namespace ui
} // namespace atoms
