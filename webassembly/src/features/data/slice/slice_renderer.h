#pragma once

#include "../charge_density/charge_density.h"

#include "core/data/colormap.h"

#include <array>

#include <vtkActor.h>
#include <vtkCutter.h>
#include <vtkPlane.h>
#include <vtkSmartPointer.h>

namespace features::data::slice {

enum class SlicePlane {
    XY = 0,
    XZ = 1,
    YZ = 2,
    Miller = 3,
};

class SliceRenderer {
public:
    void SetPlane(SlicePlane plane, float position);
    void SetMillerIndices(int h, int k, int l);
    void SetColorMap(core::data::ColorMapPreset preset);
    void SetValueRange(float minValue, float maxValue);
    void SetColorCurve(float midpoint, float sharpness);

    void Render(const features::data::charge_density::ChargeDensity& cd);
    void Clear();

private:
    SlicePlane plane_ = SlicePlane::XY;
    float position_ = 0.5f;
    int millerH_ = 1;
    int millerK_ = 1;
    int millerL_ = 1;
    core::data::ColorMapPreset colorMap_ = core::data::ColorMapPreset::Viridis;
    float valueMin_ = 0.0f;
    float valueMax_ = 1.0f;
    float colorMidpoint_ = 0.5f;
    float colorSharpness_ = 0.0f;

    vtkSmartPointer<vtkActor> sliceActor_;
};

} // namespace features::data::slice
