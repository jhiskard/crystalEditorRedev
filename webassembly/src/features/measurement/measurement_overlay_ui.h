/**
 * @file features/measurement/measurement_overlay_ui.h
 * @brief ImGui overlay and list panel for measurements.
 */
#pragma once

#include "measurement_mode.h"

#include <imgui.h>

namespace features::measurement {

class MeasurementController;
class MeasurementStore;

class MeasurementOverlayUI {
public:
    void Render(MeasurementController& controller, MeasurementStore& store, bool* showListWindow);
    void RenderModeOverlay(
        MeasurementController& controller,
        MeasurementStore& store,
        const ImVec2& viewerContentMin,
        const ImVec2& viewerContentMax);

private:
    void RenderListWindow(MeasurementStore& store, bool* showListWindow);
    void RenderStyleOptions(MeasurementController& controller, MeasurementStore& store);
    bool* StylePanelExpanded(MeasurementMode mode);

    bool distanceStylePanelExpanded_ = false;
    bool angleStylePanelExpanded_ = false;
    bool dihedralStylePanelExpanded_ = false;
    bool geometricCenterStylePanelExpanded_ = false;
    bool centerOfMassStylePanelExpanded_ = false;
};

} // namespace features::measurement
