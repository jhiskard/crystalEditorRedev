#include "bond_ui.h"

#include "bonds_controller.h"

#include <algorithm>
#include <cmath>

namespace features::edit::bonds {

BondUI::BondUI(BondsController& controller)
    : controller_(controller) {
}

void BondUI::Render(bool* open) {
    if (open != nullptr && !(*open)) {
        return;
    }

    if (!ImGui::Begin("Bonds Management", open)) {
        ImGui::End();
        return;
    }

    if (!initialized_) {
        thickness_ = controller_.GetGlobalThickness();
        opacity_ = controller_.GetGlobalOpacity();
        globalDistanceFactorPercent_ = 0.0f;
        bondTypesLinkedToGlobalFactor_ = true;
        initialized_ = true;
    }

    const int32_t sid = controller_.ActiveStructureId();
    if (sid < 0) {
        ImGui::TextDisabled("No active structure");
        if (ImGui::Button("Create structure")) {
            controller_.EnsureActiveStructure();
        }
        ImGui::End();
        return;
    }

    ImGui::Text("Structure: %d", sid);
    ImGui::SameLine();
    ImGui::Text("Bonds: %d", controller_.ActiveBondCount());

    if (ImGui::Button("Recompute")) {
        controller_.RecomputeAll();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear All")) {
        controller_.ClearAll();
    }

    if (ImGui::SliderFloat("Thickness", &thickness_, 0.1f, 3.0f, "%.2f")) {
        controller_.SetGlobalThickness(thickness_);
        thickness_ = controller_.GetGlobalThickness();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset##BondThickness")) {
        thickness_ = 1.0f;
        controller_.SetGlobalThickness(thickness_);
        thickness_ = controller_.GetGlobalThickness();
    }

    if (ImGui::SliderFloat("Opacity", &opacity_, 0.1f, 1.0f, "%.2f")) {
        controller_.SetGlobalOpacity(opacity_);
        opacity_ = controller_.GetGlobalOpacity();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset##BondOpacity")) {
        opacity_ = 1.0f;
        controller_.SetGlobalOpacity(opacity_);
        opacity_ = controller_.GetGlobalOpacity();
    }

    ImGui::Separator();
    ImGui::Text("Bond Distance Parameters");

    auto applyGlobalFactorToAllTypes = [&](float percent) {
        const std::vector<std::string> types = controller_.ListBondTypes();
        for (const std::string& bondType : types) {
            const float defaultThreshold = std::max(0.01f, controller_.GetDefaultBondDistanceThreshold(bondType));
            const float nextThreshold = defaultThreshold * (1.0f + (percent / 100.0f));
            thresholdPercentDrafts_[bondType] = percent;
            controller_.SetBondDistanceThreshold(bondType, nextThreshold);
        }
    };

    if (ImGui::SliderFloat("Bond distance factor (%)", &globalDistanceFactorPercent_, -50.0f, 50.0f, "%.1f%%")) {
        globalDistanceFactorPercent_ = std::clamp(globalDistanceFactorPercent_, -50.0f, 50.0f);
        if (bondTypesLinkedToGlobalFactor_) {
            applyGlobalFactorToAllTypes(globalDistanceFactorPercent_);
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset##GlobalBondDistanceFactor")) {
        globalDistanceFactorPercent_ = 0.0f;
        bondTypesLinkedToGlobalFactor_ = true;
        applyGlobalFactorToAllTypes(globalDistanceFactorPercent_);
    }
    if (!bondTypesLinkedToGlobalFactor_) {
        ImGui::TextDisabled("Bond Types are currently unlinked from global factor.");
    }

    ImGui::Separator();
    ImGui::Text("Bond Types");

    const std::vector<std::string> bondTypes = controller_.ListBondTypes();
    if (bondTypes.empty()) {
        ImGui::TextDisabled("No bond types yet. Press Recompute.");
        ImGui::End();
        return;
    }

    for (const std::string& bondType : bondTypes) {
        ImGui::PushID(bondType.c_str());

        bool visible = controller_.IsBondTypeVisible(bondType);
        if (ImGui::Checkbox("##visible", &visible)) {
            controller_.SetBondTypeVisible(bondType, visible);
        }
        ImGui::SameLine();
        ImGui::Text("%s (%d)", bondType.c_str(), controller_.CountBondsByType(bondType));

        const float defaultThreshold = std::max(0.01f, controller_.GetDefaultBondDistanceThreshold(bondType));
        const float threshold = controller_.GetBondDistanceThreshold(bondType);
        const float currentPercent = std::clamp(((threshold / defaultThreshold) - 1.0f) * 100.0f, -50.0f, 50.0f);

        if (bondTypesLinkedToGlobalFactor_) {
            thresholdPercentDrafts_[bondType] = globalDistanceFactorPercent_;
        } else {
            auto draftIt = thresholdPercentDrafts_.find(bondType);
            if (draftIt == thresholdPercentDrafts_.end() || std::fabs(draftIt->second - currentPercent) > 1e-4f) {
                thresholdPercentDrafts_[bondType] = currentPercent;
            }
        }

        float sliderValue = thresholdPercentDrafts_[bondType];
        if (ImGui::SliderFloat("Threshold (%)", &sliderValue, -50.0f, 50.0f, "%.1f%%")) {
            sliderValue = std::clamp(sliderValue, -50.0f, 50.0f);
            if (bondTypesLinkedToGlobalFactor_) {
                bondTypesLinkedToGlobalFactor_ = false;
            }
            thresholdPercentDrafts_[bondType] = sliderValue;
            const float nextThreshold = defaultThreshold * (1.0f + (sliderValue / 100.0f));
            controller_.SetBondDistanceThreshold(bondType, nextThreshold);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset##BondTypeThreshold")) {
            const float resetPercent = bondTypesLinkedToGlobalFactor_ ? globalDistanceFactorPercent_ : 0.0f;
            thresholdPercentDrafts_[bondType] = resetPercent;
            const float resetThreshold = defaultThreshold * (1.0f + (resetPercent / 100.0f));
            controller_.SetBondDistanceThreshold(bondType, resetThreshold);
        }
        ImGui::TextDisabled("Current: %.3f A (Default: %.3f A)", threshold, defaultThreshold);

        ImGui::Separator();
        ImGui::PopID();
    }

    ImGui::End();
}

} // namespace features::edit::bonds
