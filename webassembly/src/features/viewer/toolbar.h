#pragma once

#include "viewer_types.h"

#include <imgui.h>

namespace features::viewer {

class Toolbar {
public:
    void Render(const ImVec2& viewerContentMin, const ImVec2& viewerContentMax);
    bool IsHovered() const { return hovered_; }

private:
    void renderBoundaryAtomsButton();
    void renderMeshDisplayModeButton();
    void renderProjectionButton();
    void renderResetViewButton();
    void renderCellAlignButton();
    void renderChargeDensityControls();
    void renderArrowStepControl();

    MeshDisplayMode meshDisplayMode_ = MeshDisplayMode::Solid;
    float chargeDensityLevelPercent_ = 50.0f;
    bool hovered_ = false;
};

} // namespace features::viewer
