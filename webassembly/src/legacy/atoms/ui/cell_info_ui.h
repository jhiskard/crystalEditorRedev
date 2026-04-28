// webassembly/src/atoms/ui/cell_info_ui.h
#pragma once

#include <imgui.h>

class AtomsTemplate;

namespace atoms {
namespace ui {

/**
 * @brief "Cell Information" 섹션 전용 UI 클래스
 *
 * - cellInfo.matrix / invmatrix 테이블 표시 및 편집
 * - Edit mode 토글 및 상태 표시
 * - Edit mode 종료 시 AtomsTemplate를 통해
 *   Unit Cell, 주변 원자, 분수 좌표 재계산을 트리거
 */
class CellInfoUI {
public:
    explicit CellInfoUI(AtomsTemplate* parent);

    /// "Cell Information" 섹션 전체 렌더링
    void render();

private:
    AtomsTemplate* m_parent;   ///< 도메인/상태 계층 접근용

    // 기존 static 지역 변수들을 멤버로 이전
    bool m_editMode     = false;
    bool m_prevEditMode = false;

    // 필요하다면 내부를 더 나누기 위한 헬퍼들
    // void renderEditModeToggle();
    // void renderCellMatrixTable();
    // void renderEditModeHint();
    
    /// Edit mode가 true→false 로 바뀔 때 호출되는 핵심 처리
    void applyCellChangesOnEditEnd();
};

} // namespace ui
} // namespace atoms
