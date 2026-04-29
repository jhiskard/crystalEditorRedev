#pragma once

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace core::scene {

template <typename Event>
class Bus {
public:
    using Handler = std::function<void(const Event&)>;

    void Subscribe(Handler handler) {
        subscribers_.push_back(std::move(handler));
    }

    void Emit(const Event& event) const {
        for (const auto& handler : subscribers_) {
            if (handler) {
                handler(event);
            }
        }
    }

private:
    std::vector<Handler> subscribers_;
};

struct StructureAddedEvent {
    int32_t structureId = -1;
};

struct StructureRemovedEvent {
    int32_t structureId = -1;
};

struct StructureVisibilityChangedEvent {
    int32_t structureId = -1;
    bool visible = true;
};

struct AtomsChangedEvent {
    int32_t structureId = -1;
};

struct BondsChangedEvent {
    int32_t structureId = -1;
};

struct CellChangedEvent {
    int32_t structureId = -1;
};

struct SelectionChangedEvent {
};

class EventBus {
public:
    Bus<StructureAddedEvent> onStructureAdded;
    Bus<StructureRemovedEvent> onStructureRemoved;
    Bus<StructureVisibilityChangedEvent> onStructureVisibilityChanged;
    Bus<AtomsChangedEvent> onAtomsChanged;
    Bus<BondsChangedEvent> onBondsChanged;
    Bus<CellChangedEvent> onCellChanged;
    Bus<SelectionChangedEvent> onSelectionChanged;
};

} // namespace core::scene
