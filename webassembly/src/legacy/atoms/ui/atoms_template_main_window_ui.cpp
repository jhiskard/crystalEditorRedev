#include "atoms_template_main_window_ui.h"

#include <imgui.h>

// 하위 UI 모듈
#include "atom_editor_ui.h"
#include "bond_ui.h"
#include "cell_info_ui.h"
#include "bz_plot_ui.h"
#include "periodic_table_ui.h"
#include "bravais_lattice_ui.h"

// 선택: 로깅을 쓰고 있다면 유지, 아니면 제거 가능
// #include "../config/log_config.h"

namespace atoms::ui {

AtomsTemplateMainWindowUI::AtomsTemplateMainWindowUI(::AtomsTemplate* parent)
    : m_parent(parent)
{
    // parent 타입은 ::AtomsTemplate* 로 고정되어야 함
    m_atomEditorUI     = std::make_unique<AtomEditorUI>(m_parent);
    m_bondUI           = std::make_unique<BondUI>(m_parent);
    m_cellInfoUI       = std::make_unique<CellInfoUI>(m_parent);
    m_bzPlotUI         = std::make_unique<BZPlotUI>(m_parent);
    m_periodicTableUI  = std::make_unique<PeriodicTableUI>(m_parent);
    m_bravaisLatticeUI = std::make_unique<BravaisLatticeUI>(m_parent);
}

AtomsTemplateMainWindowUI::~AtomsTemplateMainWindowUI() = default;

void AtomsTemplateMainWindowUI::render(bool* openWindow)
{
    if (openWindow != nullptr) {
        if (!*openWindow) {
            return;
        }

        if (ImGui::Begin("Atomistic model builder", openWindow)) {

            if (ImGui::CollapsingHeader("Created Atoms")) {
                if (m_atomEditorUI) m_atomEditorUI->render();
            }

            if (ImGui::CollapsingHeader("Bonds Management")) {
                if (m_bondUI) m_bondUI->render();
            }

            if (ImGui::CollapsingHeader("Cell Information")) {
                if (m_cellInfoUI) m_cellInfoUI->render();
            }

            if (ImGui::CollapsingHeader("Brillouin Zone Plot")) {
                if (m_bzPlotUI) m_bzPlotUI->render();
            }

            if (ImGui::CollapsingHeader("Periodic Table")) {
                if (m_periodicTableUI) m_periodicTableUI->render();
            }

            if (ImGui::CollapsingHeader("Crystal Templates")) {
                if (m_bravaisLatticeUI) m_bravaisLatticeUI->render();
            }
        }

        ImGui::End();
        return;
    }

    // openWindow == nullptr 인 경우 (기존 동작 유지용)
    if (ImGui::Begin("Atoms Template")) {
        // placeholder
    }
    ImGui::End();
}

} // namespace atoms::ui
