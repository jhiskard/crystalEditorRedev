/**
 * @file features/edit/bonds/bonds_controller.h
 * @brief Integration controller for Edit/Bonds.
 */
#pragma once

#include "bond_manager.h"
#include "bond_renderer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace features::edit::bonds {

class BondsController {
public:
    explicit BondsController(core::scene::SceneState& scene);

    void Subscribe();
    void SubscribeAtomChanges();

    int32_t ActiveStructureId() const;
    int32_t EnsureActiveStructure();

    void RecomputeAll();
    void ClearAll();

    std::vector<std::string> ListBondTypes() const;
    int CountBondsByType(const std::string& bondTypeKey) const;
    int ActiveBondCount() const;

    void SetBondTypeVisible(const std::string& bondTypeKey, bool visible);
    bool IsBondTypeVisible(const std::string& bondTypeKey) const;

    void SetBondDistanceThreshold(const std::string& bondTypeKey, float threshold);
    float GetBondDistanceThreshold(const std::string& bondTypeKey) const;
    float GetDefaultBondDistanceThreshold(const std::string& bondTypeKey) const;

    void SetGlobalThickness(float thickness);
    float GetGlobalThickness() const;

    void SetGlobalOpacity(float opacity);
    float GetGlobalOpacity() const;

    BondManager& Manager() { return manager_; }
    BondRenderer& Renderer() { return renderer_; }

private:
    core::scene::SceneState& scene_;
    BondManager manager_;
    BondRenderer renderer_;
    bool subscribedAtomChanges_ = false;
};

} // namespace features::edit::bonds
