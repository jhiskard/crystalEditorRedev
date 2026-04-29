#include "batch_update_system.h"

namespace core::vtk {

BatchUpdateSystem::BatchUpdateSystem(core::scene::EventBus* eventBus)
    : eventBus_(eventBus) {
}

void BatchUpdateSystem::SetEventBus(core::scene::EventBus* eventBus) {
    eventBus_ = eventBus;
}

void BatchUpdateSystem::SetFlushHandler(FlushHandler handler) {
    flushHandler_ = std::move(handler);
}

void BatchUpdateSystem::SetActiveStructureId(int32_t structureId) {
    activeStructureId_ = structureId;
}

void BatchUpdateSystem::BeginBatch() {
    if (batchMode_) {
        return;
    }
    batchMode_ = true;
    pendingAtomGroups_.clear();
    pendingBondGroups_.clear();
}

void BatchUpdateSystem::EndBatch() {
    if (!batchMode_) {
        return;
    }

    if (flushHandler_) {
        flushHandler_(pendingAtomGroups_, pendingBondGroups_);
    }

    if (eventBus_ != nullptr) {
        if (!pendingAtomGroups_.empty()) {
            eventBus_->onAtomsChanged.Emit(core::scene::AtomsChangedEvent{activeStructureId_});
        }
        if (!pendingBondGroups_.empty()) {
            eventBus_->onBondsChanged.Emit(core::scene::BondsChangedEvent{activeStructureId_});
        }
    }

    pendingAtomGroups_.clear();
    pendingBondGroups_.clear();
    batchMode_ = false;
}

void BatchUpdateSystem::ForceBatchEnd() {
    pendingAtomGroups_.clear();
    pendingBondGroups_.clear();
    batchMode_ = false;
}

bool BatchUpdateSystem::IsBatchMode() const {
    return batchMode_;
}

void BatchUpdateSystem::ScheduleAtomGroupUpdate(const std::string& symbol) {
    if (batchMode_) {
        pendingAtomGroups_.insert(symbol);
        return;
    }
    flushSingleItem(symbol, std::string{});
}

void BatchUpdateSystem::ScheduleBondGroupUpdate(const std::string& bondKey) {
    if (batchMode_) {
        pendingBondGroups_.insert(bondKey);
        return;
    }
    flushSingleItem(std::string{}, bondKey);
}

void BatchUpdateSystem::flushSingleItem(const std::string& atomGroup,
                                        const std::string& bondGroup) {
    std::set<std::string> atoms;
    std::set<std::string> bonds;
    if (!atomGroup.empty()) {
        atoms.insert(atomGroup);
    }
    if (!bondGroup.empty()) {
        bonds.insert(bondGroup);
    }

    if (flushHandler_) {
        flushHandler_(atoms, bonds);
    }

    if (eventBus_ != nullptr) {
        if (!atoms.empty()) {
            eventBus_->onAtomsChanged.Emit(core::scene::AtomsChangedEvent{activeStructureId_});
        }
        if (!bonds.empty()) {
            eventBus_->onBondsChanged.Emit(core::scene::BondsChangedEvent{activeStructureId_});
        }
    }
}

} // namespace core::vtk
