#include "periodic_table_controller.h"

#include <algorithm>

namespace features::build::periodic_table {

namespace {

std::array<std::array<float, 3>, 3> IdentityCell() {
    return {{
        {{1.0f, 0.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, 1.0f}},
    }};
}

} // namespace

PeriodicTableController::PeriodicTableController(core::scene::SceneState& scene)
    : scene_(scene),
      selectedSymbol_("C") {
    scene_.events.onStructureRemoved.Subscribe([this](const core::scene::StructureRemovedEvent& event) {
        scene_.structureRecords.erase(event.structureId);
        if (scene_.currentStructureId == event.structureId) {
            scene_.currentStructureId = -1;
        }
    });
}

const core::data::ElementInfo* PeriodicTableController::SelectedElementInfo() const {
    if (selectedSymbol_.empty()) {
        return nullptr;
    }
    return core::data::ElementDatabase::getInstance().getElementInfo(selectedSymbol_);
}

int32_t PeriodicTableController::EnsureActiveStructure() {
    if (scene_.currentStructureId >= 0 && scene_.structures.Exists(scene_.currentStructureId)) {
        return scene_.currentStructureId;
    }

    int32_t nextId = 0;
    for (const auto& entry : scene_.structures.List()) {
        nextId = std::max(nextId, entry.first + 1);
    }

    const std::string structureName = "Structure " + std::to_string(nextId + 1);
    if (scene_.structures.Register(nextId, structureName)) {
        scene_.currentStructureId = nextId;
    } else {
        // Retry with the first available id.
        while (scene_.structures.Exists(nextId)) {
            ++nextId;
        }
        scene_.structures.Register(nextId, "Structure " + std::to_string(nextId + 1));
        scene_.currentStructureId = nextId;
    }

    return scene_.currentStructureId;
}

core::scene::StructureRecord& PeriodicTableController::EnsureStructureRecord(int32_t structureId) {
    return scene_.structureRecords[structureId];
}

void PeriodicTableController::AddAtom(const std::string& symbol) {
    const std::array<float, 3> defaultPos = ComputeDefaultPosition(symbol);
    AddAtomAt(symbol, defaultPos);
}

void PeriodicTableController::AddAtomAt(const std::string& symbol, const std::array<float, 3>& position) {
    const core::data::ElementInfo* info = core::data::ElementDatabase::getInstance().getElementInfo(symbol);
    if (info == nullptr) {
        return;
    }

    const int32_t structureId = EnsureActiveStructure();
    core::scene::StructureRecord& record = EnsureStructureRecord(structureId);

    core::scene::AtomRecord atom;
    atom.id = scene_.nextAtomId++;
    atom.symbol = symbol;
    atom.radius = std::max(0.001f, info->covalentRadius);
    atom.cartesian = position;

    if (record.cell.hasCell) {
        atom.fractional = CartesianToFractional(position, record.cell.matrix);
    }

    record.atoms.push_back(atom);
    selectedSymbol_ = symbol;

    scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{structureId});
}

void PeriodicTableController::AddAtomAtFractional(
    const std::string& symbol,
    const std::array<float, 3>& fractional) {
    const core::data::ElementInfo* info = core::data::ElementDatabase::getInstance().getElementInfo(symbol);
    if (info == nullptr) {
        return;
    }

    const int32_t structureId = EnsureActiveStructure();
    core::scene::StructureRecord& record = EnsureStructureRecord(structureId);

    core::scene::AtomRecord atom;
    atom.id = scene_.nextAtomId++;
    atom.symbol = symbol;
    atom.radius = std::max(0.001f, info->covalentRadius);

    if (record.cell.hasCell) {
        atom.fractional = fractional;
        atom.cartesian = FractionalToCartesian(fractional, record.cell.matrix);
    } else {
        atom.cartesian = fractional;
    }

    record.atoms.push_back(atom);
    selectedSymbol_ = symbol;

    scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{structureId});
}

bool PeriodicTableController::HasActiveUnitCell() const {
    const int32_t structureId = scene_.currentStructureId;
    if (structureId < 0) {
        return false;
    }

    const auto it = scene_.structureRecords.find(structureId);
    return (it != scene_.structureRecords.end()) && it->second.cell.hasCell;
}

std::array<std::array<float, 3>, 3> PeriodicTableController::ActiveUnitCell() const {
    const int32_t structureId = scene_.currentStructureId;
    if (structureId < 0) {
        return IdentityCell();
    }

    const auto it = scene_.structureRecords.find(structureId);
    if (it == scene_.structureRecords.end() || !it->second.cell.hasCell) {
        return IdentityCell();
    }

    return it->second.cell.matrix;
}

int32_t PeriodicTableController::ActiveStructureId() const {
    return scene_.currentStructureId;
}

size_t PeriodicTableController::ActiveStructureAtomCount() const {
    const int32_t structureId = scene_.currentStructureId;
    if (structureId < 0) {
        return 0;
    }

    const auto it = scene_.structureRecords.find(structureId);
    if (it == scene_.structureRecords.end()) {
        return 0;
    }

    return it->second.atoms.size();
}

} // namespace features::build::periodic_table
