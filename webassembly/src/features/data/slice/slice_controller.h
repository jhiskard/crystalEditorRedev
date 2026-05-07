#pragma once

#include "../charge_density/charge_density.h"

#include "slice_renderer.h"

#include "core/data/colormap.h"
#include "core/scene/scene_state.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace features::data::slice {

class SliceController {
public:
    explicit SliceController(core::scene::SceneState& scene);

    void Show();
    void Hide();
    void Clear();

    bool LoadFromFile(const std::string& filePath);
    bool LoadSampleData();
    void ClearData();
    void SetData(std::unique_ptr<features::data::charge_density::ChargeDensity> data);

    bool HasData() const { return data_ != nullptr; }

    void SetPlane(SlicePlane plane);
    void SetPosition(float position);
    void SetMillerIndices(int h, int k, int l);
    void SetColorMap(core::data::ColorMapPreset preset);
    void SetValueRange(float minValue, float maxValue);
    void SetColorCurve(float midpoint, float sharpness);
    void SetQuality(int qualityIndex);

    SlicePlane Plane() const { return plane_; }
    float Position() const { return position_; }
    int MillerH() const { return millerH_; }
    int MillerK() const { return millerK_; }
    int MillerL() const { return millerL_; }
    core::data::ColorMapPreset ColorMap() const { return colorMap_; }
    float ValueMin() const { return valueMin_; }
    float ValueMax() const { return valueMax_; }
    float ColorCurveMidpoint() const { return colorCurveMidpoint_; }
    float ColorCurveSharpness() const { return colorCurveSharpness_; }
    int Quality() const { return qualityIndex_; }
    float DataMin() const { return dataMin_; }
    float DataMax() const { return dataMax_; }
    std::string StatusMessage() const { return statusMessage_; }
    bool IsVisible() const { return visible_; }
    std::array<int, 3> GridShape() const;

private:
    void RenderCurrent();
    void handleStructureRemoved(const core::scene::StructureRemovedEvent& event);

    core::scene::SceneState& scene_;
    std::unique_ptr<features::data::charge_density::ChargeDensity> data_;
    SliceRenderer renderer_;
    bool visible_ = false;
    int32_t boundStructureId_ = -1;

    SlicePlane plane_ = SlicePlane::XY;
    float position_ = 0.5f;
    int millerH_ = 1;
    int millerK_ = 1;
    int millerL_ = 1;
    core::data::ColorMapPreset colorMap_ = core::data::ColorMapPreset::Viridis;
    float dataMin_ = 0.0f;
    float dataMax_ = 1.0f;
    float valueMin_ = 0.0f;
    float valueMax_ = 1.0f;
    float colorCurveMidpoint_ = 0.5f;
    float colorCurveSharpness_ = 0.0f;
    int qualityIndex_ = 1;
    std::string statusMessage_;
};

} // namespace features::data::slice
