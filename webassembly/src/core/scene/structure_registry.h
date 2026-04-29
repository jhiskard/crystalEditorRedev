#pragma once

#include "events.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace core::scene {

struct StructureEntry {
    int32_t id = -1;
    std::string name;
    bool visible = true;
};

class StructureRegistry {
public:
    explicit StructureRegistry(EventBus* eventBus = nullptr);

    void SetEventBus(EventBus* eventBus);

    bool Register(int32_t id, std::string name);
    bool Remove(int32_t id);

    bool Exists(int32_t id) const;
    bool IsVisible(int32_t id) const;
    bool SetVisible(int32_t id, bool visible);

    const std::unordered_map<int32_t, StructureEntry>& List() const;
    bool Empty() const;
    void Clear();

private:
    std::unordered_map<int32_t, StructureEntry> entries_;
    EventBus* eventBus_ = nullptr;
};

} // namespace core::scene
