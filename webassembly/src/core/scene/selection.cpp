#include "selection.h"

namespace core::scene {

void SelectionSet::SetEventBus(EventBus* eventBus) {
    eventBus_ = eventBus;
}

void SelectionSet::AddAtom(uint32_t atomId) {
    if (atoms_.insert(atomId).second) {
        notifySelectionChanged();
    }
}

void SelectionSet::RemoveAtom(uint32_t atomId) {
    if (atoms_.erase(atomId) > 0) {
        notifySelectionChanged();
    }
}

void SelectionSet::Clear() {
    if (atoms_.empty() && bonds_.empty()) {
        return;
    }
    atoms_.clear();
    bonds_.clear();
    notifySelectionChanged();
}

bool SelectionSet::ContainsAtom(uint32_t atomId) const {
    return atoms_.find(atomId) != atoms_.end();
}

const std::unordered_set<uint32_t>& SelectionSet::Atoms() const {
    return atoms_;
}

void SelectionSet::AddBond(uint32_t bondId) {
    if (bonds_.insert(bondId).second) {
        notifySelectionChanged();
    }
}

void SelectionSet::RemoveBond(uint32_t bondId) {
    if (bonds_.erase(bondId) > 0) {
        notifySelectionChanged();
    }
}

bool SelectionSet::ContainsBond(uint32_t bondId) const {
    return bonds_.find(bondId) != bonds_.end();
}

const std::unordered_set<uint32_t>& SelectionSet::Bonds() const {
    return bonds_;
}

void SelectionSet::notifySelectionChanged() const {
    if (eventBus_ != nullptr) {
        eventBus_->onSelectionChanged.Emit(SelectionChangedEvent{});
    }
}

} // namespace core::scene
