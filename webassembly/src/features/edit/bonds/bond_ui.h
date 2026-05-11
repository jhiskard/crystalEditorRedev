/**
 * @file features/edit/bonds/bond_ui.h
 * @brief Bonds Management window for Edit/Bonds.
 */
#pragma once

#include <string>
#include <unordered_map>

#include <imgui.h>

namespace features::edit::bonds {

class BondsController;

class BondUI {
public:
    explicit BondUI(BondsController& controller);

    void Render(bool* open);

private:
    BondsController& controller_;
    float thickness_ = 1.0f;
    float opacity_ = 1.0f;
    float globalDistanceFactorPercent_ = 0.0f;
    bool bondTypesLinkedToGlobalFactor_ = true;
    bool initialized_ = false;
    std::unordered_map<std::string, float> thresholdPercentDrafts_;
};

} // namespace features::edit::bonds
