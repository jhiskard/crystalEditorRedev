#pragma once

namespace features::data::slice {
class SliceController;
}

namespace features::data::charge_density {

class ChargeDensityController;

class ChargeDensityUI {
public:
    explicit ChargeDensityUI(ChargeDensityController& controller,
                             features::data::slice::SliceController* sliceController = nullptr);
    void Render(bool* open);

private:
    void SyncFromController();
    void RenderFileSection();
    void RenderInfoSection() const;
    void RenderModeSection();
    void RenderIsosurfaceControls();
    void RenderSharedOptionsTable();
    void RenderSurfaceOptions();
    void RenderVolumetricOptions();
    void ApplySharedSettings();
    void ApplySurfaceSettings();
    void ApplyVolumetricSettings();
    void SyncSliceSharedSettings() const;
    void UpdateAnimation();

    ChargeDensityController& controller_;
    features::data::slice::SliceController* sliceController_ = nullptr;
    char filePathInput_[256] = "CHGCAR";

    int modeIndex_ = 0;
    bool showIsosurface_ = true;
    bool showMultipleIsosurfaces_ = false;
    float isoValueInput_ = 0.05f;
    float isoColorInput_[4] = {0.0f, 0.5f, 1.0f, 0.7f};
    float positiveIsoValueInput_ = 0.2f;
    float positiveIsoColorInput_[4] = {1.0f, 0.0f, 0.0f, 0.6f};
    float negativeIsoValueInput_ = -0.2f;
    float negativeIsoColorInput_[4] = {0.0f, 0.0f, 1.0f, 0.6f};
    bool wireframeInput_ = false;

    bool volumeVisibilityInput_ = true;
    bool directApplyInput_ = true;
    int colorMapIndex_ = 0;
    float colorCurveMidpointInput_ = 0.5f;
    float colorCurveSharpnessInput_ = 0.0f;
    float windowInput_ = 1.0f;
    float levelInput_ = 0.5f;
    int qualityIndexInput_ = 1;

    float surfaceColorInput_[3] = {0.0f, 0.5f, 1.0f};
    float surfaceOpacityInput_ = 0.35f;
    float surfaceEdgeColorInput_[3] = {0.0f, 0.0f, 0.0f};

    int sampleDistanceIndexInput_ = 2;
    float volumeOpacityMinInput_ = 0.0f;
    float volumeOpacityMaxInput_ = 0.35f;

    int activeDataIndex_ = -1;

    bool animate_ = false;
    float animateSpeed_ = 0.35f;

    bool hadData_ = false;
    bool sharedSettingsDirty_ = false;
    bool surfaceSettingsDirty_ = false;
    bool volumetricSettingsDirty_ = false;
};

} // namespace features::data::charge_density
