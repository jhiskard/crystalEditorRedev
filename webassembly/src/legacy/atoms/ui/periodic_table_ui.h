// webassembly/src/atoms/ui/periodic_table_ui.h
#pragma once

#include <imgui.h>
#include <string>
#include <vector>

// Forward declarations
class AtomsTemplate;

namespace atoms {

namespace domain {
class ElementDatabase;
struct ElementInfo;
} // atoms
} // domain

namespace atoms {
namespace ui {

/**
 * @brief 주기율표 UI 렌더링 클래스
 * 
 * 주기율표 시각화 및 원소 선택 기능 제공
 * AtomsTemplate에서 UI 로직을 분리하여 관리
 */
class PeriodicTableUI {
public:
    /**
     * @brief 생성자
     * @param parent AtomsTemplate 인스턴스 포인터
     */
    explicit PeriodicTableUI(AtomsTemplate* parent);
    
    /**
     * @brief 소멸자
     */
    ~PeriodicTableUI() = default;
    
    /**
     * @brief 주기율표 UI 렌더링
     * 
     * ImGui::CollapsingHeader 내부에서 호출됨
     */
    void render();
    
    /**
     * @brief 선택된 원소의 symbol 반환
     * @return 선택된 원소 기호 (선택 없으면 빈 문자열)
     */
    std::string getSelectedElementSymbol() const;
    
    /**
     * @brief 선택 초기화
     */
    void clearSelection();
    
private:
    // ========================================================================
    // 렌더링 메서드
    // ========================================================================
    
    /**
     * @brief 카테고리 필터 ComboBox 렌더링
     */
    void renderCategoryFilter();
    
    /**
     * @brief 메인 주기율표 (1-7주기, 1-18족) 렌더링
     */
    void renderMainPeriodicTable();
    
    /**
     * @brief 란타넘족 (Period 8) 렌더링
     */
    void renderLanthanides();
    
    /**
     * @brief 악티늄족 (Period 9) 렌더링
     */
    void renderActinides();
    
    /**
     * @brief 선택된 원소 상세 정보 및 추가 버튼 렌더링
     */
    void renderSelectedElementDetails();
    
    /**
     * @brief 개별 원소 버튼 렌더링
     * @param element 원소 정보
     * @param buttonSize 버튼 크기
     */
    void renderElementButton(const atoms::domain::ElementInfo& element, 
                            float buttonSize);
    
    /**
     * @brief 원소 툴팁 렌더링 (마우스 호버 시)
     * @param element 원소 정보
     */
    void renderElementTooltip(const atoms::domain::ElementInfo& element);
    
    // ========================================================================
    // 유틸리티 메서드
    // ========================================================================
    
    /**
     * @brief 현재 카테고리 필터에 맞는 원소인지 확인
     * @param element 원소 정보
     * @return true: 표시해야 함, false: 숨김
     */
    bool shouldShowElement(const atoms::domain::ElementInfo& element) const;
    
    /**
     * @brief 호버 시 색상 계산 (밝게)
     * @param baseColor 기본 색상
     * @return 밝아진 색상
     */
    ImVec4 calculateHoveredColor(const ImVec4& baseColor) const;
    
    /**
     * @brief 클릭 시 색상 계산 (어둡게)
     * @param baseColor 기본 색상
     * @return 어두워진 색상
     */
    ImVec4 calculateActiveColor(const ImVec4& baseColor) const;
    
    // ========================================================================
    // 멤버 변수
    // ========================================================================
    
    AtomsTemplate* m_parent;                      // 부모 참조
    atoms::domain::ElementDatabase* m_elementDB;  // ElementDatabase 참조
    
    // UI 상태 변수
    std::string m_selectedElementSymbol;    // 선택된 원소 기호
    int m_category;                         // 원소 분류 카테고리 (0-10)
    float m_atomPosition[3];                // 원자 위치 입력
    bool m_useFractionalCoords;             // 분율 좌표 사용 여부
    
    // 레이아웃 상수
    static constexpr float BUTTON_SIZE = 40.0f;     // 기본 버튼 크기
    static constexpr float SPACING = 2.0f;          // 버튼 간격
    static constexpr float TOTAL_WIDTH = (BUTTON_SIZE + SPACING) * 18 + SPACING;  // 18족
};

} // namespace ui
} // namespace atoms
