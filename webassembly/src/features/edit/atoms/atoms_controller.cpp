#include "atoms_controller.h"

#include "../../build/periodic_table/periodic_table.h"
#include "core/vtk/vtk_viewer.h"

#include <vtkMath.h>
#include <vtkRenderer.h>

#include <algorithm>
#include <cmath>
#include <unordered_set>

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

    scene_.events.onAtomPicked.Subscribe([this](const core::scene::AtomPickedEvent& event) {
        HandleAtomPicked(event);
    });
    scene_.events.onEmptyClick.Subscribe([this](const core::scene::EmptyClickEvent& event) {
        HandleEmptyClick(event);
    });
    scene_.events.onDragSelection.Subscribe([this](const core::scene::DragSelectionEvent& event) {
        HandleDragSelection(event);
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

const core::scene::AtomRecord* AtomsController::ResolvePickedAtom(const core::scene::AtomPickedEvent& event) const {
    const core::scene::StructureRecord* record = ResolveActiveRecordConst();
    if (record == nullptr || event.structureId != ActiveStructureId()) {
        return nullptr;
    }

    const core::scene::AtomRecord* closest = nullptr;
    double closestDistance2 = 0.0;
    for (const core::scene::AtomRecord& atom : record->atoms) {
        if (!atom.visible) {
            continue;
        }

        const double dx = static_cast<double>(atom.cartesian[0]) - event.pickPosition[0];
        const double dy = static_cast<double>(atom.cartesian[1]) - event.pickPosition[1];
        const double dz = static_cast<double>(atom.cartesian[2]) - event.pickPosition[2];
        const double distance2 = dx * dx + dy * dy + dz * dz;
        if (closest == nullptr || distance2 < closestDistance2) {
            closest = &atom;
            closestDistance2 = distance2;
        }
    }
    return closest;
}

std::unordered_set<uint32_t> AtomsController::CollectAtomsInRect(const core::scene::DragSelectionEvent& event) const {
    std::unordered_set<uint32_t> selectedIds;
    const core::scene::StructureRecord* record = ResolveActiveRecordConst();
    vtkRenderer* renderer = core::vtk::VtkViewer::Instance().GetRenderer();
    if (record == nullptr || renderer == nullptr || event.structureId != ActiveStructureId()) {
        return selectedIds;
    }

    selectedIds.reserve(record->atoms.size());

    const int minX = std::min(event.x0, event.x1);
    const int maxX = std::max(event.x0, event.x1);
    const int minY = std::min(event.y0, event.y1);
    const int maxY = std::max(event.y0, event.y1);

    for (const core::scene::AtomRecord& atom : record->atoms) {
        if (!atom.visible) {
            continue;
        }

        renderer->SetWorldPoint(
            static_cast<double>(atom.cartesian[0]),
            static_cast<double>(atom.cartesian[1]),
            static_cast<double>(atom.cartesian[2]),
            1.0);
        renderer->WorldToDisplay();
        double display[3] = {0.0, 0.0, 0.0};
        renderer->GetDisplayPoint(display);
        if (!std::isfinite(display[0]) || !std::isfinite(display[1]) || !std::isfinite(display[2]) ||
            display[2] < 0.0 || display[2] > 1.0) {
            continue;
        }

        const int x = static_cast<int>(std::round(display[0]));
        const int y = static_cast<int>(std::round(display[1]));
        if (x >= minX && x <= maxX && y >= minY && y <= maxY) {
            selectedIds.insert(atom.id);
        }
    }

    return selectedIds;
}

void AtomsController::SelectSameElement(int32_t structureId, const std::string& symbol) {
    auto recordIt = scene_.structureRecords.find(structureId);
    if (recordIt == scene_.structureRecords.end()) {
        return;
    }

    std::unordered_set<uint32_t> selectedIds;
    selectedIds.reserve(recordIt->second.atoms.size());
    for (const core::scene::AtomRecord& atom : recordIt->second.atoms) {
        if (atom.visible && atom.symbol == symbol) {
            selectedIds.insert(atom.id);
        }
    }
    manager_.SetSelectionByIds(structureId, selectedIds, false);
}

void AtomsController::HandleAtomPicked(const core::scene::AtomPickedEvent& event) {
    if (!event.selectionModifier || event.structureId != ActiveStructureId()) {
        return;
    }

    const core::scene::AtomRecord* atom = ResolvePickedAtom(event);
    if (atom == nullptr) {
        return;
    }

    if (event.doubleClick) {
        SelectSameElement(event.structureId, atom->symbol);
        return;
    }

    manager_.SetSelectionByIds(event.structureId, std::unordered_set<uint32_t>{atom->id}, false);
}

void AtomsController::HandleEmptyClick(const core::scene::EmptyClickEvent& event) {
    if (event.structureId != ActiveStructureId()) {
        return;
    }
    manager_.SelectNone(event.structureId);
}

void AtomsController::HandleDragSelection(const core::scene::DragSelectionEvent& event) {
    if ((!event.selectionModifier && !event.additive) || event.structureId != ActiveStructureId()) {
        return;
    }

    manager_.SetSelectionByIds(event.structureId, CollectAtomsInRect(event), event.additive);
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
