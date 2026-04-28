// webassembly/src/atoms/ui/bravais_lattice_ui.h
#pragma once

#include "../domain/crystal_structure.h"
#include "../domain/crystal_system.h"
// #include <imgui.h>
#include <array>
#include <string>

// Forward declaration
class AtomsTemplate;

namespace atoms {
namespace ui {

/**
 * @brief Bravais 격자 템플릿 UI 렌더링 클래스
 * 
 * 14가지 Bravais 격자 선택 및 파라미터 입력 기능 제공
 * AtomsTemplate에서 UI 로직을 분리하여 관리
 */
class BravaisLatticeUI {
public:
    /**
     * @brief 생성자
     * @param parent AtomsTemplate 인스턴스 포인터
     */
    explicit BravaisLatticeUI(AtomsTemplate* parent);
    
    /**
     * @brief 소멸자
     */
    ~BravaisLatticeUI() = default;
    
    /**
     * @brief Bravais 격자 템플릿 UI 렌더링
     * 
     * ImGui::CollapsingHeader 내부에서 호출됨
     */
    void render();
    
    /**
     * @brief 선택된 격자 유형 반환
     * @return 선택된 격자 인덱스 (-1: 선택 없음)
     */
    int getSelectedLatticeType() const { return m_selectedLatticeType; }
    
    /**
     * @brief 특정 격자의 파라미터 반환
     * @param index 격자 인덱스 (0-13)
     * @return 격자 파라미터
     */
    const atoms::domain::BravaisParameters& getLatticeParams(int index) const {
        return m_latticeParams[index];
    }
    
    /**
     * @brief 선택 초기화
     */
    void clearSelection();
    
private:
    // ========================================================================
    // 렌더링 메서드
    // ========================================================================
    
    /**
     * @brief 결정계 필터 렌더링
     */
    void renderCategoryFilter();
    
    /**
     * @brief Bravais 격자 테이블 렌더링
     * 
     * 버튼 + 파라미터 입력 필드를 테이블 형태로 표시
     */
    void renderLatticeTable();
    
    /**
     * @brief 특정 격자의 파라미터 입력 UI 렌더링
     * @param latticeIndex 격자 인덱스 (0-13)
     */
    void renderParameterInputs(int latticeIndex);
    
    /**
     * @brief 선택된 격자의 상세 설명 렌더링
     */
    void renderLatticeDescription();
    
    // ========================================================================
    // 이벤트 핸들러
    // ========================================================================
    
    /**
     * @brief 격자 선택 시 콜백
     * @param latticeIndex 선택된 격자 인덱스
     */
    void onLatticeSelected(int latticeIndex);

    /**
     * @brief 격자 변경 시 원자 간 거리 경고 팝업 렌더링
     */
    void renderOverlapWarningPopup();

    /**
     * @brief 선택된 격자 적용 시 원자 간 거리가 과도하게 가까운지 검사
     * @param type 격자 타입
     * @param params 격자 파라미터
     * @param outMinDistance 가장 가까운 원자 간 거리
     * @param outMinThreshold 경고 임계값
     * @param outSymbolA 가장 가까운 원자 A의 원소 기호
     * @param outSymbolB 가장 가까운 원자 B의 원소 기호
     * @return true면 경고 필요
     */
    bool checkPredictedOverlap(
        atoms::domain::BravaisLatticeType type,
        const atoms::domain::BravaisParameters& params,
        float& outMinDistance,
        float& outMinThreshold,
        std::string& outSymbolA,
        std::string& outSymbolB
    ) const;
    
    // ========================================================================
    // 유틸리티 메서드
    // ========================================================================
    
    /**
     * @brief 격자가 현재 필터에 의해 표시되는지 확인
     * @param latticeIndex 격자 인덱스
     * @return true: 표시, false: 숨김
     */
    bool isLatticeVisible(int latticeIndex) const;
    
    /**
     * @brief 파라미터 기본값 초기화
     */
    void initializeDefaultParameters();
    
    /**
     * @brief 격자 유형 이름 반환
     * @param latticeIndex 격자 인덱스
     * @return 격자 이름
     */
    const char* getLatticeName(int latticeIndex) const;
    
    // ========================================================================
    // 멤버 변수
    // ========================================================================
    
    AtomsTemplate* m_parent;  // 부모 AtomsTemplate 참조
    
    // UI State
    int m_selectedLatticeType;  // 현재 선택된 격자 (-1: 선택 없음)
    std::array<atoms::domain::BravaisParameters, 14> m_latticeParams;  // 각 격자의 파라미터
    
    // Filter State (결정계별 필터)
    bool m_showCubic;
    bool m_showTetragonal;
    bool m_showOrthorhombic;
    bool m_showMonoclinic;
    bool m_showTriclinic;
    bool m_showRhombohedral;
    bool m_showHexagonal;

    // Overlap warning popup state
    bool m_requestOverlapWarningPopup;
    int m_pendingLatticeIndex;
    float m_pendingMinDistance;
    float m_pendingMinThreshold;
    std::string m_pendingSymbolA;
    std::string m_pendingSymbolB;
    
};

} // namespace ui
} // namespace atoms
