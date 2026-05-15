#pragma once

#include <cstdint>
#include <array>
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

struct AtomPickedEvent {
    int32_t structureId = -1;
    std::array<double, 3> pickPosition = {0.0, 0.0, 0.0};
    int screenX = 0;
    int screenY = 0;
    bool selectionModifier = false;
    bool doubleClick = false;
};

struct EmptyClickEvent {
    int32_t structureId = -1;
    int screenX = 0;
    int screenY = 0;
    bool selectionModifier = false;
};

struct DragSelectionEvent {
    int32_t structureId = -1;
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    int viewportHeight = 0;
    bool additive = false;
    bool selectionModifier = false;
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
    Bus<AtomPickedEvent> onAtomPicked;
    Bus<EmptyClickEvent> onEmptyClick;
    Bus<DragSelectionEvent> onDragSelection;
};

} // namespace core::scene
