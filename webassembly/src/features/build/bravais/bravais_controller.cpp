#include "bravais_controller.h"

#include "../periodic_table/periodic_table.h"

#include "core/data/element_database.h"

#include <algorithm>
#include <string>

namespace features::build::bravais {

BravaisController::BravaisController(core::scene::SceneState& scene)
    : scene_(scene) {
    scene_.events.onStructureRemoved.Subscribe([this](const core::scene::StructureRemovedEvent& event) {
        scene_.structureRecords.erase(event.structureId);
        if (scene_.currentStructureId == event.structureId) {
            scene_.currentStructureId = -1;
        }
    });
}

int32_t BravaisController::EnsureActiveStructure() {
    if (scene_.currentStructureId >= 0 && scene_.structures.Exists(scene_.currentStructureId)) {
        return scene_.currentStructureId;
    }

    int32_t nextId = 0;
    for (const auto& entry : scene_.structures.List()) {
        nextId = std::max(nextId, entry.first + 1);
    }

    const std::string name = "Structure " + std::to_string(nextId + 1);
    if (scene_.structures.Register(nextId, name)) {
        scene_.currentStructureId = nextId;
    } else {
        while (scene_.structures.Exists(nextId)) {
            ++nextId;
        }
        scene_.structures.Register(nextId, "Structure " + std::to_string(nextId + 1));
        scene_.currentStructureId = nextId;
    }

    return scene_.currentStructureId;
}

void BravaisController::Apply(
    BravaisLatticeType type,
    const BravaisParameters& params,
    bool preserveExistingAtoms) {
    currentType_ = type;
    params_ = params;
    preserveExistingAtoms_ = preserveExistingAtoms;

    const int32_t structureId = EnsureActiveStructure();
    core::scene::StructureRecord& record = scene_.structureRecords[structureId];

    const bool hadCell = record.cell.hasCell;

    record.cell.hasCell = true;
    record.cell.matrix = CrystalStructureGenerator::ComputeLatticeMatrix(type, params);

    if (!preserveExistingAtoms_) {
        record.atoms.clear();
    }

    for (auto& atom : record.atoms) {
        if (hadCell) {
            // Keep fractional position and remap into the updated lattice.
            atom.cartesian = periodic_table::FractionalToCartesian(atom.fractional, record.cell.matrix);
        }

        if (!hadCell) {
            // First lattice assignment: infer fractional coordinates from existing cartesian positions.
            atom.fractional = periodic_table::CartesianToFractional(atom.cartesian, record.cell.matrix);
        }

        // If old and new cells both exist, refresh fractional to stay numerically stable.
        if (hadCell) {
            atom.fractional = periodic_table::CartesianToFractional(atom.cartesian, record.cell.matrix);
        }
    }

    if (record.atoms.empty()) {
        const auto basisPositions = CrystalStructureGenerator::GenerateAtomPositions(type);
        const core::data::ElementInfo* seed = core::data::ElementDatabase::getInstance().getElementInfo("C");

        for (const auto& fracPos : basisPositions) {
            core::scene::AtomRecord atom;
            atom.id = scene_.nextAtomId++;
            atom.symbol = (seed != nullptr) ? seed->symbol : std::string("C");
            atom.radius = (seed != nullptr) ? std::max(0.001f, seed->covalentRadius) : 1.0f;
            atom.fractional = fracPos;
            atom.cartesian = periodic_table::FractionalToCartesian(fracPos, record.cell.matrix);
            record.atoms.push_back(atom);
        }
    }
    scene_.events.onCellChanged.Emit(core::scene::CellChangedEvent{structureId});
    scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{structureId});
}

CrystalSystem BravaisController::CurrentSystem() const {
    return CrystalSystemMapper::GetCrystalSystem(currentType_);
}

} // namespace features::build::bravais
