#pragma once

#include <cstddef>

class vtkColorTransferFunction;
class vtkLookupTable;

namespace common {

enum class ColorMapPreset {
    Grayscale = 0,
    Rainbow = 1,
    CoolToWarm = 2,
    Viridis = 3,
    Plasma = 4,
    Count
};

constexpr int kColorMapPresetCount = static_cast<int>(ColorMapPreset::Count);

int ClampColorMapPresetIndex(int index, ColorMapPreset fallback = ColorMapPreset::Grayscale);
ColorMapPreset ColorMapPresetFromIndex(int index, ColorMapPreset fallback = ColorMapPreset::Grayscale);
int ColorMapPresetToIndex(ColorMapPreset preset);

const char* GetColorMapPresetLabel(ColorMapPreset preset);
const char* const* GetColorMapPresetLabels();

void EvaluateColorMap(ColorMapPreset preset, float t, float& r, float& g, float& b);

void ApplyColorMapToLookupTable(vtkLookupTable* lut, ColorMapPreset preset);
void ApplyColorMapToTransferFunction(vtkColorTransferFunction* ctf,
                                     ColorMapPreset preset,
                                     double rangeMin,
                                     double midValue,
                                     double rangeMax,
                                     double midpoint = 0.5,
                                     double sharpness = 0.0);

}  // namespace common
