#include "surrounding_atom_manager.h"

#include "../atoms_template.h"
#include "atom_manager.h"
#include "bond_manager.h"
#include "cell_manager.h"
#include "element_database.h"
#include "../infrastructure/batch_update_system.h"
#include "../../config/log_config.h"

#include <algorithm>

namespace atoms::domain {

SurroundingAtomManager::SurroundingAtomManager(::AtomsTemplate* parent)
    : m_parent(parent) {}

void SurroundingAtomManager::createSurroundingAtoms() {
    if (!m_parent) {
        return;
    }

    SPDLOG_INFO("Creating surrounding atoms with unified rendering system...");

    if (m_parent->isSurroundingsVisible()) {
        for (auto& [symbol, group] : atomGroups) {
            auto& transforms = group.transforms;
            auto& colors = group.colors;
            auto& atomIds = group.atomIds;
            auto& atomTypes = group.atomTypes;
            auto& originalIndices = group.originalIndices;

            for (int i = static_cast<int>(atomTypes.size()) - 1; i >= 0; --i) {
                if (atomTypes[i] == atoms::domain::AtomType::SURROUNDING) {
                    transforms.erase(transforms.begin() + i);
                    colors.erase(colors.begin() + i);
                    atomIds.erase(atomIds.begin() + i);
                    atomTypes.erase(atomTypes.begin() + i);
                    originalIndices.erase(originalIndices.begin() + i);
                }
            }
        }

        surroundingAtoms.clear();
        surroundingBonds.clear();
        SPDLOG_DEBUG("Cleared existing surrounding atoms");
    }

    if (createdAtoms.empty() || !atoms::domain::isCellVisible()) {
        SPDLOG_WARN("Cannot create surrounding atoms: no atoms or no cell defined");
        return;
    }

    atoms::infrastructure::BatchUpdateSystem::BatchGuard guard(
        m_parent ? m_parent->batchSystem() : nullptr);

    try {
        const float threshold = 0.1f;
        int surroundingAtomsCreated = 0;
        int originalAtomsProcessed = 0;

        SPDLOG_DEBUG("Scanning original atoms for boundary proximity (threshold: {:.1f}%)", threshold * 100);

        for (const auto& originalAtom : createdAtoms) {
            if (originalAtom.atomType != atoms::domain::AtomType::ORIGINAL) {
                continue;
            }

            if (originalAtom.id == 0) {
                SPDLOG_WARN("Original atom {} has no ID - skipping surrounding generation",
                           originalAtom.symbol);
                continue;
            }

            originalAtomsProcessed++;

            float fracCoords[3];
            float currentPosition[3];

            if (originalAtom.modified) {
                currentPosition[0] = originalAtom.tempPosition[0];
                currentPosition[1] = originalAtom.tempPosition[1];
                currentPosition[2] = originalAtom.tempPosition[2];
            } else {
                currentPosition[0] = originalAtom.position[0];
                currentPosition[1] = originalAtom.position[1];
                currentPosition[2] = originalAtom.position[2];
            }

            atoms::domain::cartesianToFractional(currentPosition, fracCoords, cellInfo.invmatrix);

            bool nearLowBoundary = (fracCoords[0] <= threshold) ||
                                  (fracCoords[1] <= threshold) ||
                                  (fracCoords[2] <= threshold);
            bool nearHighBoundary = (fracCoords[0] >= (1.0f - threshold)) ||
                                   (fracCoords[1] >= (1.0f - threshold)) ||
                                   (fracCoords[2] >= (1.0f - threshold));

            if (!nearLowBoundary && !nearHighBoundary) {
                SPDLOG_DEBUG("Atom {} at fractional ({:.3f}, {:.3f}, {:.3f}) is NOT near boundary - skipping",
                           originalAtom.symbol, fracCoords[0], fracCoords[1], fracCoords[2]);
                continue;
            }

            SPDLOG_DEBUG("Atom {} at fractional ({:.3f}, {:.3f}, {:.3f}) is near boundary - creating surrounding atoms",
                        originalAtom.symbol, fracCoords[0], fracCoords[1], fracCoords[2]);

            std::vector<std::array<float, 3>> translationVectors;

            bool needsA_positive = (fracCoords[0] <= threshold);
            bool needsA_negative = (fracCoords[0] >= (1.0f - threshold));
            bool needsB_positive = (fracCoords[1] <= threshold);
            bool needsB_negative = (fracCoords[1] >= (1.0f - threshold));
            bool needsC_positive = (fracCoords[2] <= threshold);
            bool needsC_negative = (fracCoords[2] >= (1.0f - threshold));

            if (needsA_positive) {
                translationVectors.push_back({cellInfo.matrix[0][0], cellInfo.matrix[0][1], cellInfo.matrix[0][2]});
            }
            if (needsA_negative) {
                translationVectors.push_back({-cellInfo.matrix[0][0], -cellInfo.matrix[0][1], -cellInfo.matrix[0][2]});
            }
            if (needsB_positive) {
                translationVectors.push_back({cellInfo.matrix[1][0], cellInfo.matrix[1][1], cellInfo.matrix[1][2]});
            }
            if (needsB_negative) {
                translationVectors.push_back({-cellInfo.matrix[1][0], -cellInfo.matrix[1][1], -cellInfo.matrix[1][2]});
            }
            if (needsC_positive) {
                translationVectors.push_back({cellInfo.matrix[2][0], cellInfo.matrix[2][1], cellInfo.matrix[2][2]});
            }
            if (needsC_negative) {
                translationVectors.push_back({-cellInfo.matrix[2][0], -cellInfo.matrix[2][1], -cellInfo.matrix[2][2]});
            }

            if (needsA_positive && needsB_positive) {
                translationVectors.push_back({cellInfo.matrix[0][0] + cellInfo.matrix[1][0],
                                            cellInfo.matrix[0][1] + cellInfo.matrix[1][1],
                                            cellInfo.matrix[0][2] + cellInfo.matrix[1][2]});
            }
            if (needsA_positive && needsB_negative) {
                translationVectors.push_back({cellInfo.matrix[0][0] - cellInfo.matrix[1][0],
                                            cellInfo.matrix[0][1] - cellInfo.matrix[1][1],
                                            cellInfo.matrix[0][2] - cellInfo.matrix[1][2]});
            }
            if (needsA_negative && needsB_positive) {
                translationVectors.push_back({-cellInfo.matrix[0][0] + cellInfo.matrix[1][0],
                                            -cellInfo.matrix[0][1] + cellInfo.matrix[1][1],
                                            -cellInfo.matrix[0][2] + cellInfo.matrix[1][2]});
            }
            if (needsA_negative && needsB_negative) {
                translationVectors.push_back({-cellInfo.matrix[0][0] - cellInfo.matrix[1][0],
                                            -cellInfo.matrix[0][1] - cellInfo.matrix[1][1],
                                            -cellInfo.matrix[0][2] - cellInfo.matrix[1][2]});
            }

            if (needsB_positive && needsC_positive) {
                translationVectors.push_back({cellInfo.matrix[1][0] + cellInfo.matrix[2][0],
                                            cellInfo.matrix[1][1] + cellInfo.matrix[2][1],
                                            cellInfo.matrix[1][2] + cellInfo.matrix[2][2]});
            }
            if (needsB_positive && needsC_negative) {
                translationVectors.push_back({cellInfo.matrix[1][0] - cellInfo.matrix[2][0],
                                            cellInfo.matrix[1][1] - cellInfo.matrix[2][1],
                                            cellInfo.matrix[1][2] - cellInfo.matrix[2][2]});
            }
            if (needsB_negative && needsC_positive) {
                translationVectors.push_back({-cellInfo.matrix[1][0] + cellInfo.matrix[2][0],
                                            -cellInfo.matrix[1][1] + cellInfo.matrix[2][1],
                                            -cellInfo.matrix[1][2] + cellInfo.matrix[2][2]});
            }
            if (needsB_negative && needsC_negative) {
                translationVectors.push_back({-cellInfo.matrix[1][0] - cellInfo.matrix[2][0],
                                            -cellInfo.matrix[1][1] - cellInfo.matrix[2][1],
                                            -cellInfo.matrix[1][2] - cellInfo.matrix[2][2]});
            }

            if (needsA_positive && needsC_positive) {
                translationVectors.push_back({cellInfo.matrix[0][0] + cellInfo.matrix[2][0],
                                            cellInfo.matrix[0][1] + cellInfo.matrix[2][1],
                                            cellInfo.matrix[0][2] + cellInfo.matrix[2][2]});
            }
            if (needsA_positive && needsC_negative) {
                translationVectors.push_back({cellInfo.matrix[0][0] - cellInfo.matrix[2][0],
                                            cellInfo.matrix[0][1] - cellInfo.matrix[2][1],
                                            cellInfo.matrix[0][2] - cellInfo.matrix[2][2]});
            }
            if (needsA_negative && needsC_positive) {
                translationVectors.push_back({-cellInfo.matrix[0][0] + cellInfo.matrix[2][0],
                                            -cellInfo.matrix[0][1] + cellInfo.matrix[2][1],
                                            -cellInfo.matrix[0][2] + cellInfo.matrix[2][2]});
            }
            if (needsA_negative && needsC_negative) {
                translationVectors.push_back({-cellInfo.matrix[0][0] - cellInfo.matrix[2][0],
                                            -cellInfo.matrix[0][1] - cellInfo.matrix[2][1],
                                            -cellInfo.matrix[0][2] - cellInfo.matrix[2][2]});
            }

            if (needsA_positive && needsB_positive && needsC_positive) {
                translationVectors.push_back({cellInfo.matrix[0][0] + cellInfo.matrix[1][0] + cellInfo.matrix[2][0],
                                            cellInfo.matrix[0][1] + cellInfo.matrix[1][1] + cellInfo.matrix[2][1],
                                            cellInfo.matrix[0][2] + cellInfo.matrix[1][2] + cellInfo.matrix[2][2]});
            }
            if (needsA_positive && needsB_positive && needsC_negative) {
                translationVectors.push_back({cellInfo.matrix[0][0] + cellInfo.matrix[1][0] - cellInfo.matrix[2][0],
                                            cellInfo.matrix[0][1] + cellInfo.matrix[1][1] - cellInfo.matrix[2][1],
                                            cellInfo.matrix[0][2] + cellInfo.matrix[1][2] - cellInfo.matrix[2][2]});
            }
            if (needsA_positive && needsB_negative && needsC_positive) {
                translationVectors.push_back({cellInfo.matrix[0][0] - cellInfo.matrix[1][0] + cellInfo.matrix[2][0],
                                            cellInfo.matrix[0][1] - cellInfo.matrix[1][1] + cellInfo.matrix[2][1],
                                            cellInfo.matrix[0][2] - cellInfo.matrix[1][2] + cellInfo.matrix[2][2]});
            }
            if (needsA_positive && needsB_negative && needsC_negative) {
                translationVectors.push_back({cellInfo.matrix[0][0] - cellInfo.matrix[1][0] - cellInfo.matrix[2][0],
                                            cellInfo.matrix[0][1] - cellInfo.matrix[1][1] - cellInfo.matrix[2][1],
                                            cellInfo.matrix[0][2] - cellInfo.matrix[1][2] - cellInfo.matrix[2][2]});
            }
            if (needsA_negative && needsB_positive && needsC_positive) {
                translationVectors.push_back({-cellInfo.matrix[0][0] + cellInfo.matrix[1][0] + cellInfo.matrix[2][0],
                                            -cellInfo.matrix[0][1] + cellInfo.matrix[1][1] + cellInfo.matrix[2][1],
                                            -cellInfo.matrix[0][2] + cellInfo.matrix[1][2] + cellInfo.matrix[2][2]});
            }
            if (needsA_negative && needsB_positive && needsC_negative) {
                translationVectors.push_back({-cellInfo.matrix[0][0] + cellInfo.matrix[1][0] - cellInfo.matrix[2][0],
                                            -cellInfo.matrix[0][1] + cellInfo.matrix[1][1] - cellInfo.matrix[2][1],
                                            -cellInfo.matrix[0][2] + cellInfo.matrix[1][2] - cellInfo.matrix[2][2]});
            }
            if (needsA_negative && needsB_negative && needsC_positive) {
                translationVectors.push_back({-cellInfo.matrix[0][0] - cellInfo.matrix[1][0] + cellInfo.matrix[2][0],
                                            -cellInfo.matrix[0][1] - cellInfo.matrix[1][1] + cellInfo.matrix[2][1],
                                            -cellInfo.matrix[0][2] - cellInfo.matrix[1][2] + cellInfo.matrix[2][2]});
            }
            if (needsA_negative && needsB_negative && needsC_negative) {
                translationVectors.push_back({-cellInfo.matrix[0][0] - cellInfo.matrix[1][0] - cellInfo.matrix[2][0],
                                            -cellInfo.matrix[0][1] - cellInfo.matrix[1][1] - cellInfo.matrix[2][1],
                                            -cellInfo.matrix[0][2] - cellInfo.matrix[1][2] - cellInfo.matrix[2][2]});
            }

            const auto& elementDB = atoms::domain::ElementDatabase::getInstance();
            for (const auto& translation : translationVectors) {
                float surroundingPosition[3];
                surroundingPosition[0] = currentPosition[0] + translation[0];
                surroundingPosition[1] = currentPosition[1] + translation[1];
                surroundingPosition[2] = currentPosition[2] + translation[2];

                std::string symbol = originalAtom.modified ? originalAtom.tempSymbol : originalAtom.symbol;
                atoms::domain::Color4f color = elementDB.getDefaultColor(symbol);
                float radius = originalAtom.modified ? originalAtom.tempRadius : originalAtom.radius;
                radius = std::max(radius, 0.001f);

                m_parent->createAtomSphere(symbol.c_str(), color, radius, surroundingPosition, atoms::domain::AtomType::SURROUNDING);

                if (!surroundingAtoms.empty()) {
                    surroundingAtoms.back().originalAtomId = originalAtom.id;
                    surroundingAtoms.back().bondRadius = std::max(originalAtom.bondRadius, 0.001f);
                    surroundingAtomsCreated++;

                    SPDLOG_DEBUG("Created surrounding atom {} (ID: {}) derived from original atom ID: {}",
                               symbol, surroundingAtoms.back().id, originalAtom.id);
                }
            }

            if (!translationVectors.empty()) {
                SPDLOG_DEBUG("Original atom {} (ID: {}) generated {} surrounding atoms",
                           originalAtom.symbol, originalAtom.id, translationVectors.size());
            }
        }

        m_parent->setSurroundingsVisible(true);

        SPDLOG_INFO("Created {} surrounding atoms from {} boundary atoms (processed {}/{} original atoms)",
                    surroundingAtomsCreated,
                    surroundingAtomsCreated > 0 ? "some" : "none",
                    originalAtomsProcessed, createdAtoms.size());

        SPDLOG_DEBUG("Creating bonds for surrounding atoms...");

        if (!surroundingBonds.empty()) {
            SPDLOG_DEBUG("Clearing {} existing surrounding bonds", surroundingBonds.size());
            for (const auto& bond : surroundingBonds) {
                if (bond.isInstanced && bond.instanceIndex != SIZE_MAX) {
                    if (atoms::domain::removeBondFromGroup(bond.bondGroupKey, bond.instanceIndex)) {
                        if (auto* batch = m_parent->batchSystem()) {
                            batch->scheduleBondGroupUpdate(bond.bondGroupKey);
                        }
                    }
                }
            }
            surroundingBonds.clear();
        }

        SPDLOG_DEBUG("Creating bonds between original and surrounding atoms...");
        m_parent->createBondsForAtoms({}, true, true, false);

        SPDLOG_INFO("Created surrounding bonds (all instanced)");

    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error during createSurroundingAtoms: {}", e.what());

        for (const auto& atom : surroundingAtoms) {
            if (atom.id != 0) {
                removeAtomFromGroup(atom.symbol, atom.id);
            }
        }
        surroundingAtoms.clear();
        surroundingBonds.clear();
        m_parent->setSurroundingsVisible(false);

        throw;
    }

    SPDLOG_INFO("Completed createSurroundingAtoms with boundary filtering and bond generation");
}

void SurroundingAtomManager::hideSurroundingAtoms() {
    if (!m_parent) {
        return;
    }

    SPDLOG_INFO("Hiding surrounding atoms with unified rendering system...");

    atoms::infrastructure::BatchUpdateSystem::BatchGuard guard(
        m_parent ? m_parent->batchSystem() : nullptr);

    try {
        SPDLOG_DEBUG("Clearing all bonds (original + surrounding) within batch...");

        int originalBondsCount = createdBonds.size();
        int surroundingBondsCount = surroundingBonds.size();

        clearAllBondGroups();
        createdBonds.clear();
        surroundingBonds.clear();
        m_parent->resetBondIdCounter(1);

        SPDLOG_INFO("Cleared all bonds: {} original + {} surrounding (all were instanced)",
                   originalBondsCount, surroundingBondsCount);

        SPDLOG_DEBUG("Removing surrounding atoms with unified rendering...");

        int atomsRemoved = 0;
        for (const auto& atom : surroundingAtoms) {
            if (atom.id != 0) {
                if (atoms::domain::removeAtomFromGroup(atom.symbol, atom.id)) {
                    if (auto* batch = m_parent->batchSystem()) {
                        batch->scheduleAtomGroupUpdate(atom.symbol);
                    }
                }
                atomsRemoved++;
            }
        }

        surroundingAtoms.clear();
        m_parent->setSurroundingsVisible(false);

        SPDLOG_INFO("Removed {} surrounding atoms (all were instanced)", atomsRemoved);

        SPDLOG_DEBUG("Recreating original bonds within batch...");

        m_parent->createBondsForAtoms({}, true, false, false);

        SPDLOG_INFO("Recreated bonds for original atoms only (all instanced)");

    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error during hideSurroundingAtoms: {}", e.what());

        surroundingAtoms.clear();
        surroundingBonds.clear();
        m_parent->setSurroundingsVisible(false);

        throw;
    }

    SPDLOG_INFO("Completed hideSurroundingAtoms (all unified instanced)");
}

bool SurroundingAtomManager::isVisible() const {
    return m_parent ? m_parent->isSurroundingsVisible() : false;
}

void SurroundingAtomManager::setVisible(bool visible) {
    if (!m_parent) {
        return;
    }
    m_parent->setSurroundingsVisible(visible);
}

} // namespace atoms::domain
