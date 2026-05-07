#pragma once

#include "charge_density.h"
#include "isosurface_renderer.h"
#include "volume_renderer.h"

#include "core/data/colormap.h"
#include "core/scene/scene_state.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace features::data::charge_density {

enum class Mode {
    None,
    Isosurface,
    Surface,
    Volumetric,
};

class ChargeDensityController {
public:
    explicit ChargeDensityController(core::scene::SceneState& scene);

    void Show(Mode mode);
    void Clear();

    bool LoadFromFile(const std::string& filePath);
    bool LoadSampleData();
    void ClearData();
    void SetData(std::unique_ptr<ChargeDensity> data);

    bool HasData() const { return data_ != nullptr; }
    Mode CurrentMode() const { return mode_; }

    void SetIsoValue(float value);
    void SetWireframe(bool enabled);
    void SetColorMap(core::data::ColorMapPreset preset);
    void SetOpacity(float opacity);
    void SetValueRange(float minValue, float maxValue);
    void SetShowMultipleIsosurfaces(bool enabled);
    void SetPositiveIsoValue(float value);
    void SetNegativeIsoValue(float value);
    void SetPrimaryIsosurfaceVisible(bool visible);
    void SetIsoColor(const std::array<float, 4>& rgba);
    void SetPositiveIsoColor(const std::array<float, 4>& rgba);
    void SetNegativeIsoColor(const std::array<float, 4>& rgba);

    void SetVolumeVisibility(bool visible);
    void SetDirectApply(bool enabled);
    void SetColorCurve(float midpoint, float sharpness);
    void SetWindowLevel(float window, float level);
    void SetQuality(int qualityIndex);
    void SetSurfaceColor(const std::array<float, 3>& rgb);
    void SetSurfaceOpacity(float opacity);
    void SetSurfaceEdgeColor(const std::array<float, 3>& rgb);
    void SetSampleDistanceIndex(int index);
    void SetVolumeOpacityMin(float opacity);
    void SetVolumeOpacityMax(float opacity);

    std::vector<std::string> DataNames() const;
    int ActiveDataIndex() const { return activeDataIndex_; }
    bool SelectDataIndex(int index);

    float IsoValue() const { return isoValue_; }
    bool Wireframe() const { return wireframe_; }
    core::data::ColorMapPreset ColorMap() const { return colorMap_; }
    float Opacity() const { return surfaceOpacity_; }
    float ValueMin() const { return valueMin_; }
    float ValueMax() const { return valueMax_; }
    float DataMin() const { return dataMin_; }
    float DataMax() const { return dataMax_; }
    bool PrimaryIsosurfaceVisible() const { return showPrimaryIsosurface_; }
    const std::array<float, 4>& IsoColor() const { return isoColor_; }
    bool ShowMultipleIsosurfaces() const { return showMultipleIsosurfaces_; }
    float PositiveIsoValue() const { return positiveIsoValue_; }
    float NegativeIsoValue() const { return negativeIsoValue_; }
    const std::array<float, 4>& PositiveIsoColor() const { return positiveIsoColor_; }
    const std::array<float, 4>& NegativeIsoColor() const { return negativeIsoColor_; }

    bool VolumeVisibility() const { return volumeVisibility_; }
    bool DirectApply() const { return directApply_; }
    float ColorCurveMidpoint() const { return colorCurveMidpoint_; }
    float ColorCurveSharpness() const { return colorCurveSharpness_; }
    float Window() const { return window_; }
    float Level() const { return level_; }
    int Quality() const { return qualityIndex_; }
    const std::array<float, 3>& SurfaceColor() const { return surfaceColor_; }
    float SurfaceOpacity() const { return surfaceOpacity_; }
    const std::array<float, 3>& SurfaceEdgeColor() const { return surfaceEdgeColor_; }
    int SampleDistanceIndex() const { return sampleDistanceIndex_; }
    float VolumeOpacityMin() const { return volumeOpacityMin_; }
    float VolumeOpacityMax() const { return volumeOpacityMax_; }

    std::array<int, 3> GridShape() const;
    std::string StatusMessage() const { return statusMessage_; }

private:
    void RenderCurrentMode();
    bool setActiveDataByIndex(int index);
    void pushDataEntry(const std::string& name, std::unique_ptr<ChargeDensity> data);
    void syncWindowLevelToRange();
    void handleStructureRemoved(const core::scene::StructureRemovedEvent& event);

    core::scene::SceneState& scene_;
    std::unique_ptr<ChargeDensity> data_;
    IsosurfaceRenderer isosurfaceRenderer_;
    VolumeRenderer volumeRenderer_;

    Mode mode_ = Mode::None;
    int32_t boundStructureId_ = -1;
    int activeDataIndex_ = -1;
    std::vector<std::pair<std::string, std::unique_ptr<ChargeDensity>>> dataEntries_;

    float isoValue_ = 0.0f;
    std::array<float, 4> isoColor_ = {0.0f, 0.5f, 1.0f, 0.7f};
    bool wireframe_ = false;
    core::data::ColorMapPreset colorMap_ = core::data::ColorMapPreset::Viridis;
    bool showPrimaryIsosurface_ = true;
    float dataMin_ = 0.0f;
    float dataMax_ = 1.0f;
    float valueMin_ = 0.0f;
    float valueMax_ = 1.0f;
    bool showMultipleIsosurfaces_ = false;
    float positiveIsoValue_ = 0.1f;
    float negativeIsoValue_ = -0.1f;
    std::array<float, 4> positiveIsoColor_ = {1.0f, 0.0f, 0.0f, 0.6f};
    std::array<float, 4> negativeIsoColor_ = {0.0f, 0.0f, 1.0f, 0.6f};

    bool volumeVisibility_ = true;
    bool directApply_ = true;
    float colorCurveMidpoint_ = 0.5f;
    float colorCurveSharpness_ = 0.0f;
    float window_ = 1.0f;
    float level_ = 0.5f;
    int qualityIndex_ = 1;
    std::array<float, 3> surfaceColor_ = {0.0f, 0.5f, 1.0f};
    float surfaceOpacity_ = 0.35f;
    std::array<float, 3> surfaceEdgeColor_ = {0.0f, 0.0f, 0.0f};
    int sampleDistanceIndex_ = 2;
    float volumeOpacityMin_ = 0.0f;
    float volumeOpacityMax_ = 0.35f;

    std::string statusMessage_;
};

} // namespace features::data::charge_density
