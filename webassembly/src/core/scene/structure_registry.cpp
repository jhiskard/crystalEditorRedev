#include "structure_registry.h"

#include <utility>

namespace core::scene {

StructureRegistry::StructureRegistry(EventBus* eventBus)
    : eventBus_(eventBus) {
}

void StructureRegistry::SetEventBus(EventBus* eventBus) {
    eventBus_ = eventBus;
}

bool StructureRegistry::Register(int32_t id, std::string name) {
    if (id < 0 || Exists(id)) {
        return false;
    }

    StructureEntry entry;
    entry.id = id;
    entry.name = std::move(name);
    entry.visible = true;

    entries_.emplace(id, std::move(entry));
    if (eventBus_ != nullptr) {
        eventBus_->onStructureAdded.Emit(StructureAddedEvent{id});
    }
    return true;
}

bool StructureRegistry::Remove(int32_t id) {
    if (entries_.erase(id) == 0) {
        return false;
    }

    if (eventBus_ != nullptr) {
        eventBus_->onStructureRemoved.Emit(StructureRemovedEvent{id});
    }
    return true;
}

bool StructureRegistry::Exists(int32_t id) const {
    return entries_.find(id) != entries_.end();
}

bool StructureRegistry::IsVisible(int32_t id) const {
    const auto it = entries_.find(id);
    if (it == entries_.end()) {
        return false;
    }
    return it->second.visible;
}

bool StructureRegistry::SetVisible(int32_t id, bool visible) {
    const auto it = entries_.find(id);
    if (it == entries_.end()) {
        return false;
    }
    if (it->second.visible == visible) {
        return true;
    }

    it->second.visible = visible;
    if (eventBus_ != nullptr) {
        eventBus_->onStructureVisibilityChanged.Emit(StructureVisibilityChangedEvent{id, visible});
    }
    return true;
}

const std::unordered_map<int32_t, StructureEntry>& StructureRegistry::List() const {
    return entries_;
}

bool StructureRegistry::Empty() const {
    return entries_.empty();
}

void StructureRegistry::Clear() {
    entries_.clear();
}

} // namespace core::scene
