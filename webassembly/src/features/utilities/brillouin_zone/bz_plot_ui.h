#pragma once

#include <string>

namespace features::utilities::bz {

class BZPlotController;

class BZPlotUI {
public:
    explicit BZPlotUI(BZPlotController& controller);

    void Render(bool* open);

private:
    void RenderBandpathConfig();
    void RenderOptions();
    void RenderActionButtons();
    void RenderStatus();
    void RenderSpecialPointsTable();

    BZPlotController& controller_;
    char pathInput_[128] = "All";
    int npointsInput_ = 50;
    bool showVectors_ = true;
    bool showLabels_ = true;
    std::string lastErrorMessage_;
};

} // namespace features::utilities::bz
