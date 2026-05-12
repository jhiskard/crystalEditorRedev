/**
 * @file features/measurement/measurement_overlay_ui.h
 * @brief ImGui overlay and list panel for measurements.
 */
#pragma once

namespace features::measurement {

class MeasurementController;
class MeasurementStore;

class MeasurementOverlayUI {
public:
    void Render(MeasurementController& controller, MeasurementStore& store, bool* showListWindow);

private:
    void RenderModeOverlay(MeasurementController& controller);
    void RenderListWindow(MeasurementStore& store, bool* showListWindow);
};

} // namespace features::measurement
