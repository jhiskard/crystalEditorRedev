// webassembly/src/atoms/ui/bond_ui.cpp
#include "bond_ui.h"
#include "../atoms_template.h"
#include "../domain/bond_manager.h"
#include "../../config/log_config.h"

namespace atoms {
namespace ui {

BondUI::BondUI(AtomsTemplate* parent)
    : m_parent(parent) {
    SPDLOG_DEBUG("BondUI initialized");
}

void BondUI::render() {
    if (!m_parent) {
        ImGui::TextColored(
            ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
            "Bond UI is not connected to AtomsTemplate."
        );
        return;
    }

    renderBondOperationsSection();
    ImGui::Separator();
    renderBondStyleSection();
    ImGui::Separator();
    renderBondDistanceSection();
    ImGui::Separator();
    renderPerformanceSection();
}

void BondUI::renderBondOperationsSection() {
    ImGui::Text("Bond Operations:");

    // 모든 결합 생성 버튼
    if (ImGui::Button("Add Bonds (All)")) {
        SPDLOG_INFO("User requested to create all bonds");

        auto guard = m_parent->createBatchGuard();
        m_parent->createAllBonds();

        SPDLOG_INFO("All bonds recreated successfully");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Create bonds between all atoms\n"
            "based on current distance and tolerance settings."
        );
    }

    ImGui::SameLine();

    // 모든 결합 제거 버튼
    if (ImGui::Button("Clear Bonds (All)")) {
        SPDLOG_INFO("User requested to clear all bonds");

        auto guard = m_parent->createBatchGuard();
        m_parent->clearAllBonds();

        SPDLOG_INFO("All bonds cleared successfully");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Remove all existing bonds from the scene.\n"
            "This will not delete atoms themselves."
        );
    }
}

void BondUI::renderBondStyleSection() {
    ImGui::Text("Bond Style:");

    // 두께 조절
    float thickness = m_parent->getBondThickness();
    bool thicknessChanged = false;

    if (ImGui::SliderFloat("Thickness", &thickness, 0.1f, 3.0f, "%.1f")) {
        thicknessChanged = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Control visual thickness of bonds.\n"
            "Larger values make bonds appear thicker."
        );
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Reset##BondThickness")) {
        thickness = 1.0f;
        thicknessChanged = true;
    }

    if (thicknessChanged) {
        SPDLOG_DEBUG("Bond thickness changed to {:.2f}", thickness);
        m_parent->setBondThickness(thickness);
        m_parent->updateAllBondGroupThickness();
    }

    // 투명도 조절
    float opacity = m_parent->getBondOpacity();
    bool opacityChanged = false;

    if (ImGui::SliderFloat("Opacity", &opacity, 0.1f, 1.0f, "%.2f")) {
        opacityChanged = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Control transparency of bonds.\n"
            "Lower values make bonds more transparent."
        );
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Reset##BondOpacity")) {
        opacity = 1.0f;
        opacityChanged = true;
    }

    if (opacityChanged) {
        SPDLOG_DEBUG("Bond opacity changed to {:.2f}", opacity);
        m_parent->setBondOpacity(opacity);
        m_parent->updateAllBondGroupOpacity();
    }
}

void BondUI::renderBondDistanceSection() {
    ImGui::Text("Bond Distance Parameters:");

    // 거리 스케일 팩터
    float scalingFactor = m_parent->getBondScalingFactor();
    bool scalingChanged = false;

    if (ImGui::SliderFloat("Distance factor", &scalingFactor, 0.1f, 2.0f, "%.2f")) {
        scalingChanged = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Scale the maximum allowed distance for bond creation.\n"
            "Higher values allow longer bonds to be created."
        );
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Reset##BondDistanceFactor")) {
        scalingFactor = 1.0f;
        scalingChanged = true;
    }

    if (scalingChanged) {
        SPDLOG_DEBUG("Bond distance factor changed to {:.2f}", scalingFactor);
        m_parent->setBondScalingFactor(scalingFactor);

        // 거리 스케일 변경 시, 전체 결합을 다시 생성하여 반영
        auto guard = m_parent->createBatchGuard();
        m_parent->createAllBonds();
    }

    // 허용 오차 팩터
    float toleranceFactor = m_parent->getBondToleranceFactor();
    bool toleranceChanged = false;

    if (ImGui::SliderFloat("Tolerance factor", &toleranceFactor, 0.0f, 0.5f, "%.3f")) {
        toleranceChanged = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Fine-tune the tolerance for bond distance checks.\n"
            "Higher values make bond detection more lenient."
        );
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Reset##BondToleranceFactor")) {
        toleranceFactor = 0.10f;
        toleranceChanged = true;
    }

    if (toleranceChanged) {
        SPDLOG_DEBUG("Bond tolerance factor changed to {:.3f}", toleranceFactor);
        m_parent->setBondToleranceFactor(toleranceFactor);

        auto guard = m_parent->createBatchGuard();
        m_parent->createAllBonds();
    }
}

void BondUI::renderPerformanceSection() {
    ImGui::Text("Bond Performance:");

    auto stats = m_parent->getBondPerformanceStats();
    if (stats.updateCount <= 0) {
        ImGui::TextDisabled("No bond performance data available yet.");
        return;
    }

    ImGui::BulletText("Last update time: %.2f ms", stats.lastUpdateTime);
    ImGui::BulletText("Average update time: %.2f ms", stats.averageUpdateTime);
    ImGui::BulletText("Total bond updates: %d", stats.updateCount);

    if (stats.lastUpdateTime > 50.0f || stats.averageUpdateTime > 50.0f) {
        ImGui::TextColored(
            ImVec4(1.0f, 0.5f, 0.1f, 1.0f),
            "Tip: Bond update is relatively slow.\n"
            "- Try reducing the number of atoms.\n"
            "- Reduce the distance factor.\n"
            "- Hide boundary atoms when not needed."
        );
    }

    if (m_parent->isBatchMode()) {
        ImGui::TextColored(
            ImVec4(0.2f, 0.8f, 0.2f, 1.0f),
            "Batch mode is active - bond updates are optimized."
        );
    }
}

} // namespace ui
} // namespace atoms
