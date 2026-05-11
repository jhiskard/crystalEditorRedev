#include "surrounding_atom_manager.h"

#include "../../build/periodic_table/periodic_table.h"

#include <algorithm>
#include <array>

namespace features::edit::atoms {
namespace {

std::vector<int> BuildAxisShifts(float fractionalCoord, float threshold) {
    std::vector<int> shifts = {0};
    if (fractionalCoord <= threshold) {
        shifts.push_back(1);
    }
    if (fractionalCoord >= (1.0f - threshold)) {
        shifts.push_back(-1);
    }
    return shifts;
}

} // namespace

SurroundingAtomManager::SurroundingAtomManager(core::scene::SceneState& scene, cell::CellManager& cellManager)
    : scene_(scene)
    , cellManager_(cellManager) {
}

void SurroundingAtomManager::Recompute(int32_t structureId) {
    int32_t sid = structureId;
    if (sid < 0) {
        sid = scene_.currentStructureId;
    }
    if (sid < 0 || !scene_.structures.Exists(sid)) {
        return;
    }

    ClearGenerated(sid);

    if (!enabled_) {
        scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{sid});
        return;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end() ||
        !recordIt->second.cell.hasCell ||
        !recordIt->second.cell.visible) {
        scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{sid});
        return;
    }

    core::scene::StructureRecord& record = recordIt->second;
    constexpr float kBoundaryThreshold = 0.1f;

    std::vector<core::scene::AtomRecord> generatedAtoms;
    generatedAtoms.reserve(record.atoms.size() * 8);

    for (const auto& atom : record.atoms) {
        if (atom.group == "Boundary" || atom.id == 0) {
            continue;
        }

        const std::array<float, 3> frac =
            features::build::periodic_table::CartesianToFractional(atom.cartesian, record.cell.matrix);
        const auto xs = BuildAxisShifts(frac[0], kBoundaryThreshold);
        const auto ys = BuildAxisShifts(frac[1], kBoundaryThreshold);
        const auto zs = BuildAxisShifts(frac[2], kBoundaryThreshold);

        for (int dx : xs) {
            for (int dy : ys) {
                for (int dz : zs) {
                    if (dx == 0 && dy == 0 && dz == 0) {
                        continue;
                    }

                    core::scene::AtomRecord image = atom;
                    image.id = scene_.nextAtomId++;
                    image.group = "Boundary";
                    image.selected = false;
                    image.fractional = {
                        frac[0] + static_cast<float>(dx),
                        frac[1] + static_cast<float>(dy),
                        frac[2] + static_cast<float>(dz),
                    };
                    image.cartesian =
                        features::build::periodic_table::FractionalToCartesian(image.fractional, record.cell.matrix);
                    generatedAtoms.push_back(image);
                }
            }
        }
    }

    auto& generatedIds = generatedAtomIdsByStructure_[sid];
    generatedIds.reserve(generatedAtoms.size());
    for (const auto& atom : generatedAtoms) {
        generatedIds.push_back(atom.id);
    }

    if (!generatedAtoms.empty()) {
        record.atoms.insert(record.atoms.end(), generatedAtoms.begin(), generatedAtoms.end());
    }

    scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{sid});
}

void SurroundingAtomManager::SetEnabled(bool enabled) {
    if (enabled_ == enabled) {
        return;
    }
    enabled_ = enabled;
}

void SurroundingAtomManager::ClearGenerated(int32_t structureId) {
    auto recordIt = scene_.structureRecords.find(structureId);
    if (recordIt == scene_.structureRecords.end()) {
        generatedAtomIdsByStructure_.erase(structureId);
        return;
    }

    auto idsIt = generatedAtomIdsByStructure_.find(structureId);
    if (idsIt == generatedAtomIdsByStructure_.end()) {
        // Fallback cleanup if ids were not tracked.
        auto& atoms = recordIt->second.atoms;
        atoms.erase(
            std::remove_if(atoms.begin(), atoms.end(), [](const core::scene::AtomRecord& atom) {
                return atom.group == "Boundary";
            }),
            atoms.end());
        return;
    }

    const std::vector<uint32_t>& ids = idsIt->second;
    auto& atoms = recordIt->second.atoms;
    atoms.erase(
        std::remove_if(atoms.begin(), atoms.end(), [&ids](const core::scene::AtomRecord& atom) {
            return std::find(ids.begin(), ids.end(), atom.id) != ids.end();
        }),
        atoms.end());
    generatedAtomIdsByStructure_.erase(idsIt);
}

} // namespace features::edit::atoms
