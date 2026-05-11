/**
 * @file features/edit/bonds/bond_manager.h
 * @brief Bond data mutator and recompute logic for Edit/Bonds.
 */
#pragma once

#include "core/scene/scene_state.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace features::edit::bonds {

class BondManager {
public:
    explicit BondManager(core::scene::SceneState& scene);

    void RecomputeAll(int32_t structureId);
    void ClearAll(int32_t structureId);

    void SetBondTypeVisible(int32_t structureId, const std::string& bondTypeKey, bool visible);
    bool IsBondTypeVisible(const std::string& bondTypeKey) const;

    void SetBondDistanceThreshold(const std::string& bondTypeKey, float distance);
    float GetBondDistanceThreshold(const std::string& bondTypeKey) const;
    float GetDefaultBondDistanceThreshold(const std::string& bondTypeKey) const;

    void SetGlobalThickness(float thickness);
    void SetGlobalOpacity(float opacity);
    float GetGlobalThickness() const { return globalThickness_; }
    float GetGlobalOpacity() const { return globalOpacity_; }

    std::vector<std::string> ListBondTypes(int32_t structureId) const;
    int CountBondsByType(int32_t structureId, const std::string& bondTypeKey) const;
    int ActiveBondCount(int32_t structureId) const;

private:
    int32_t ResolveStructureId(int32_t structureId) const;
    core::scene::StructureRecord* ResolveRecordMutable(int32_t structureId);
    const core::scene::StructureRecord* ResolveRecordConst(int32_t structureId) const;

    static std::string BuildBondTypeKey(const std::string& symbolA, const std::string& symbolB);
    float ComputeDefaultThreshold(const std::string& symbolA, const std::string& symbolB) const;
    float GetOrCreateThreshold(const std::string& bondTypeKey, const std::string& symbolA, const std::string& symbolB);

    core::scene::SceneState& scene_;
    std::unordered_map<std::string, float> thresholdByType_;
    std::unordered_map<std::string, bool> visibleByType_;
    float globalThickness_ = 1.0f;
    float globalOpacity_ = 1.0f;
};

} // namespace features::edit::bonds
