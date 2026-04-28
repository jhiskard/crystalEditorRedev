// webassembly/src/atoms/ui/atom_editor_ui.h
#pragma once

#include <imgui.h>
#include <string>
#include <vector>

// Forward declarations
class AtomsTemplate;

namespace atoms {
// class AtomsTemplate;

namespace ui {

/**
 * @brief "Created Atoms" 편집 섹션 전용 UI 클래스
 *
 * - 원자 목록/편집 테이블(UI)을 담당
 * - 실제 데이터 변경/배치는 AtomsTemplate 및 도메인 레이어에 위임
 */
class AtomEditorUI {
public:
    explicit AtomEditorUI(AtomsTemplate* parent);

    /// "Created Atoms" 섹션 전체를 렌더링
    void render();

private:
    AtomsTemplate* m_parent;  ///< 도메인/상태에 접근하기 위한 상위 객체 포인터

    // UI 상태 (기존 static 지역변수 대체)
    bool m_editMode = false;
    bool m_hasChanges = false;
    bool m_useFractionalCoords = false;
    bool m_boundaryAtomsEnabled = false;

    /// 원자 테이블을 렌더링하고, 변경사항이 발생했는지 여부를 반환
    bool renderAtomTable(bool editMode, bool useFractionalCoords);

    /// 선택/이동 관련 패널 UI를 렌더링
    void renderSelectionPanel(bool editMode, bool useFractionalCoords, bool* hasChanges);
};

} // namespace ui
} // namespace atoms
