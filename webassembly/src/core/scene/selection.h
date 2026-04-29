#pragma once

#include "events.h"

#include <cstdint>
#include <unordered_set>

namespace core::scene {

class SelectionSet {
public:
    void SetEventBus(EventBus* eventBus);

    void AddAtom(uint32_t atomId);
    void RemoveAtom(uint32_t atomId);
    void Clear();
    bool ContainsAtom(uint32_t atomId) const;
    const std::unordered_set<uint32_t>& Atoms() const;

    void AddBond(uint32_t bondId);
    void RemoveBond(uint32_t bondId);
    bool ContainsBond(uint32_t bondId) const;
    const std::unordered_set<uint32_t>& Bonds() const;

private:
    void notifySelectionChanged() const;

    std::unordered_set<uint32_t> atoms_;
    std::unordered_set<uint32_t> bonds_;
    EventBus* eventBus_ = nullptr;
};

} // namespace core::scene
