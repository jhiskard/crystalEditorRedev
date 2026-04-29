#pragma once

#include "../scene/events.h"

#include <cstdint>
#include <functional>
#include <set>
#include <string>

namespace core::vtk {

class BatchUpdateSystem {
public:
    using FlushHandler = std::function<void(const std::set<std::string>& atomGroups,
                                            const std::set<std::string>& bondGroups)>;

    explicit BatchUpdateSystem(core::scene::EventBus* eventBus = nullptr);

    void SetEventBus(core::scene::EventBus* eventBus);
    void SetFlushHandler(FlushHandler handler);
    void SetActiveStructureId(int32_t structureId);

    void BeginBatch();
    void EndBatch();
    void ForceBatchEnd();

    bool IsBatchMode() const;

    void ScheduleAtomGroupUpdate(const std::string& symbol);
    void ScheduleBondGroupUpdate(const std::string& bondKey);

private:
    void flushSingleItem(const std::string& atomGroup, const std::string& bondGroup);

    core::scene::EventBus* eventBus_ = nullptr;
    FlushHandler flushHandler_;
    int32_t activeStructureId_ = -1;
    bool batchMode_ = false;
    std::set<std::string> pendingAtomGroups_;
    std::set<std::string> pendingBondGroups_;
};

} // namespace core::vtk
