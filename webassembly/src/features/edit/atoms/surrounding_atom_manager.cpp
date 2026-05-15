#include "surrounding_atom_manager.h"

#include "../../build/periodic_table/periodic_table.h"

#include <algorithm>
#include <array>
#include <vector>

namespace features::edit::atoms {
namespace {

using Vector3 = std::array<float, 3>;
using CellMatrix = std::array<std::array<float, 3>, 3>;

struct BoundaryTranslation {
    Vector3 fractionalShift = {0.0f, 0.0f, 0.0f};
    Vector3 cartesianShift = {0.0f, 0.0f, 0.0f};
};

Vector3 Scale(const Vector3& value, float scale) {
    return {value[0] * scale, value[1] * scale, value[2] * scale};
}

Vector3 Add(const Vector3& lhs, const Vector3& rhs) {
    return {lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2]};
}

BoundaryTranslation MakeTranslation(const CellMatrix& matrix, int aShift, int bShift, int cShift) {
    Vector3 cartesian = {0.0f, 0.0f, 0.0f};
    if (aShift != 0) {
        cartesian = Add(cartesian, Scale(matrix[0], static_cast<float>(aShift)));
    }
    if (bShift != 0) {
        cartesian = Add(cartesian, Scale(matrix[1], static_cast<float>(bShift)));
    }
    if (cShift != 0) {
        cartesian = Add(cartesian, Scale(matrix[2], static_cast<float>(cShift)));
    }

    return BoundaryTranslation{
        {static_cast<float>(aShift), static_cast<float>(bShift), static_cast<float>(cShift)},
        cartesian,
    };
}

void PushIf(
    std::vector<BoundaryTranslation>& translations,
    bool condition,
    const CellMatrix& matrix,
    int aShift,
    int bShift,
    int cShift) {
    if (condition) {
        translations.push_back(MakeTranslation(matrix, aShift, bShift, cShift));
    }
}

std::vector<BoundaryTranslation> BuildLegacyBoundaryTranslations(
    const Vector3& fractional,
    const CellMatrix& matrix,
    float threshold) {
    const bool needsAPositive = fractional[0] <= threshold;
    const bool needsANegative = fractional[0] >= (1.0f - threshold);
    const bool needsBPositive = fractional[1] <= threshold;
    const bool needsBNegative = fractional[1] >= (1.0f - threshold);
    const bool needsCPositive = fractional[2] <= threshold;
    const bool needsCNegative = fractional[2] >= (1.0f - threshold);

    std::vector<BoundaryTranslation> translations;
    translations.reserve(26);

    PushIf(translations, needsAPositive, matrix, 1, 0, 0);
    PushIf(translations, needsANegative, matrix, -1, 0, 0);
    PushIf(translations, needsBPositive, matrix, 0, 1, 0);
    PushIf(translations, needsBNegative, matrix, 0, -1, 0);
    PushIf(translations, needsCPositive, matrix, 0, 0, 1);
    PushIf(translations, needsCNegative, matrix, 0, 0, -1);

    PushIf(translations, needsAPositive && needsBPositive, matrix, 1, 1, 0);
    PushIf(translations, needsAPositive && needsBNegative, matrix, 1, -1, 0);
    PushIf(translations, needsANegative && needsBPositive, matrix, -1, 1, 0);
    PushIf(translations, needsANegative && needsBNegative, matrix, -1, -1, 0);

    PushIf(translations, needsBPositive && needsCPositive, matrix, 0, 1, 1);
    PushIf(translations, needsBPositive && needsCNegative, matrix, 0, 1, -1);
    PushIf(translations, needsBNegative && needsCPositive, matrix, 0, -1, 1);
    PushIf(translations, needsBNegative && needsCNegative, matrix, 0, -1, -1);

    PushIf(translations, needsAPositive && needsCPositive, matrix, 1, 0, 1);
    PushIf(translations, needsAPositive && needsCNegative, matrix, 1, 0, -1);
    PushIf(translations, needsANegative && needsCPositive, matrix, -1, 0, 1);
    PushIf(translations, needsANegative && needsCNegative, matrix, -1, 0, -1);

    PushIf(translations, needsAPositive && needsBPositive && needsCPositive, matrix, 1, 1, 1);
    PushIf(translations, needsAPositive && needsBPositive && needsCNegative, matrix, 1, 1, -1);
    PushIf(translations, needsAPositive && needsBNegative && needsCPositive, matrix, 1, -1, 1);
    PushIf(translations, needsAPositive && needsBNegative && needsCNegative, matrix, 1, -1, -1);
    PushIf(translations, needsANegative && needsBPositive && needsCPositive, matrix, -1, 1, 1);
    PushIf(translations, needsANegative && needsBPositive && needsCNegative, matrix, -1, 1, -1);
    PushIf(translations, needsANegative && needsBNegative && needsCPositive, matrix, -1, -1, 1);
    PushIf(translations, needsANegative && needsBNegative && needsCNegative, matrix, -1, -1, -1);

    return translations;
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
    generatedAtoms.reserve(record.atoms.size() * 26);

    for (const auto& atom : record.atoms) {
        if (atom.group == "Boundary" || atom.id == 0) {
            continue;
        }

        const std::array<float, 3> frac =
            features::build::periodic_table::CartesianToFractional(atom.cartesian, record.cell.matrix);
        const auto translations =
            BuildLegacyBoundaryTranslations(frac, record.cell.matrix, kBoundaryThreshold);

        for (const auto& translation : translations) {
            core::scene::AtomRecord image = atom;
            image.id = scene_.nextAtomId++;
            image.group = "Boundary";
            image.selected = false;
            image.fractional = {
                frac[0] + translation.fractionalShift[0],
                frac[1] + translation.fractionalShift[1],
                frac[2] + translation.fractionalShift[2],
            };
            image.cartesian = Add(atom.cartesian, translation.cartesianShift);
            generatedAtoms.push_back(image);
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
