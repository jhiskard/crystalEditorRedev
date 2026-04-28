// webassembly/src/atoms/ui/bond_ui.h
#pragma once

#include <imgui.h>

class AtomsTemplate;

namespace atoms {
namespace ui {

/**
 * @brief Bond 관리 전용 UI 클래스
 *
 * - 결합 생성/삭제, 스타일(두께/투명도), 거리 스케일링 등
 * - 실제 데이터 변경은 AtomsTemplate 및 도메인 레이어에 위임
 */
class BondUI {
public:
    explicit BondUI(AtomsTemplate* parent);

    /// "Atom & Bond Tools" 섹션 중 Bond 관련 UI 전체를 렌더링
    void render();

private:
    AtomsTemplate* m_parent;  ///< 도메인/상태에 접근하기 위한 상위 객체 포인터

    /// 결합 생성/삭제 등 고수준 Bond 작업 섹션
    void renderBondOperationsSection();

    /// 결합 두께/투명도 등 스타일 섹션
    void renderBondStyleSection();

    /// 거리 스케일링/허용오차 등 Bond 생성 파라미터 섹션
    void renderBondDistanceSection();

    /// 결합 업데이트 성능 통계 섹션
    void renderPerformanceSection();
};

} // namespace ui
} // namespace atoms
