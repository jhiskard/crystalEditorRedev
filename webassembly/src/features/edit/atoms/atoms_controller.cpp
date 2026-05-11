#include "atoms_controller.h"

#include "../../build/periodic_table/periodic_table.h"
#include "core/vtk/vtk_viewer.h"

#include <algorithm>

namespace features::edit::atoms {

AtomsController::AtomsController(core::scene::SceneState& scene, cell::CellManager& cellManager)
    : scene_(scene)
    , cellManager_(cellManager)
    , manager_(scene)
    , surroundingManager_(scene, cellManager)
    , renderer_(scene) {
}

void AtomsController::SubscribeEvents() {
    if (subscribedEvents_) {
        return;
    }
    subscribedEvents_ = true;

    renderer_.Subscribe();

    scene_.events.onStructureAdded.Subscribe([this](const core::scene::StructureAddedEvent& event) {
        if (event.structureId >= 0 && scene_.structureRecords.find(event.structureId) == scene_.structureRecords.end()) {
            scene_.structureRecords[event.structureId] = core::scene::StructureRecord{};
        }
        SyncMouseInteractorStructure();
    });

    scene_.events.onCellChanged.Subscribe([this](const core::scene::CellChangedEvent& event) {
        manager_.OnCellChanged(event.structureId, false);
        if (surroundingManager_.IsEnabled()) {
            surroundingManager_.Recompute(event.structureId);
        }
    });

    scene_.events.onSelectionChanged.Subscribe([this](const core::scene::SelectionChangedEvent&) {
        SyncSelectionFlagsFromSelectionSet();
    });
}

void AtomsController::Subscribe(core::vtk::MouseInteractor& mouseInteractor) {
    mouseInteractor_ = &mouseInteractor;
    mouseInteractor_->SetEventBus(&scene_.events);
    mouseInteractor_->SetRenderRequestHandler([]() {
        core::vtk::VtkViewer::Instance().RequestRender();
    });
    SyncMouseInteractorStructure();
}

int32_t AtomsController::ActiveStructureId() const {
    if (scene_.currentStructureId < 0 || !scene_.structures.Exists(scene_.currentStructureId)) {
        return -1;
    }
    return scene_.currentStructureId;
}

int32_t AtomsController::EnsureActiveStructure() {
    if (scene_.currentStructureId >= 0 && scene_.structures.Exists(scene_.currentStructureId)) {
        if (scene_.structureRecords.find(scene_.currentStructureId) == scene_.structureRecords.end()) {
            scene_.structureRecords[scene_.currentStructureId] = core::scene::StructureRecord{};
        }
        SyncMouseInteractorStructure();
        return scene_.currentStructureId;
    }

    int32_t nextId = 0;
    for (const auto& [id, meta] : scene_.structures.List()) {
        (void)meta;
        nextId = std::max(nextId, id + 1);
    }

    if (!scene_.structures.Register(nextId, "Structure " + std::to_string(nextId + 1))) {
        while (scene_.structures.Exists(nextId)) {
            ++nextId;
        }
        scene_.structures.Register(nextId, "Structure " + std::to_string(nextId + 1));
    }

    scene_.currentStructureId = nextId;
    scene_.structureRecords[nextId] = core::scene::StructureRecord{};
    scene_.events.onStructureAdded.Emit(core::scene::StructureAddedEvent{nextId});
    SyncMouseInteractorStructure();
    return nextId;
}

const std::vector<core::scene::AtomRecord>* AtomsController::ActiveAtoms() const {
    const core::scene::StructureRecord* record = ResolveActiveRecordConst();
    if (record == nullptr) {
        return nullptr;
    }
    return &record->atoms;
}

std::vector<core::scene::AtomRecord>* AtomsController::ActiveAtomsMutable() {
    core::scene::StructureRecord* record = ResolveActiveRecord();
    if (record == nullptr) {
        return nullptr;
    }
    return &record->atoms;
}

size_t AtomsController::ActiveAtomCount() const {
    const auto* atoms = ActiveAtoms();
    return (atoms != nullptr) ? atoms->size() : 0u;
}

const core::scene::AtomRecord* AtomsController::AtomAt(size_t index) const {
    const auto* atoms = ActiveAtoms();
    if (atoms == nullptr || index >= atoms->size()) {
        return nullptr;
    }
    return &(*atoms)[index];
}

int AtomsController::AddAtom(const std::string& symbol) {
    return AddAtomAt(symbol, {0.0f, 0.0f, 0.0f}, false);
}

int AtomsController::AddAtomAt(
    const std::string& symbol,
    const std::array<float, 3>& position,
    bool fractionalPosition) {
    const int32_t sid = EnsureActiveStructure();
    if (sid < 0) {
        return -1;
    }

    std::array<float, 3> cartesian = position;
    if (fractionalPosition) {
        core::scene::StructureRecord* record = ResolveActiveRecord();
        if (record != nullptr && record->cell.hasCell) {
            cartesian = features::build::periodic_table::FractionalToCartesian(position, record->cell.matrix);
        }
    }

    const int createdId = manager_.Add(sid, symbol, cartesian);
    if (createdId >= 0 && surroundingManager_.IsEnabled()) {
        surroundingManager_.Recompute(sid);
    }
    return createdId;
}

bool AtomsController::RemoveAtomByIndex(size_t index) {
    const auto* atom = AtomAt(index);
    if (atom == nullptr) {
        return false;
    }
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return false;
    }
    const bool removed = manager_.Remove(sid, atom->id);
    if (removed && surroundingManager_.IsEnabled()) {
        surroundingManager_.Recompute(sid);
    }
    return removed;
}

int AtomsController::RemoveSelectedAtoms() {
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return 0;
    }
    const int removed = manager_.RemoveSelected(sid);
    if (removed > 0 && surroundingManager_.IsEnabled()) {
        surroundingManager_.Recompute(sid);
    }
    return removed;
}

bool AtomsController::SetSelectedByIndex(size_t index, bool selected) {
    const auto* atom = AtomAt(index);
    if (atom == nullptr) {
        return false;
    }
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return false;
    }
    return manager_.SetSelected(sid, atom->id, selected);
}

bool AtomsController::SetVisibleByIndex(size_t index, bool visible) {
    const auto* atom = AtomAt(index);
    if (atom == nullptr) {
        return false;
    }
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return false;
    }
    return manager_.SetVisible(sid, atom->id, visible);
}

bool AtomsController::SetSymbolByIndex(size_t index, const std::string& symbol) {
    const auto* atom = AtomAt(index);
    if (atom == nullptr) {
        return false;
    }
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return false;
    }
    const bool changed = manager_.SetSymbol(sid, atom->id, symbol);
    if (changed && surroundingManager_.IsEnabled()) {
        surroundingManager_.Recompute(sid);
    }
    return changed;
}

bool AtomsController::SetGroupByIndex(size_t index, const std::string& group) {
    const auto* atom = AtomAt(index);
    if (atom == nullptr) {
        return false;
    }
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return false;
    }
    return manager_.SetGroup(sid, atom->id, group);
}

bool AtomsController::SetRadiusByIndex(size_t index, float radius) {
    const auto* atom = AtomAt(index);
    if (atom == nullptr) {
        return false;
    }
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return false;
    }
    const bool changed = manager_.SetRadius(sid, atom->id, radius);
    if (changed && surroundingManager_.IsEnabled()) {
        surroundingManager_.Recompute(sid);
    }
    return changed;
}

bool AtomsController::SetPositionByIndex(size_t index, const std::array<float, 3>& value, bool fractionalInput) {
    const auto* atom = AtomAt(index);
    if (atom == nullptr) {
        return false;
    }
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return false;
    }

    bool changed = false;
    if (fractionalInput) {
        changed = manager_.MoveFractional(sid, atom->id, value);
    } else {
        changed = manager_.Move(sid, atom->id, value);
    }
    if (changed && surroundingManager_.IsEnabled()) {
        surroundingManager_.Recompute(sid);
    }
    return changed;
}

void AtomsController::SelectAll() {
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return;
    }
    manager_.SelectAll(sid);
}

void AtomsController::SelectNone() {
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return;
    }
    manager_.SelectNone(sid);
}

void AtomsController::InvertSelection() {
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return;
    }
    manager_.InvertSelection(sid);
}

int AtomsController::TranslateSelected(const std::array<float, 3>& delta, bool fractionalSpace) {
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return 0;
    }
    const int changed = manager_.TranslateSelected(sid, delta, fractionalSpace);
    if (changed > 0 && surroundingManager_.IsEnabled()) {
        surroundingManager_.Recompute(sid);
    }
    return changed;
}

int AtomsController::SetRadiusSelected(float radius) {
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return 0;
    }
    const int changed = manager_.SetRadiusSelected(sid, radius);
    if (changed > 0 && surroundingManager_.IsEnabled()) {
        surroundingManager_.Recompute(sid);
    }
    return changed;
}

void AtomsController::SetBoundaryAtomsEnabled(bool enabled) {
    if (surroundingManager_.IsEnabled() == enabled) {
        return;
    }
    surroundingManager_.SetEnabled(enabled);
    const int32_t sid = ActiveStructureId();
    if (sid >= 0) {
        surroundingManager_.Recompute(sid);
    }
}

bool AtomsController::BoundaryAtomsEnabled() const {
    return surroundingManager_.IsEnabled();
}

core::scene::StructureRecord* AtomsController::ResolveActiveRecord() {
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return nullptr;
    }
    auto it = scene_.structureRecords.find(sid);
    if (it == scene_.structureRecords.end()) {
        return nullptr;
    }
    return &it->second;
}

const core::scene::StructureRecord* AtomsController::ResolveActiveRecordConst() const {
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return nullptr;
    }
    auto it = scene_.structureRecords.find(sid);
    if (it == scene_.structureRecords.end()) {
        return nullptr;
    }
    return &it->second;
}

void AtomsController::SyncMouseInteractorStructure() {
    if (mouseInteractor_ != nullptr) {
        mouseInteractor_->SetActiveStructureId(ActiveStructureId());
    }
}

void AtomsController::SyncSelectionFlagsFromSelectionSet() {
    std::vector<int32_t> changedStructures;

    for (auto& [structureId, record] : scene_.structureRecords) {
        bool changed = false;
        for (auto& atom : record.atoms) {
            const bool selected = scene_.selection.ContainsAtom(atom.id);
            if (atom.selected != selected) {
                atom.selected = selected;
                changed = true;
            }
        }
        if (changed) {
            changedStructures.push_back(structureId);
        }
    }

    for (int32_t sid : changedStructures) {
        scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{sid});
    }
}

} // namespace features::edit::atoms
