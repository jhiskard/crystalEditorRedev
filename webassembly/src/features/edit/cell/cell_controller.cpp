#include "cell_controller.h"

#include "../../build/periodic_table/periodic_table.h"

#include <algorithm>
#include <string>

namespace features::edit::cell {

CellController::CellController(core::scene::SceneState& scene)
    : scene_(scene)
    , manager_(scene)
    , renderer_(scene) {
}

void CellController::Subscribe() {
    renderer_.Subscribe();
}

int32_t CellController::ActiveStructureId() const {
    const int32_t sid = scene_.currentStructureId;
    if (sid < 0 || !scene_.structures.Exists(sid)) {
        return -1;
    }
    return sid;
}

int32_t CellController::EnsureActiveStructure() {
    if (scene_.currentStructureId >= 0 && scene_.structures.Exists(scene_.currentStructureId)) {
        return scene_.currentStructureId;
    }

    int32_t nextId = 0;
    for (const auto& entry : scene_.structures.List()) {
        nextId = std::max(nextId, entry.first + 1);
    }

    if (!scene_.structures.Register(nextId, "Structure " + std::to_string(nextId + 1))) {
        while (scene_.structures.Exists(nextId)) {
            ++nextId;
        }
        scene_.structures.Register(nextId, "Structure " + std::to_string(nextId + 1));
    }

    scene_.currentStructureId = nextId;
    if (scene_.structureRecords.find(nextId) == scene_.structureRecords.end()) {
        scene_.structureRecords[nextId] = core::scene::StructureRecord{};
    }
    return nextId;
}

bool CellController::HasActiveUnitCell() const {
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return false;
    }

    const auto it = scene_.structureRecords.find(sid);
    return it != scene_.structureRecords.end() && it->second.cell.hasCell;
}

std::array<std::array<float, 3>, 3> CellController::GetActiveMatrix() const {
    return manager_.GetMatrix(-1);
}

void CellController::ApplyMatrix(const std::array<std::array<float, 3>, 3>& matrix, bool preserveCartesian) {
    const int32_t sid = EnsureActiveStructure();
    if (sid < 0) {
        return;
    }

    manager_.SetMatrix(sid, matrix);

    core::scene::StructureRecord& record = scene_.structureRecords[sid];
    for (auto& atom : record.atoms) {
        if (preserveCartesian) {
            atom.fractional = features::build::periodic_table::CartesianToFractional(atom.cartesian, matrix);
        } else {
            atom.cartesian = features::build::periodic_table::FractionalToCartesian(atom.fractional, matrix);
            atom.fractional = features::build::periodic_table::CartesianToFractional(atom.cartesian, matrix);
        }
    }

    scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{sid});
}

void CellController::SetActiveMatrixParameters(
    float a,
    float b,
    float c,
    float alpha,
    float beta,
    float gamma,
    bool preserveCartesian) {
    const int32_t sid = EnsureActiveStructure();
    if (sid < 0) {
        return;
    }

    manager_.SetParameters(sid, a, b, c, alpha, beta, gamma);
    const auto matrix = manager_.GetMatrix(sid);

    core::scene::StructureRecord& record = scene_.structureRecords[sid];
    for (auto& atom : record.atoms) {
        if (preserveCartesian) {
            atom.fractional = features::build::periodic_table::CartesianToFractional(atom.cartesian, matrix);
        } else {
            atom.cartesian = features::build::periodic_table::FractionalToCartesian(atom.fractional, matrix);
            atom.fractional = features::build::periodic_table::CartesianToFractional(atom.cartesian, matrix);
        }
    }

    scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{sid});
}

void CellController::GetActiveMatrixParameters(
    float& a,
    float& b,
    float& c,
    float& alpha,
    float& beta,
    float& gamma) const {
    manager_.GetParameters(-1, a, b, c, alpha, beta, gamma);
}

float CellController::ActiveCellVolume() const {
    return manager_.Volume(-1);
}

void CellController::AlignAxis(int axis) {
    const int32_t sid = EnsureActiveStructure();
    if (sid < 0) {
        return;
    }

    manager_.AlignAxis(sid, axis);

    const auto it = scene_.structureRecords.find(sid);
    if (it == scene_.structureRecords.end()) {
        return;
    }

    const auto matrix = it->second.cell.matrix;
    for (auto& atom : it->second.atoms) {
        atom.fractional = features::build::periodic_table::CartesianToFractional(atom.cartesian, matrix);
    }
    scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{sid});
}

void CellController::SetUnitCellVisible(bool visible) {
    for (auto& [structureId, record] : scene_.structureRecords) {
        (void)structureId;
        record.cell.visible = visible;
    }
    renderer_.SetUnitCellVisible(visible);
}

bool CellController::IsUnitCellVisible() const {
    return renderer_.IsUnitCellVisible(-1);
}

bool CellController::HasUnitCell() const {
    return renderer_.HasUnitCell(-1);
}

} // namespace features::edit::cell
