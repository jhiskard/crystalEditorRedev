#include "atom_manager.h"
#include "../atoms_template.h"
#include "bond_manager.h"
#include "element_database.h"
#include "cell_manager.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace atoms {
namespace domain {

// 전역 원자 벡터 정의 (실제 메모리 할당)
std::vector<atoms::domain::AtomInfo> createdAtoms;
std::vector<atoms::domain::AtomInfo> surroundingAtoms;
std::map<std::string, atoms::domain::AtomGroupInfo> atomGroups;
static bool surroundingsVisible = false;
static uint32_t nextAtomId = 1;

bool isSurroundingsVisible() {
    return surroundingsVisible;
}

void setSurroundingsVisible(bool visible) {
    surroundingsVisible = visible;
}

uint32_t generateUniqueAtomId() {
    return nextAtomId++;
}

void applyAtomChanges(::AtomsTemplate* parent,
                      float bondScalingFactor,
                      const ElementDatabase& elementDB) {
    (void)bondScalingFactor;
    if (!parent) {
        SPDLOG_ERROR("applyAtomChanges called with null parent");
        return;
    }

    std::vector<size_t> modifiedAtomIndices;
    int changesApplied = 0;
    bool hasModifications = false;

    for (size_t i = 0; i < createdAtoms.size(); ++i) {
        if (createdAtoms[i].modified) {
            hasModifications = true;
            modifiedAtomIndices.push_back(i);
        }
    }

    if (!hasModifications) {
        SPDLOG_DEBUG("No modified atoms found - skipping applyAtomChanges");
        return;
    }

    SPDLOG_INFO("Applying atom changes for {} modified atoms", modifiedAtomIndices.size());

    constexpr float kMinAtomRadius = 0.001f;
    constexpr float kPositionEpsilon = 1e-6f;
    constexpr float kRadiusEpsilon = 1e-6f;
    bool hasBondAffectingChanges = false;

    for (size_t idx : modifiedAtomIndices) {
        atoms::domain::AtomInfo& atom = createdAtoms[idx];

        bool symbolChanged = (atom.symbol != atom.tempSymbol);
        bool positionChanged =
            std::fabs(atom.position[0] - atom.tempPosition[0]) > kPositionEpsilon ||
            std::fabs(atom.position[1] - atom.tempPosition[1]) > kPositionEpsilon ||
            std::fabs(atom.position[2] - atom.tempPosition[2]) > kPositionEpsilon;
        bool radiusChanged =
            std::fabs(atom.radius - atom.tempRadius) > kRadiusEpsilon;

        if (symbolChanged) {
            hasBondAffectingChanges = true;
            SPDLOG_DEBUG("Symbol changed for atom {}: {} -> {}", idx + 1, atom.symbol, atom.tempSymbol);

            std::string oldSymbol = atom.symbol;
            if (atom.id != 0) {
                removeAtomFromGroup(oldSymbol, atom.id);
                if (auto* batch = parent->batchSystem()) {
                    batch->scheduleAtomGroupUpdate(oldSymbol);
                }
            }

            atom.symbol = atom.tempSymbol;
            atom.position[0] = atom.tempPosition[0];
            atom.position[1] = atom.tempPosition[1];
            atom.position[2] = atom.tempPosition[2];
            atom.fracPosition[0] = atom.tempFracPosition[0];
            atom.fracPosition[1] = atom.tempFracPosition[1];
            atom.fracPosition[2] = atom.tempFracPosition[2];
            // Keep manually edited radius when symbol and radius are edited together.
            // If radius was not edited, follow element default radius for the new symbol.
            if (radiusChanged) {
                const float clampedRadius = std::max(atom.tempRadius, kMinAtomRadius);
                atom.radius = clampedRadius;
                atom.tempRadius = clampedRadius;
            } else {
                float newRadius = elementDB.getDefaultRadius(atom.symbol);
                atom.radius = std::max(newRadius, kMinAtomRadius);
                atom.tempRadius = atom.radius;
            }
            atom.bondRadius = std::max(elementDB.getDefaultRadius(atom.symbol), kMinAtomRadius);

            atom.color = elementDB.getDefaultColor(atom.symbol);

            initializeAtomGroup(atom.symbol, atom.radius);
            parent->initializeUnifiedAtomGroupVTK(atom.symbol, atom.radius);

            float adjustedRadius = atom.radius * 0.5f;
            vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
            transform->Identity();
            transform->Translate(atom.position[0], atom.position[1], atom.position[2]);

            float scaleFactor = adjustedRadius / 0.5f;
            transform->Scale(scaleFactor, scaleFactor, scaleFactor);

            atoms::domain::addAtomToGroup(atom.symbol, transform, atom.color, atom.id, atom.atomType, idx);
            if (auto* batch = parent->batchSystem()) {
                batch->scheduleAtomGroupUpdate(atom.symbol);
            }

            SPDLOG_DEBUG("Migrated atom {} from group {} to group {} (ID: {})",
                        idx + 1, oldSymbol, atom.symbol, atom.id);

        } else if (positionChanged || radiusChanged) {
            SPDLOG_DEBUG("Position/radius changed for atom {}: updating transform", idx + 1);
            if (positionChanged) {
                hasBondAffectingChanges = true;
            }

            float oldPosition[3] = {atom.position[0], atom.position[1], atom.position[2]};
            atom.position[0] = atom.tempPosition[0];
            atom.position[1] = atom.tempPosition[1];
            atom.position[2] = atom.tempPosition[2];
            atom.fracPosition[0] = atom.tempFracPosition[0];
            atom.fracPosition[1] = atom.tempFracPosition[1];
            atom.fracPosition[2] = atom.tempFracPosition[2];
            atom.radius = std::max(atom.tempRadius, kMinAtomRadius);
            atom.tempRadius = atom.radius;

            float positionDelta[3] = {
                atom.position[0] - oldPosition[0],
                atom.position[1] - oldPosition[1],
                atom.position[2] - oldPosition[2]
            };

            bool hasPositionChange =
                (positionDelta[0] != 0.0f || positionDelta[1] != 0.0f || positionDelta[2] != 0.0f);

            if (atom.id != 0 && atomGroups.find(atom.symbol) != atomGroups.end()) {
                auto& group = atomGroups[atom.symbol];
                auto it = std::find(group.atomIds.begin(), group.atomIds.end(), atom.id);
                if (it != group.atomIds.end()) {
                    size_t instanceIndex = std::distance(group.atomIds.begin(), it);
                    if (instanceIndex < group.transforms.size()) {
                        vtkSmartPointer<vtkTransform> existingTransform = group.transforms[instanceIndex];
                        if (existingTransform) {
                            existingTransform->Identity();
                            existingTransform->Translate(atom.position[0], atom.position[1], atom.position[2]);

                            float adjustedRadius = atom.radius * 0.5f;
                            float scaleFactor = adjustedRadius / 0.5f;
                            existingTransform->Scale(scaleFactor, scaleFactor, scaleFactor);

                            if (auto* batch = parent->batchSystem()) {
                                batch->scheduleAtomGroupUpdate(atom.symbol);
                            }

                            SPDLOG_DEBUG("Updated existing transform for atom {} (ID: {}, instance: {})",
                                       idx + 1, atom.id, instanceIndex);
                        }
                    }
                }
            }

            if (parent->isSurroundingsVisible() && atom.id != 0 && (hasPositionChange || radiusChanged)) {
                SPDLOG_DEBUG("Updating SURROUNDING atoms derived from original atom {} (ID: {}, positionChanged: {}, radiusChanged: {})",
                           idx + 1, atom.id, hasPositionChange, radiusChanged);

                int updatedSurroundingAtoms = 0;
                std::set<std::string> updatedSurroundingGroups;

                for (size_t j = 0; j < surroundingAtoms.size(); j++) {
                    atoms::domain::AtomInfo& surroundingAtom = surroundingAtoms[j];

                    if (surroundingAtom.originalAtomId == atom.id) {
                        float oldSurroundingPosition[3] = {
                            surroundingAtom.position[0],
                            surroundingAtom.position[1],
                            surroundingAtom.position[2]
                        };
                        const float oldSurroundingRadius = surroundingAtom.radius;

                        if (hasPositionChange) {
                            surroundingAtom.position[0] += positionDelta[0];
                            surroundingAtom.position[1] += positionDelta[1];
                            surroundingAtom.position[2] += positionDelta[2];

                            atoms::domain::cartesianToFractional(
                                surroundingAtom.position,
                                surroundingAtom.fracPosition,
                                cellInfo.invmatrix);

                            surroundingAtom.tempPosition[0] = surroundingAtom.position[0];
                            surroundingAtom.tempPosition[1] = surroundingAtom.position[1];
                            surroundingAtom.tempPosition[2] = surroundingAtom.position[2];
                            surroundingAtom.tempFracPosition[0] = surroundingAtom.fracPosition[0];
                            surroundingAtom.tempFracPosition[1] = surroundingAtom.fracPosition[1];
                            surroundingAtom.tempFracPosition[2] = surroundingAtom.fracPosition[2];
                        }

                        if (radiusChanged) {
                            surroundingAtom.radius = atom.radius;
                            surroundingAtom.tempRadius = surroundingAtom.radius;
                        }

                        if (surroundingAtom.id != 0 && atomGroups.find(surroundingAtom.symbol) != atomGroups.end()) {
                            auto& surroundingGroup = atomGroups[surroundingAtom.symbol];
                            auto surr_it = std::find(surroundingGroup.atomIds.begin(),
                                                     surroundingGroup.atomIds.end(),
                                                     surroundingAtom.id);
                            if (surr_it != surroundingGroup.atomIds.end()) {
                                size_t surroundingInstanceIndex = std::distance(surroundingGroup.atomIds.begin(), surr_it);
                                if (surroundingInstanceIndex < surroundingGroup.transforms.size()) {
                                    vtkSmartPointer<vtkTransform> surroundingTransform =
                                        surroundingGroup.transforms[surroundingInstanceIndex];

                                    if (surroundingTransform) {
                                        surroundingTransform->Identity();
                                        surroundingTransform->Translate(surroundingAtom.position[0],
                                                                       surroundingAtom.position[1],
                                                                       surroundingAtom.position[2]);

                                        float surroundingAdjustedRadius = surroundingAtom.radius * 0.5f;
                                        float surroundingScaleFactor = surroundingAdjustedRadius / 0.5f;
                                        surroundingTransform->Scale(surroundingScaleFactor,
                                                                  surroundingScaleFactor,
                                                                  surroundingScaleFactor);

                                        updatedSurroundingGroups.insert(surroundingAtom.symbol);
                                    }
                                }
                            }
                        }

                        updatedSurroundingAtoms++;

                        if (hasPositionChange && radiusChanged) {
                            SPDLOG_DEBUG("Updated SURROUNDING atom {} (ID: {}) derived from original atom {} - position: ({:.3f}, {:.3f}, {:.3f}) -> ({:.3f}, {:.3f}, {:.3f}), radius: {:.3f} -> {:.3f}",
                                       j + 1, surroundingAtom.id, atom.id,
                                       oldSurroundingPosition[0], oldSurroundingPosition[1], oldSurroundingPosition[2],
                                       surroundingAtom.position[0], surroundingAtom.position[1], surroundingAtom.position[2],
                                       oldSurroundingRadius, surroundingAtom.radius);
                        } else if (hasPositionChange) {
                            SPDLOG_DEBUG("Updated SURROUNDING atom {} (ID: {}) derived from original atom {} - position: ({:.3f}, {:.3f}, {:.3f}) -> ({:.3f}, {:.3f}, {:.3f})",
                                       j + 1, surroundingAtom.id, atom.id,
                                       oldSurroundingPosition[0], oldSurroundingPosition[1], oldSurroundingPosition[2],
                                       surroundingAtom.position[0], surroundingAtom.position[1], surroundingAtom.position[2]);
                        } else if (radiusChanged) {
                            SPDLOG_DEBUG("Updated SURROUNDING atom {} (ID: {}) derived from original atom {} - radius: {:.3f} -> {:.3f}",
                                       j + 1, surroundingAtom.id, atom.id,
                                       oldSurroundingRadius, surroundingAtom.radius);
                        }
                    }
                }

                for (const std::string& groupSymbol : updatedSurroundingGroups) {
                    if (auto* batch = parent->batchSystem()) {
                        batch->scheduleAtomGroupUpdate(groupSymbol);
                    }
                }

                if (updatedSurroundingAtoms > 0) {
                    SPDLOG_INFO("Updated {} SURROUNDING atoms derived from original atom {} (ID: {})",
                               updatedSurroundingAtoms, idx + 1, atom.id);
                }
            }

        } else {
            SPDLOG_DEBUG("No actual changes detected for atom {} despite modified flag", idx + 1);
        }

        atom.tempSymbol = atom.symbol;
        atom.tempPosition[0] = atom.position[0];
        atom.tempPosition[1] = atom.position[1];
        atom.tempPosition[2] = atom.position[2];
        atom.tempFracPosition[0] = atom.fracPosition[0];
        atom.tempFracPosition[1] = atom.fracPosition[1];
        atom.tempFracPosition[2] = atom.fracPosition[2];
        atom.tempRadius = atom.radius;

        atom.modified = false;
        changesApplied++;

        SPDLOG_DEBUG("Applied changes to atom {}: symbol={}, position=({:.4f}, {:.4f}, {:.4f}), radius={:.3f}",
                    idx + 1, atom.symbol, atom.position[0], atom.position[1], atom.position[2], atom.radius);
    }

    SPDLOG_INFO("Applied changes to {} atoms", changesApplied);

    if (!hasBondAffectingChanges) {
        SPDLOG_INFO("Skipping bond regeneration: only visual radius changes were applied");
        return;
    }

    SPDLOG_INFO("Regenerating ALL bonds due to bond-affecting atom modifications...");

    bool wasSurroundingsVisible = parent->isSurroundingsVisible();
    if (wasSurroundingsVisible) {
        SPDLOG_DEBUG("Temporarily disabling surrounding atoms for clean bond regeneration");
        parent->hideSurroundingAtoms();
    }

    SPDLOG_DEBUG("Creating all bonds from scratch");
    parent->createAllBonds();
    SPDLOG_DEBUG("All bonds recreated successfully");

    if (wasSurroundingsVisible) {
        SPDLOG_DEBUG("Re-enabling surrounding atoms with updated bonds");
        parent->createSurroundingAtoms();
    }

    SPDLOG_INFO("Completed full bond regeneration for {} modified atoms", changesApplied);
}

void initializeAtomGroup(const std::string& symbol, float radius) {
    auto it = atomGroups.find(symbol);
    if (it == atomGroups.end()) {
        AtomGroupInfo group;
        group.element    = symbol;
        group.baseRadius = radius;

        atomGroups.emplace(symbol, std::move(group));
        SPDLOG_INFO("Initialized new atom group: {} (radius = {})", symbol, radius);
    } else {
        it->second.baseRadius = radius;
        SPDLOG_INFO("Updated atom group radius: {} (radius = {})", symbol, radius);
    }
}

bool hasAtomGroup(const std::string& symbol) {
    return atomGroups.find(symbol) != atomGroups.end();
}

AtomGroupInfo* findAtomGroup(const std::string& symbol) {
    auto it = atomGroups.find(symbol);
    if (it == atomGroups.end()) {
        return nullptr;
    }
    return &it->second;
}

void addAtomToGroup(const std::string& symbol,
                    vtkSmartPointer<vtkTransform> transform,
                    // const ImVec4& color,
                    const Color4f& color, 
                    uint32_t atomId,
                    AtomType atomType,
                    size_t originalIndex) 
{
    auto it = atomGroups.find(symbol);
    if (it == atomGroups.end()) {
        SPDLOG_ERROR("addAtomToGroup: atom group '{}' not found", symbol);
        return;
    }

    auto& group = it->second;
    group.transforms.push_back(transform);
    group.colors.push_back(color);
    group.atomIds.push_back(atomId);
    group.atomTypes.push_back(atomType);
    group.originalIndices.push_back(originalIndex);
}

bool removeAtomFromGroup(const std::string& symbol, uint32_t atomId) {
    auto it = atomGroups.find(symbol);
    if (it == atomGroups.end()) {
        SPDLOG_WARN("removeAtomFromGroup: atom group '{}' not found", symbol);
        return false;
    }

    auto& group = it->second;
    const auto& ids = group.atomIds;

    auto pos = std::find(ids.begin(), ids.end(), atomId);
    if (pos == ids.end()) {
        SPDLOG_WARN("removeAtomFromGroup: atom {} not found in group '{}'", atomId, symbol);
        return false;
    }

    size_t index = static_cast<size_t>(std::distance(ids.begin(), pos));

    group.transforms.erase(group.transforms.begin() + index);
    group.colors.erase(group.colors.begin() + index);
    group.atomIds.erase(group.atomIds.begin() + index);
    group.atomTypes.erase(group.atomTypes.begin() + index);
    group.originalIndices.erase(group.originalIndices.begin() + index);

    return true;
}

void clearAllAtomGroups() {
    for (auto& [symbol, group] : atomGroups) {
        group.transforms.clear();
        group.colors.clear();
        group.atomIds.clear();
        group.atomTypes.clear();
        group.originalIndices.clear();
    }
    atomGroups.clear();

    SPDLOG_INFO("AtomManager: cleared all atom groups");
}

const std::map<std::string, AtomGroupInfo>& getAtomGroups() {
    return atomGroups;
}

bool getAtomPositionAndRadius(
    const atoms::domain::AtomInfo& atom, double position[3], double& radius
) {
    try {
        const double bondRadius = static_cast<double>(
            std::max(atom.bondRadius, 0.001f));

        // 편집 모드에서는 임시 위치만 사용 (결합 반경은 내부 bondRadius 고정)
        if (atom.modified) {
            position[0] = static_cast<double>(atom.tempPosition[0]);
            position[1] = static_cast<double>(atom.tempPosition[1]);
            position[2] = static_cast<double>(atom.tempPosition[2]);
            radius = bondRadius;
            return true;
        }
        
        // ✅ AtomInfo의 기본 위치 + 결합 전용 반경 사용
        position[0] = static_cast<double>(atom.position[0]);
        position[1] = static_cast<double>(atom.position[1]);
        position[2] = static_cast<double>(atom.position[2]);
        radius = bondRadius;
        
        return true;
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error getting atom position/radius: {}", e.what());
        return false;
    }
}

} // atoms
} // domain 
