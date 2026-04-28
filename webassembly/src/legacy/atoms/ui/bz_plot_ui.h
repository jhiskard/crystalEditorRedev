#pragma once

#include <imgui.h>
#include <string>

class AtomsTemplate;

namespace atoms {
namespace ui {

/**
 * @brief "Brillouin Zone Plot" 섹션 전용 UI 클래스
 *
 * - 밴드 경로(path, npoints) 입력
 * - 옵션 (벡터, 라벨) 토글
 * - "Show BZ Plot" / "Show Crystal" 토글 버튼
 * - Clear BZ 버튼
 * - 상태/에러/특수점 테이블 표시
 *
 * 실제 BZ 계산/렌더링은 AtomsTemplate 도메인 메서드에 위임.
 */
class BZPlotUI {
public:
    explicit BZPlotUI(AtomsTemplate* parent);

    /// "Brillouin Zone Plot" 섹션 전체 렌더링
    void render();
    bool IsShowingBZ() const { return m_showingBZ; }
    void SetShowingBZ(bool showing);

private:
    AtomsTemplate* m_parent = nullptr;

    // 기존 static 지역 변수들을 멤버로 이동
    char  m_pathInput[128] = "All";
    int   m_npointsInput   = 50;
    bool  m_showVectors    = true;
    bool  m_showLabels     = true;
    bool  m_showingBZ      = false;
    std::string m_lastErrorMessage;

    void renderBandpathConfig();
    void renderOptions();
    void renderToggleAndClearButtons();
    void renderStatus();
    void renderSpecialPointsTable();

    void renderBZplot();
};

} // namespace ui
} // namespace atoms
