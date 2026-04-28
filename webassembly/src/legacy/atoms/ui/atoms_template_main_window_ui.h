#pragma once

#include <memory>

// 전역 AtomsTemplate로 고정 (네임스페이스 불일치 방지)
class AtomsTemplate;

namespace atoms::ui {

class AtomEditorUI;
class BondUI;
class CellInfoUI;
class BZPlotUI;
class PeriodicTableUI;
class BravaisLatticeUI;

/**
 * @brief AtomsTemplate의 최상위 ImGui 창/레이아웃을 전담하는 UI 파사드.
 *
 * - ImGui::Begin/End, CollapsingHeader 등 레이아웃 관리
 * - 하위 UI 모듈(AtomEditorUI/BondUI/...) 호출
 *
 * NOTE:
 * - parent 타입을 ::AtomsTemplate* 로 고정하여
 *   (atoms::AtomsTemplate* vs ::AtomsTemplate*) 네임스페이스 불일치로 인한 빌드 블로커를 제거.
 */
class AtomsTemplateMainWindowUI {
public:
    explicit AtomsTemplateMainWindowUI(::AtomsTemplate* parent);
    ~AtomsTemplateMainWindowUI();

    AtomsTemplateMainWindowUI(const AtomsTemplateMainWindowUI&) = delete;
    AtomsTemplateMainWindowUI& operator=(const AtomsTemplateMainWindowUI&) = delete;
    AtomsTemplateMainWindowUI(AtomsTemplateMainWindowUI&&) = delete;
    AtomsTemplateMainWindowUI& operator=(AtomsTemplateMainWindowUI&&) = delete;

    /// 기존 AtomsTemplate::Render(...) 역할을 대체
    void render(bool* openWindow = nullptr);

private:
    ::AtomsTemplate* m_parent = nullptr;

    std::unique_ptr<AtomEditorUI>     m_atomEditorUI;
    std::unique_ptr<BondUI>           m_bondUI;
    std::unique_ptr<CellInfoUI>       m_cellInfoUI;
    std::unique_ptr<BZPlotUI>         m_bzPlotUI;
    std::unique_ptr<PeriodicTableUI>  m_periodicTableUI;
    std::unique_ptr<BravaisLatticeUI> m_bravaisLatticeUI;
};

} // namespace atoms::ui
