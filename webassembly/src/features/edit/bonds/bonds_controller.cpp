#include "bonds_controller.h"

#include <algorithm>

namespace features::edit::bonds {

BondsController::BondsController(core::scene::SceneState& scene)
    : scene_(scene)
    , manager_(scene)
    , renderer_(scene) {
}

void BondsController::Subscribe() {
    renderer_.Subscribe();
}

void BondsController::SubscribeAtomChanges() {
    if (subscribedAtomChanges_) {
        return;
    }
    subscribedAtomChanges_ = true;

    scene_.events.onAtomsChanged.Subscribe([this](const core::scene::AtomsChangedEvent& event) {
        manager_.RecomputeAll(event.structureId);
    });
    scene_.events.onCellChanged.Subscribe([this](const core::scene::CellChangedEvent& event) {
        manager_.RecomputeAll(event.structureId);
    });
}

int32_t BondsController::ActiveStructureId() const {
    if (scene_.currentStructureId < 0 || !scene_.structures.Exists(scene_.currentStructureId)) {
        return -1;
    }
    return scene_.currentStructureId;
}

int32_t BondsController::EnsureActiveStructure() {
    if (scene_.currentStructureId >= 0 && scene_.structures.Exists(scene_.currentStructureId)) {
        if (scene_.structureRecords.find(scene_.currentStructureId) == scene_.structureRecords.end()) {
            scene_.structureRecords[scene_.currentStructureId] = core::scene::StructureRecord{};
        }
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
    return nextId;
}

void BondsController::RecomputeAll() {
    const int32_t sid = EnsureActiveStructure();
    if (sid < 0) {
        return;
    }
    manager_.RecomputeAll(sid);
}

void BondsController::ClearAll() {
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return;
    }
    manager_.ClearAll(sid);
}

std::vector<std::string> BondsController::ListBondTypes() const {
    return manager_.ListBondTypes(ActiveStructureId());
}

int BondsController::CountBondsByType(const std::string& bondTypeKey) const {
    return manager_.CountBondsByType(ActiveStructureId(), bondTypeKey);
}

int BondsController::ActiveBondCount() const {
    return manager_.ActiveBondCount(ActiveStructureId());
}

void BondsController::SetBondTypeVisible(const std::string& bondTypeKey, bool visible) {
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return;
    }
    manager_.SetBondTypeVisible(sid, bondTypeKey, visible);
}

bool BondsController::IsBondTypeVisible(const std::string& bondTypeKey) const {
    return manager_.IsBondTypeVisible(bondTypeKey);
}

void BondsController::SetBondDistanceThreshold(const std::string& bondTypeKey, float threshold) {
    manager_.SetBondDistanceThreshold(bondTypeKey, threshold);
    RecomputeAll();
}

float BondsController::GetBondDistanceThreshold(const std::string& bondTypeKey) const {
    return manager_.GetBondDistanceThreshold(bondTypeKey);
}

float BondsController::GetDefaultBondDistanceThreshold(const std::string& bondTypeKey) const {
    return manager_.GetDefaultBondDistanceThreshold(bondTypeKey);
}

void BondsController::SetGlobalThickness(float thickness) {
    manager_.SetGlobalThickness(thickness);
    renderer_.UpdateAllBondGroupThickness(manager_.GetGlobalThickness());
}

float BondsController::GetGlobalThickness() const {
    return manager_.GetGlobalThickness();
}

void BondsController::SetGlobalOpacity(float opacity) {
    manager_.SetGlobalOpacity(opacity);
    renderer_.UpdateAllBondGroupOpacity(manager_.GetGlobalOpacity());
}

float BondsController::GetGlobalOpacity() const {
    return manager_.GetGlobalOpacity();
}

} // namespace features::edit::bonds
