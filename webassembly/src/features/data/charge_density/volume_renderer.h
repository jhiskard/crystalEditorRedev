#pragma once

#include "charge_density.h"

#include "core/data/colormap.h"

#include <vtkSmartPointer.h>

class vtkVolume;

namespace features::data::charge_density {

class VolumeRenderer {
public:
    void Render(const ChargeDensity& cd,
                core::data::ColorMapPreset preset,
                float rangeMin,
                float rangeMax,
                float colorMidpoint,
                float colorSharpness,
                int sampleDistanceIndex,
                float opacityMin,
                float opacityMax,
                int qualityIndex);
    void Clear();

private:
    vtkSmartPointer<vtkVolume> volume_;
};

} // namespace features::data::charge_density
