// webassembly/src/atoms/ui/bravais_lattice_ui.cpp
#include "bravais_lattice_ui.h"
#include "../domain/crystal_structure.h"
#include "../domain/crystal_system.h"
#include "../domain/atom_manager.h"
#include "../domain/cell_manager.h"
#include "../../app.h"
#include "../../config/log_config.h"
#include "../atoms_template.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace atoms {
namespace ui {

namespace {
float calcScaledInputWidth() {
    const ImGuiStyle& style = ImGui::GetStyle();
    const float numericSampleWidth = ImGui::CalcTextSize("-1234.567").x;
    const float extraPadding = 8.0f * App::UiScale();
    const float minWidth = App::TextBaseWidth() * 6.5f;
    return std::max(minWidth, numericSampleWidth + style.FramePadding.x * 2.0f + extraPadding);
}
}

// ============================================================================
// 생성자
// ============================================================================

BravaisLatticeUI::BravaisLatticeUI(AtomsTemplate* parent)
    : m_parent(parent)
    , m_selectedLatticeType(-1)
    , m_showCubic(true)
    , m_showTetragonal(false)
    , m_showOrthorhombic(false)
    , m_showMonoclinic(false)
    , m_showTriclinic(false)
    , m_showRhombohedral(false)
    , m_showHexagonal(false)
    , m_requestOverlapWarningPopup(false)
    , m_pendingLatticeIndex(-1)
    , m_pendingMinDistance(0.0f)
    , m_pendingMinThreshold(0.0f) {
    
    // 파라미터 기본값 초기화
    initializeDefaultParameters();
    
    SPDLOG_DEBUG("BravaisLatticeUI initialized");
}

// ============================================================================
// 파라미터 기본값 초기화
// ============================================================================

void BravaisLatticeUI::initializeDefaultParameters() {
    // 모든 격자에 대한 기본 파라미터 설정
    for (int i = 0; i < 14; i++) {
        atoms::domain::BravaisLatticeType type = 
            static_cast<atoms::domain::BravaisLatticeType>(i);
        
        switch (type) {
            case atoms::domain::BravaisLatticeType::SIMPLE_CUBIC:
            case atoms::domain::BravaisLatticeType::BODY_CENTERED_CUBIC:
            case atoms::domain::BravaisLatticeType::FACE_CENTERED_CUBIC:
                // Cubic: a = b = c = 1.0
                m_latticeParams[i].a = 1.0f;
                m_latticeParams[i].b = 1.0f;
                m_latticeParams[i].c = 1.0f;
                m_latticeParams[i].alpha = 90.0f;
                m_latticeParams[i].beta = 90.0f;
                m_latticeParams[i].gamma = 90.0f;
                break;
                
            case atoms::domain::BravaisLatticeType::SIMPLE_TETRAGONAL:
            case atoms::domain::BravaisLatticeType::BODY_CENTERED_TETRAGONAL:
                // Tetragonal: a = b = 1.0, c = 1.5
                m_latticeParams[i].a = 1.0f;
                m_latticeParams[i].b = 1.0f;
                m_latticeParams[i].c = 1.5f;
                m_latticeParams[i].alpha = 90.0f;
                m_latticeParams[i].beta = 90.0f;
                m_latticeParams[i].gamma = 90.0f;
                break;
                
            case atoms::domain::BravaisLatticeType::SIMPLE_ORTHORHOMBIC:
            case atoms::domain::BravaisLatticeType::BODY_CENTERED_ORTHORHOMBIC:
            case atoms::domain::BravaisLatticeType::FACE_CENTERED_ORTHORHOMBIC:
            case atoms::domain::BravaisLatticeType::BASE_CENTERED_ORTHORHOMBIC:
                // Orthorhombic: a = 1.0, b = 1.2, c = 1.5
                m_latticeParams[i].a = 1.0f;
                m_latticeParams[i].b = 1.2f;
                m_latticeParams[i].c = 1.5f;
                m_latticeParams[i].alpha = 90.0f;
                m_latticeParams[i].beta = 90.0f;
                m_latticeParams[i].gamma = 90.0f;
                break;
                
            case atoms::domain::BravaisLatticeType::SIMPLE_MONOCLINIC:
            case atoms::domain::BravaisLatticeType::BASE_CENTERED_MONOCLINIC:
                // Monoclinic: a = 1.0, b = 1.2, c = 1.5, beta = 100.0
                m_latticeParams[i].a = 1.0f;
                m_latticeParams[i].b = 1.2f;
                m_latticeParams[i].c = 1.5f;
                m_latticeParams[i].alpha = 90.0f;
                m_latticeParams[i].beta = 100.0f;
                m_latticeParams[i].gamma = 90.0f;
                break;
                
            case atoms::domain::BravaisLatticeType::TRICLINIC:
                // Triclinic: 모든 값 다름
                m_latticeParams[i].a = 1.0f;
                m_latticeParams[i].b = 1.2f;
                m_latticeParams[i].c = 1.5f;
                m_latticeParams[i].alpha = 85.0f;
                m_latticeParams[i].beta = 95.0f;
                m_latticeParams[i].gamma = 100.0f;
                break;
                
            case atoms::domain::BravaisLatticeType::RHOMBOHEDRAL:
                // Rhombohedral: a = b = c = 1.0, alpha = beta = gamma = 80.0
                m_latticeParams[i].a = 1.0f;
                m_latticeParams[i].b = 1.0f;
                m_latticeParams[i].c = 1.0f;
                m_latticeParams[i].alpha = 80.0f;
                m_latticeParams[i].beta = 80.0f;
                m_latticeParams[i].gamma = 80.0f;
                break;
                
            case atoms::domain::BravaisLatticeType::HEXAGONAL:
                // Hexagonal: a = b = 1.0, c = 1.6, gamma = 120.0
                m_latticeParams[i].a = 1.0f;
                m_latticeParams[i].b = 1.0f;
                m_latticeParams[i].c = 1.6f;
                m_latticeParams[i].alpha = 90.0f;
                m_latticeParams[i].beta = 90.0f;
                m_latticeParams[i].gamma = 120.0f;
                break;
        }
    }
    
    SPDLOG_DEBUG("Initialized default parameters for 14 Bravais lattices");
}

// ============================================================================
// 메인 렌더링
// ============================================================================

void BravaisLatticeUI::render() {
    ImGui::Text("Bravais Lattice Templates");
    
    // 결정계 필터
    renderCategoryFilter();
    
    ImGui::Separator();
    
    // Bravais 격자 테이블
    renderLatticeTable();
    
    // 선택된 격자 정보
    if (m_selectedLatticeType >= 0) {
        ImGui::Separator();
        renderLatticeDescription();
    }

    renderOverlapWarningPopup();
}

// ============================================================================
// 결정계 필터
// ============================================================================

void BravaisLatticeUI::renderCategoryFilter() {
    ImGui::Text("Filter by Crystal System:");

    struct FilterItem {
        const char* label;
        bool* value;
    };

    const std::array<FilterItem, 7> filters = {{
        {"Cubic", &m_showCubic},
        {"Tetragonal", &m_showTetragonal},
        {"Orthorhombic", &m_showOrthorhombic},
        {"Monoclinic", &m_showMonoclinic},
        {"Triclinic", &m_showTriclinic},
        {"Rhombohedral", &m_showRhombohedral},
        {"Hexagonal", &m_showHexagonal}
    }};

    const ImGuiStyle& style = ImGui::GetStyle();
    const float windowVisibleX2 =
        ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

    for (size_t i = 0; i < filters.size(); ++i) {
        ImGui::Checkbox(filters[i].label, filters[i].value);

        if (i + 1 >= filters.size()) {
            continue;
        }

        const char* nextLabel = filters[i + 1].label;
        const float nextItemWidth =
            ImGui::GetFrameHeight() +
            style.ItemInnerSpacing.x +
            ImGui::CalcTextSize(nextLabel).x +
            style.FramePadding.x * 2.0f;
        const float nextItemX2 =
            ImGui::GetItemRectMax().x + style.ItemSpacing.x + nextItemWidth;

        if (nextItemX2 < windowVisibleX2) {
            ImGui::SameLine();
        }
    }
}

// ============================================================================
// Bravais 격자 테이블
// ============================================================================

void BravaisLatticeUI::renderLatticeTable() {
    // 사용 가능한 창 너비 계산
    const float availWidth = ImGui::GetContentRegionAvail().x;
    const ImGuiStyle& style = ImGui::GetStyle();

    float maxLatticeLabelWidth = 0.0f;
    for (int i = 0; i < 14; ++i) {
        maxLatticeLabelWidth = std::max(maxLatticeLabelWidth, ImGui::CalcTextSize(getLatticeName(i)).x);
    }

    const float minButtonWidth =
        maxLatticeLabelWidth + style.FramePadding.x * 2.0f + (App::UiScale() * 16.0f);
    const float preferredButtonWidth = availWidth * 0.28f;
    const float maxButtonWidth = std::max(minButtonWidth, availWidth * 0.50f);
    const float buttonWidth = std::clamp(std::max(minButtonWidth, preferredButtonWidth),
                                         minButtonWidth,
                                         maxButtonWidth);
    
    if (ImGui::BeginTable("BravaisLatticeTable", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Button", ImGuiTableColumnFlags_WidthFixed, buttonWidth);
        ImGui::TableSetupColumn("Parameters", ImGuiTableColumnFlags_WidthStretch);
        
        // 각 격자 유형별 행
        for (int i = 0; i < 14; i++) {
            if (!isLatticeVisible(i)) continue;  // 필터링 적용
            
            ImGui::TableNextRow();
            
            // 첫 번째 열: 버튼
            ImGui::TableNextColumn();
            if (ImGui::Button(getLatticeName(i), ImVec2(-FLT_MIN, 0))) {
                onLatticeSelected(i);
            }
            
            // 두 번째 열: 파라미터 입력
            ImGui::TableNextColumn();
            renderParameterInputs(i);
        }
        
        ImGui::EndTable();
    }
}

// ============================================================================
// 파라미터 입력 렌더링
// ============================================================================

void BravaisLatticeUI::renderParameterInputs(int latticeIndex) {
    atoms::domain::BravaisLatticeType type = 
        static_cast<atoms::domain::BravaisLatticeType>(latticeIndex);
    
    auto& params = m_latticeParams[latticeIndex];
    bool paramsChanged = false;
    const float inputWidth = calcScaledInputWidth();

    const ImGuiStyle& style = ImGui::GetStyle();
    const float availableWidth = std::max(ImGui::GetContentRegionAvail().x, inputWidth);
    float usedRowWidth = 0.0f;

    auto beginFlowField = [&](float fieldWidth) {
        if (usedRowWidth > 0.0f &&
            usedRowWidth + style.ItemSpacing.x + fieldWidth <= availableWidth) {
            ImGui::SameLine();
            usedRowWidth += style.ItemSpacing.x + fieldWidth;
            return;
        }
        usedRowWidth = fieldWidth;
    };

    auto renderFloatField = [&](const char* label,
                                const std::string& id,
                                float& value,
                                const char* format,
                                auto&& onChanged) {
        const float labelWidth = ImGui::CalcTextSize(label).x;
        const float fieldWidth = labelWidth + style.ItemInnerSpacing.x + inputWidth;
        beginFlowField(fieldWidth);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(inputWidth);
        if (ImGui::InputFloat(id.c_str(), &value, 0.0f, 0.0f, format)) {
            paramsChanged = true;
            onChanged();
        }
    };

    auto renderDescription = [&](const char* text) {
        ImGui::TextDisabled("%s", text);
    };

    // 각 격자 유형별 파라미터 설정 UI (좁은 폭에서 자동 줄바꿈)
    switch (type) {
        case atoms::domain::BravaisLatticeType::SIMPLE_CUBIC:
        case atoms::domain::BravaisLatticeType::BODY_CENTERED_CUBIC:
        case atoms::domain::BravaisLatticeType::FACE_CENTERED_CUBIC:
            renderFloatField("a:", "##a_" + std::to_string(latticeIndex), params.a, "%.3f", [&]() {
                params.b = params.c = params.a;
            });
            renderDescription("(a = b = c, all angles = 90°)");
            break;

        case atoms::domain::BravaisLatticeType::SIMPLE_TETRAGONAL:
        case atoms::domain::BravaisLatticeType::BODY_CENTERED_TETRAGONAL:
            renderFloatField("a:", "##a_" + std::to_string(latticeIndex), params.a, "%.3f", [&]() {
                params.b = params.a;
            });
            renderFloatField("c:", "##c_" + std::to_string(latticeIndex), params.c, "%.3f", []() {});
            renderDescription("(a = b != c, all angles = 90°)");
            break;

        case atoms::domain::BravaisLatticeType::SIMPLE_ORTHORHOMBIC:
        case atoms::domain::BravaisLatticeType::BODY_CENTERED_ORTHORHOMBIC:
        case atoms::domain::BravaisLatticeType::FACE_CENTERED_ORTHORHOMBIC:
        case atoms::domain::BravaisLatticeType::BASE_CENTERED_ORTHORHOMBIC:
            renderFloatField("a:", "##a_" + std::to_string(latticeIndex), params.a, "%.3f", []() {});
            renderFloatField("b:", "##b_" + std::to_string(latticeIndex), params.b, "%.3f", []() {});
            renderFloatField("c:", "##c_" + std::to_string(latticeIndex), params.c, "%.3f", []() {});
            renderDescription("(all angles = 90°)");
            break;

        case atoms::domain::BravaisLatticeType::SIMPLE_MONOCLINIC:
        case atoms::domain::BravaisLatticeType::BASE_CENTERED_MONOCLINIC:
            renderFloatField("a:", "##a_" + std::to_string(latticeIndex), params.a, "%.3f", []() {});
            renderFloatField("b:", "##b_" + std::to_string(latticeIndex), params.b, "%.3f", []() {});
            renderFloatField("c:", "##c_" + std::to_string(latticeIndex), params.c, "%.3f", []() {});
            renderFloatField("beta:", "##beta_" + std::to_string(latticeIndex), params.beta, "%.1f", []() {});
            renderDescription("(alpha = gamma = 90°)");
            break;

        case atoms::domain::BravaisLatticeType::TRICLINIC:
            renderFloatField("a:", "##a_" + std::to_string(latticeIndex), params.a, "%.3f", []() {});
            renderFloatField("b:", "##b_" + std::to_string(latticeIndex), params.b, "%.3f", []() {});
            renderFloatField("c:", "##c_" + std::to_string(latticeIndex), params.c, "%.3f", []() {});
            renderFloatField("alpha:", "##alpha_" + std::to_string(latticeIndex), params.alpha, "%.1f", []() {});
            renderFloatField("beta:", "##beta_" + std::to_string(latticeIndex), params.beta, "%.1f", []() {});
            renderFloatField("gamma:", "##gamma_" + std::to_string(latticeIndex), params.gamma, "%.1f", []() {});
            renderDescription("(all angles != 90°)");
            break;

        case atoms::domain::BravaisLatticeType::RHOMBOHEDRAL:
            renderFloatField("a:", "##a_" + std::to_string(latticeIndex), params.a, "%.3f", [&]() {
                params.b = params.c = params.a;
            });
            renderFloatField("alpha:", "##alpha_" + std::to_string(latticeIndex), params.alpha, "%.1f", [&]() {
                params.beta = params.gamma = params.alpha;
            });
            renderDescription("(a = b = c, alpha = beta = gamma)");
            break;

        case atoms::domain::BravaisLatticeType::HEXAGONAL:
            renderFloatField("a:", "##a_" + std::to_string(latticeIndex), params.a, "%.3f", [&]() {
                params.b = params.a;
                params.gamma = 120.0f;
            });
            renderFloatField("c:", "##c_" + std::to_string(latticeIndex), params.c, "%.3f", []() {});
            renderDescription("(a = b, alpha = beta = 90°, gamma = 120°)");
            break;
    }
}

// ============================================================================
// 선택된 격자 설명
// ============================================================================

void BravaisLatticeUI::renderLatticeDescription() {
    atoms::domain::BravaisLatticeType type = 
        static_cast<atoms::domain::BravaisLatticeType>(m_selectedLatticeType);
    
    ImGui::Text("Selected: %s", 
                atoms::domain::CrystalStructureGenerator::getLatticeName(type));
    
    // TreeNode로 상세 설명 접기/펼치기
    if (ImGui::TreeNode("Lattice Description")) {
        ImGui::TextWrapped("%s", 
            atoms::domain::CrystalStructureGenerator::getLatticeDescription(type));
        ImGui::TreePop();
    }
}

// ============================================================================
// 이벤트 핸들러
// ============================================================================

void BravaisLatticeUI::onLatticeSelected(int latticeIndex) {
    m_selectedLatticeType = latticeIndex;
    
    SPDLOG_INFO("Bravais lattice selected: {} (index={})", 
                getLatticeName(latticeIndex), latticeIndex);
    
    // AtomsTemplate에 격자 설정 요청
    if (m_parent) {
        atoms::domain::BravaisLatticeType type = 
            static_cast<atoms::domain::BravaisLatticeType>(latticeIndex);

        float minDistance = 0.0f;
        float minThreshold = 0.0f;
        std::string symbolA;
        std::string symbolB;
        if (checkPredictedOverlap(
                type, m_latticeParams[latticeIndex],
                minDistance, minThreshold, symbolA, symbolB)) {
            m_pendingLatticeIndex = latticeIndex;
            m_pendingMinDistance = minDistance;
            m_pendingMinThreshold = minThreshold;
            m_pendingSymbolA = symbolA;
            m_pendingSymbolB = symbolB;
            m_requestOverlapWarningPopup = true;
            return;
        }

        m_parent->setBravaisLattice(type, m_latticeParams[latticeIndex], true);
    }
}

void BravaisLatticeUI::renderOverlapWarningPopup() {
    if (m_pendingLatticeIndex < 0) {
        return;
    }

    const char* popupTitle = "Potential Atom Overlap";
    if (m_requestOverlapWarningPopup) {
        ImGui::OpenPopup(popupTitle);
        m_requestOverlapWarningPopup = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(popupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("The selected lattice may place existing atoms too close together.");
        ImGui::Text("Selected lattice: %s", getLatticeName(m_pendingLatticeIndex));
        if (!m_pendingSymbolA.empty() && !m_pendingSymbolB.empty()) {
            ImGui::Text("Closest pair: %s - %s", m_pendingSymbolA.c_str(), m_pendingSymbolB.c_str());
        }
        ImGui::Text("Minimum predicted distance: %.3f", m_pendingMinDistance);
        ImGui::Text("Warning threshold: %.3f", m_pendingMinThreshold);

        ImGui::Separator();

        if (ImGui::Button("Clear Existing Atoms")) {
            if (m_parent) {
                auto type = static_cast<atoms::domain::BravaisLatticeType>(m_pendingLatticeIndex);
                m_parent->setBravaisLattice(type, m_latticeParams[m_pendingLatticeIndex], false);
            }
            m_pendingLatticeIndex = -1;
            m_pendingSymbolA.clear();
            m_pendingSymbolB.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Keep Existing Atoms")) {
            if (m_parent) {
                auto type = static_cast<atoms::domain::BravaisLatticeType>(m_pendingLatticeIndex);
                m_parent->setBravaisLattice(type, m_latticeParams[m_pendingLatticeIndex], true);
            }
            m_pendingLatticeIndex = -1;
            m_pendingSymbolA.clear();
            m_pendingSymbolB.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

bool BravaisLatticeUI::checkPredictedOverlap(
    atoms::domain::BravaisLatticeType type,
    const atoms::domain::BravaisParameters& params,
    float& outMinDistance,
    float& outMinThreshold,
    std::string& outSymbolA,
    std::string& outSymbolB
) const {
    const auto& atoms = atoms::domain::createdAtoms;
    if (atoms.size() < 2) {
        return false;
    }

    constexpr float kOverlapThresholdScale = 0.8f;

    float cellMatrix[3][3];
    atoms::domain::CrystalStructureGenerator::generateLatticeVectors(type, params, cellMatrix);

    std::vector<std::array<float, 3>> positions;
    std::vector<float> radii;
    std::vector<std::string> symbols;
    positions.reserve(atoms.size());
    radii.reserve(atoms.size());
    symbols.reserve(atoms.size());

    for (const auto& atom : atoms) {
        std::array<float, 3> cartPos = {0.0f, 0.0f, 0.0f};
        atoms::domain::fractionalToCartesian(atom.fracPosition, cartPos.data(), cellMatrix);
        positions.push_back(cartPos);
        radii.push_back(atom.modified ? atom.tempRadius : atom.radius);
        symbols.push_back(atom.modified ? atom.tempSymbol : atom.symbol);
    }

    bool overlapFound = false;
    float worstRatio = 1.0f;

    for (size_t i = 0; i < positions.size(); ++i) {
        for (size_t j = i + 1; j < positions.size(); ++j) {
            double dx = positions[j][0] - positions[i][0];
            double dy = positions[j][1] - positions[i][1];
            double dz = positions[j][2] - positions[i][2];
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            double threshold = (radii[i] + radii[j]) * kOverlapThresholdScale;
            if (threshold <= 0.0) {
                continue;
            }

            double ratio = distance / threshold;
            if (ratio < worstRatio) {
                worstRatio = static_cast<float>(ratio);
                outMinDistance = static_cast<float>(distance);
                outMinThreshold = static_cast<float>(threshold);
                outSymbolA = symbols[i];
                outSymbolB = symbols[j];
            }

            if (ratio < 1.0f) {
                overlapFound = true;
            }
        }
    }

    return overlapFound;
}

// ============================================================================
// 유틸리티 메서드
// ============================================================================

bool BravaisLatticeUI::isLatticeVisible(int latticeIndex) const {
    atoms::domain::BravaisLatticeType type = 
        static_cast<atoms::domain::BravaisLatticeType>(latticeIndex);
    
    atoms::domain::CrystalSystem system = 
        atoms::domain::CrystalSystemMapper::getCrystalSystem(type);
    
    switch (system) {
        case atoms::domain::CrystalSystem::CUBIC:
            return m_showCubic;
        case atoms::domain::CrystalSystem::TETRAGONAL:
            return m_showTetragonal;
        case atoms::domain::CrystalSystem::ORTHORHOMBIC:
            return m_showOrthorhombic;
        case atoms::domain::CrystalSystem::MONOCLINIC:
            return m_showMonoclinic;
        case atoms::domain::CrystalSystem::TRICLINIC:
            return m_showTriclinic;
        case atoms::domain::CrystalSystem::RHOMBOHEDRAL:
            return m_showRhombohedral;
        case atoms::domain::CrystalSystem::HEXAGONAL:
            return m_showHexagonal;
        default:
            return false;
    }
}

const char* BravaisLatticeUI::getLatticeName(int latticeIndex) const {
    atoms::domain::BravaisLatticeType type = 
        static_cast<atoms::domain::BravaisLatticeType>(latticeIndex);
    return atoms::domain::CrystalStructureGenerator::getLatticeName(type);
}

void BravaisLatticeUI::clearSelection() {
    m_selectedLatticeType = -1;
    SPDLOG_DEBUG("Bravais lattice selection cleared");
}

} // namespace ui
} // namespace atoms
