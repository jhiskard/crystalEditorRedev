#include "bz_plot_ui.h"
#include "../atoms_template.h"
#include "../domain/special_points.h"
#include "../domain/cell_manager.h"
#include "../../app.h"
#include "../../config/log_config.h"

#include <algorithm>
#include <cstdio>

namespace atoms {
namespace ui {

BZPlotUI::BZPlotUI(AtomsTemplate* parent)
    : m_parent(parent)
{
    SPDLOG_DEBUG("BZPlotUI initialized");
}

void BZPlotUI::SetShowingBZ(bool showing) {
    m_showingBZ = showing;
    if (!m_showingBZ) {
        m_lastErrorMessage.clear();
    }
}

void BZPlotUI::render() {
    if (!m_parent) {
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
                           "BZPlotUI: AtomsTemplate is not available.");
        return;
    }

    renderBandpathConfig();
    ImGui::Spacing();
    renderOptions();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    renderToggleAndClearButtons();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    renderStatus();
    ImGui::Spacing();
    renderSpecialPointsTable();
    // renderBZplot();
}

void BZPlotUI::renderBandpathConfig() {
    ImGui::Text("Bandpath Configuration:");
    ImGui::Spacing();

    // Path 입력
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Path:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(250.0f);
    ImGui::InputText("##path", m_pathInput, IM_ARRAYSIZE(m_pathInput));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Enter k-point path using special point symbols\n"
            "Examples:\n"
            "  - All (automatic path selection based on lattice)\n"
            "  - GXMGRX (cubic)\n"
            "  - GXMGRX,MR (with branch)\n"
            "  - GKLUWLK (hexagonal)"
        );
    }

    // Npoints 입력
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Npoints:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(240.0f);
    if (ImGui::InputInt("##npoints", &m_npointsInput)) {
        if (m_npointsInput < 10)  m_npointsInput = 10;
        if (m_npointsInput > 500) m_npointsInput = 500;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Number of interpolation points along each path segment.\n"
            "Typical range: 10 - 200."
        );
    }
}

void BZPlotUI::renderOptions() {
    ImGui::Text("Options:");
    ImGui::Spacing();

    // 🔹 토글 이전 상태 저장
    bool prevShowVectors = m_showVectors;
    bool prevShowLabels  = m_showLabels;

    // 🔹 벡터 표시 토글
    ImGui::Checkbox("Show reciprocal vectors", &m_showVectors);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Toggle visibility of reciprocal lattice vectors.");
    }

    // 🔹 라벨 표시 토글
    ImGui::Checkbox("Show special point labels", &m_showLabels);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Toggle labels for special k-points.");
    }

    // 🔹 둘 중 하나라도 값이 바뀌었는지 확인
    bool vectorsChanged = (prevShowVectors != m_showVectors);
    bool labelsChanged  = (prevShowLabels  != m_showLabels);
    bool optionsChanged = vectorsChanged || labelsChanged;

    // 이미 BZ Plot 모드일 때만 즉시 업데이트
    if (optionsChanged && m_showingBZ && m_parent) {
        m_lastErrorMessage.clear();

        std::string pathStr(m_pathInput);
        int npoints = m_npointsInput;

        // ⚠️ 중요: 현재 옵션(m_showVectors / m_showLabels)을 그대로 넘겨서
        // 전체 BZ Plot을 다시 생성하도록 한다.
        bool success = m_parent->enterBZPlotMode(
            pathStr,
            npoints,
            m_showVectors,   // ← 벡터 표시 여부
            m_showLabels,    // ← 라벨 표시 여부 (여기가 핵심)
            m_lastErrorMessage
        );

        // 다시 그리기 성공 여부에 따라 로그만 참고용으로 사용
        if (!success && !m_lastErrorMessage.empty()) {
            SPDLOG_ERROR("Failed to update BZ options: {}", m_lastErrorMessage);
        }
    }
}

void BZPlotUI::renderToggleAndClearButtons() {
    const char* buttonText = m_showingBZ ? "Show Crystal" : "Show BZ Plot";
    const ImGuiStyle& style = ImGui::GetStyle();
    const float maxLabelWidth = std::max(
        std::max(ImGui::CalcTextSize("Show BZ Plot").x, ImGui::CalcTextSize("Show Crystal").x),
        ImGui::CalcTextSize("Clear BZ").x);
    const float minButtonWidth =
        maxLabelWidth + style.FramePadding.x * 2.0f + (App::UiScale() * 12.0f);
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const bool sameLineButtons =
        availableWidth >= (minButtonWidth * 2.0f + style.ItemSpacing.x);
    const float buttonWidth = sameLineButtons
        ? std::max(minButtonWidth, (availableWidth - style.ItemSpacing.x) * 0.5f)
        : availableWidth;
    const ImVec2 buttonSize(buttonWidth, 0.0f);

    // Show BZ Plot / Show Crystal 토글 버튼
    if (ImGui::Button(buttonText, buttonSize)) {
        m_lastErrorMessage.clear();

        if (!m_showingBZ) {
            // BZ Plot 모드 진입
            std::string pathStr(m_pathInput);
            int npoints = m_npointsInput;

            bool success = false;
            if (m_parent) {
                success = m_parent->enterBZPlotMode(
                    pathStr,
                    npoints,
                    m_showVectors,
                    m_showLabels,
                    m_lastErrorMessage
                );
            }

            if (success) {
                m_showingBZ = true;
            }
        } else {
            // Crystal 모드로 복귀
            if (m_parent) {
                m_parent->exitBZPlotMode();
            }
            m_showingBZ = false;
        }
    }

    if (sameLineButtons) {
        ImGui::SameLine();
    }

    // Clear BZ 버튼
    if (ImGui::Button("Clear BZ", buttonSize)) {
        m_lastErrorMessage.clear();
        if (m_parent) {
            // BZ Plot을 제거하고 crystal 모드로 복귀
            m_parent->exitBZPlotMode();
        }
        m_showingBZ = false;
    }
}

void BZPlotUI::renderStatus() {
    ImGui::Spacing();

    if (m_showingBZ) {
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f),
                           "● BZ Plot Mode");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                           "(vectors: %s, labels: %s)",
                           m_showVectors ? "ON" : "OFF",
                           m_showLabels ? "ON" : "OFF");
    } else {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                           "● Crystal Mode");
    }

    if (!m_lastErrorMessage.empty()) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextWrapped("Error: %s", m_lastErrorMessage.c_str());
        ImGui::PopStyleColor();
    }
}

void BZPlotUI::renderSpecialPointsTable() {
    // 격자 타입 감지 및 특수점 데이터 조회
    std::string latticeType =
        atoms::domain::SpecialPointsDatabase::detectLatticeType(cellInfo.matrix);
    auto special_points =
        atoms::domain::SpecialPointsDatabase::getSpecialPoints(latticeType);

    ImGui::Spacing();
    ImGui::Text("Special k-points for lattice: %s", latticeType.c_str());
    ImGui::Spacing();

    if (special_points.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f),
                           "No special points to display");
        return;
    }

    ImGui::Text("Total points: %zu", special_points.size());
    ImGui::Separator();
    ImGui::Spacing();

    ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingFixedFit;

    if (ImGui::BeginTable("SpecialPointsTable", 4, tableFlags)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("a", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("b", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("c", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableHeadersRow();

        auto labelColor = ImVec4(0.8f, 0.9f, 1.0f, 1.0f);

        // for (const auto& [label, coords] : special_points)
        for (const auto& kv : special_points) {
            const std::string& label = kv.first;
            const auto& coords = kv.second;

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(labelColor, "%s", label.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.6f", coords[0]);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.6f", coords[1]);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.6f", coords[2]);
        }

        ImGui::EndTable();
    }
}

/*
// 기존 static 지역 변수들을 멤버로 이동
char  m_pathInput[128] = "All";
int   m_npointsInput   = 50;
bool  m_showVectors    = true;
bool  m_showLabels     = true;
bool  m_showingBZ      = false;
*/
void BZPlotUI::renderBZplot() {
    // 입력 필드 상태 (static으로 유지)
    static char pathInput[128] = "All";  // ⭐ 기본값 "All"로 변경
    static int npointsInput = 50;
    static bool showVectors = true;
    static bool showLabels = true;
    static bool showingBZ = false;  // BZ 표시 상태
    static std::string lastErrorMessage;
    
    // ========================================================================
    // 1. 입력 UI
    // ========================================================================
    
    ImGui::Text("Bandpath Configuration:");
    ImGui::Spacing();
    
    // Path 입력
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Path:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##path", pathInput, IM_ARRAYSIZE(pathInput));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Enter k-point path using special point symbols\n"
            "Examples:\n"
            "  - All (automatic path selection based on lattice)\n"
            "  - GXMGRX (cubic)\n"
            "  - GXMGRX,MR (with branch)\n"
            "  - GKLUWLK (hexagonal)"
        );
    }
    
    // Npoints 입력
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Npoints:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::InputInt("##npoints", &npointsInput);
    if (npointsInput < 10) npointsInput = 10;
    if (npointsInput > 500) npointsInput = 500;
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Number of interpolation points along the path (10-500)");
    }
    
    ImGui::Spacing();
    
    // 옵션 체크박스
    ImGui::Checkbox("Show reciprocal vectors", &showVectors);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Display the reciprocal lattice vectors (b1, b2, b3)");
    }
    
    ImGui::Checkbox("Show special point labels", &showLabels);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Display labels at high-symmetry points (Γ, X, M, etc.)");
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // ========================================================================
    // 2. 토글 버튼 (Show BZ Plot ↔ Show Crystal)
    // ========================================================================
    
    const char* buttonText = showingBZ ? "Show Crystal" : "Show BZ Plot";
    const ImGuiStyle& style = ImGui::GetStyle();
    const float maxLabelWidth = std::max(
        std::max(ImGui::CalcTextSize("Show BZ Plot").x, ImGui::CalcTextSize("Show Crystal").x),
        ImGui::CalcTextSize("Clear BZ").x);
    const float minButtonWidth =
        maxLabelWidth + style.FramePadding.x * 2.0f + (App::UiScale() * 12.0f);
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const bool sameLineButtons =
        availableWidth >= (minButtonWidth * 2.0f + style.ItemSpacing.x);
    const float buttonWidth = sameLineButtons
        ? std::max(minButtonWidth, (availableWidth - style.ItemSpacing.x) * 0.5f)
        : availableWidth;
    const ImVec2 buttonSize(buttonWidth, 0.0f);
    
    if (ImGui::Button(buttonText, buttonSize)) {
        if (!showingBZ) {
            // ============================================================
            // BZ Plot 표시 모드로 전환
            // ============================================================
            SPDLOG_INFO("=== Switching to BZ Plot mode ===");
            SPDLOG_INFO("Path: '{}', npoints: {}, vectors: {}, labels: {}", 
                       pathInput, npointsInput, showVectors, showLabels);
            
            lastErrorMessage.clear();
            
            try {
                // 1. 역격자 행렬 준비
                double cell[3][3];
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        cell[i][j] = cellInfo.matrix[i][j];
                    }
                }
                double icell[3][3];
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        // icell[i][j] = cellInfo.invmatrix[i][j];
                        // cellInfo.invmatrix is TRANSPOSED reciprocal lattice vectors...
                        icell[i][j] = cellInfo.invmatrix[j][i];
                    }
                }
                
                SPDLOG_DEBUG("Inverse cell matrix:");
                SPDLOG_DEBUG("  [{}, {}, {}]", icell[0][0], icell[0][1], icell[0][2]);
                SPDLOG_DEBUG("  [{}, {}, {}]", icell[1][0], icell[1][1], icell[1][2]);
                SPDLOG_DEBUG("  [{}, {}, {}]", icell[2][0], icell[2][1], icell[2][2]);
                
                // 2. BZ vertices 계산
                SPDLOG_INFO("Calculating BZ vertices...");
                auto bzData = atoms::domain::BZCalculator::calculateBZVertices(icell);
                
                if (!bzData.success) {
                    lastErrorMessage = "BZ calculation failed: " + bzData.errorMessage;
                    SPDLOG_ERROR("{}", lastErrorMessage);
                    return;
                }
                
                SPDLOG_INFO("BZ calculation successful. Facets: {}, Vertices in first facet: {}", 
                           bzData.facets.size(), 
                           bzData.facets.empty() ? 0 : bzData.facets[0].vertices.size());
                
                // 3. 기존 객체 숨기기 (원자, 결합, Unit Cell)
                SPDLOG_INFO("Hiding crystal structure...");
                
                if (m_parent->m_vtkRenderer) {
                    m_parent->m_vtkRenderer->setAllAtomGroupsVisible(false);
                    m_parent->m_vtkRenderer->setAllBondGroupsVisible(false);
                    
                    if (m_parent->isCellVisible()) {
                        m_parent->m_vtkRenderer->setUnitCellVisible(false);
                    }
                }
                
                // ⭐ 4. "All" 키워드 처리 - 자동 경로 선택
                std::string pathStr(pathInput);
                
                if (pathStr == "All" || pathStr == "all" || pathStr == "ALL") {
                    SPDLOG_INFO("'All' keyword detected - determining lattice type automatically");

                    // ⭐ SpecialPointsDatabase::detectLatticeType() 호출
                    std::string latticeType = atoms::domain::SpecialPointsDatabase::detectLatticeType(cellInfo.matrix);
                    
                    SPDLOG_INFO("Detected lattice type: {}", latticeType);
                    
                    // SpecialPointsDatabase에서 기본 경로 가져오기
                    pathStr = atoms::domain::SpecialPointsDatabase::getDefaultPath(latticeType);
                    
                    SPDLOG_INFO("Auto-selected path: '{}'", pathStr);
                }

                // 5. 완전한 BZ Plot 생성 (모든 요소 포함)
                if (m_parent->m_vtkRenderer) {
                    SPDLOG_INFO("Creating complete BZ Plot with path: '{}'", pathStr);
                    
                    m_parent->m_vtkRenderer->createCompleteBZPlot(
                        bzData,
                        cell, 
                        icell, 
                        showVectors,    // 역격자 벡터 표시
                        showLabels,     // 특수점 라벨 표시
                        pathStr,        // 밴드 경로
                        npointsInput    // 보간 점 개수
                    );
                    
                    SPDLOG_INFO("Complete BZ Plot created successfully");
                    
                    showingBZ = true;
                } 
                
                else {
                    lastErrorMessage = "VTKRenderer not initialized";
                    SPDLOG_ERROR("{}", lastErrorMessage);
                }
                
            } catch (const std::exception& e) {
                lastErrorMessage = std::string("Exception: ") + e.what();
                SPDLOG_ERROR("BZ Plot rendering failed: {}", lastErrorMessage);
            }
            
        } else {
            // ============================================================
            // Crystal 표시 모드로 전환
            // ============================================================
            SPDLOG_INFO("=== Switching to Crystal mode ===");
            
            // 1. BZ Plot 숨기기
            if (m_parent->m_vtkRenderer) {
                SPDLOG_INFO("Hiding BZ Plot...");
                m_parent->m_vtkRenderer->setBZPlotVisible(false);
            }
            
            // 2. 기존 객체 다시 표시 (원자, 결합, Unit Cell)
            SPDLOG_INFO("Showing crystal structure...");
            
            if (m_parent->m_vtkRenderer) {
                m_parent->m_vtkRenderer->setAllAtomGroupsVisible(true);
                m_parent->m_vtkRenderer->setAllBondGroupsVisible(true);
                
                if (m_parent->isCellVisible()) {
                    m_parent->m_vtkRenderer->setUnitCellVisible(true);
                }
            }
            
            showingBZ = false;
        }
        
        // 렌더링 업데이트
        VtkViewer::Instance().RequestRender();
    }
    
    if (sameLineButtons) {
        ImGui::SameLine();
    }
    
    // Clear 버튼
    if (ImGui::Button("Clear BZ", buttonSize)) {
        if (m_parent->m_vtkRenderer) {
            m_parent->m_vtkRenderer->clearBZPlot();
            
            // Crystal 모드로 돌아가기
            if (showingBZ) {
                if (m_parent->m_vtkRenderer) {
                    m_parent->m_vtkRenderer->setAllAtomGroupsVisible(true);
                    m_parent->m_vtkRenderer->setAllBondGroupsVisible(true);
                    
                    if (m_parent->isCellVisible()) {
                        m_parent->m_vtkRenderer->setUnitCellVisible(true);
                    }
                }
                showingBZ = false;
            }
            
            lastErrorMessage.clear();
            SPDLOG_INFO("BZ Plot cleared");
            VtkViewer::Instance().RequestRender();
        }
    }
    
    // ========================================================================
    // 3. 상태 표시
    // ========================================================================
    
    ImGui::Spacing();
    
    // 현재 모드 표시
    if (showingBZ) {
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), 
                         "● BZ Plot Mode");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), 
                         "(vectors: %s, labels: %s)", 
                         showVectors ? "ON" : "OFF",
                         showLabels ? "ON" : "OFF");
    } else {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 
                         "● Crystal Mode");
    }
    
    // 에러 메시지 표시
    if (!lastErrorMessage.empty()) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextWrapped("Error: %s", lastErrorMessage.c_str());
        ImGui::PopStyleColor();
    }
    
    // ========================================================================
    // 4. 추가 정보
    // ========================================================================
    
    ImGui::Spacing();
    
    // 격자 타입
    std::string latticeType;
    latticeType = atoms::domain::SpecialPointsDatabase::detectLatticeType(cellInfo.matrix);

    // special points 가져오기
    auto special_points = atoms::domain::SpecialPointsDatabase::getSpecialPoints(latticeType);

    // special points 출력
    // atoms::domain::SpecialPointsDatabase::printSpecialPointsTable(special_points);
    
    if (special_points.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "No special points to display");
    }
    
    else {
        ImGui::Text("Total points: %zu", special_points.size());
        ImGui::Separator();
        ImGui::Spacing();
        
        // ImGui 테이블 생성
        ImGuiTableFlags tableFlags = 
            ImGuiTableFlags_Borders | 
            ImGuiTableFlags_RowBg | 
            ImGuiTableFlags_SizingFixedFit;
        
        ImGui::BeginTable("SpecialPointsTable", 4, tableFlags);
        // 헤더 설정
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("a", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("b", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("c", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableHeadersRow();
        
        // 데이터 행
        for (const auto& [label, coords] : special_points) {
            ImGui::TableNextRow();
            
            // Label 열
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", label.c_str());
            
            // a 열
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.6f", coords[0]);
            
            // b 열
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.6f", coords[1]);
            
            // c 열
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.6f", coords[2]);
        }
        ImGui::EndTable();
    }
}

} // namespace ui
} // namespace atoms
