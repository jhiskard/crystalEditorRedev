#pragma once

#include "core/data/element_database.h"

#include <array>
#include <string>

#include <imgui.h>

namespace features::build::periodic_table {

class PeriodicTableController;

class PeriodicTableUI {
public:
    explicit PeriodicTableUI(PeriodicTableController& controller);

    void Render(bool* open);

private:
    void RenderCategoryFilter();
    void RenderMainPeriodicTable();
    void RenderLanthanides();
    void RenderActinides();
    void RenderElementDetails();
    void RenderElementButton(const core::data::ElementInfo& element, float buttonSize);
    void RenderElementTooltip(const core::data::ElementInfo& element);

    bool ShouldShowElement(const core::data::ElementInfo& element) const;
    bool MatchesFilter(const std::string& symbol) const;

    PeriodicTableController& controller_;
    char searchBuffer_[64] = {0};
    int category_ = 0;
    std::array<float, 3> cartesianPosition_ = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> fractionalPosition_ = {0.0f, 0.0f, 0.0f};
    bool useFractionalCoords_ = false;
};

} // namespace features::build::periodic_table
