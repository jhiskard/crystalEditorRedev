#include "colormap.h"

#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace common {
namespace {

struct ColorStop {
    float t;
    float r;
    float g;
    float b;
};

constexpr std::array<const char*, kColorMapPresetCount> kColorMapLabels = {
    "Grayscale",
    "Rainbow",
    "Cool to Warm",
    "Viridis",
    "Plasma"
};

constexpr std::array<ColorStop, 5> kViridisStops = {{
    {0.00f, 0.267004f, 0.004874f, 0.329415f},
    {0.25f, 0.229739f, 0.322361f, 0.545706f},
    {0.50f, 0.127568f, 0.566949f, 0.550556f},
    {0.75f, 0.369214f, 0.788888f, 0.382914f},
    {1.00f, 0.993248f, 0.906157f, 0.143936f}
}};

constexpr std::array<ColorStop, 5> kPlasmaStops = {{
    {0.00f, 0.050383f, 0.029803f, 0.527975f},
    {0.25f, 0.494877f, 0.011990f, 0.657865f},
    {0.50f, 0.798216f, 0.280197f, 0.469538f},
    {0.75f, 0.973416f, 0.585761f, 0.251540f},
    {1.00f, 0.940015f, 0.975158f, 0.131326f}
}};

constexpr std::array<ColorStop, 3> kRainbowStops = {{
    {0.00f, 0.0f, 0.0f, 1.0f},
    {0.50f, 0.0f, 1.0f, 0.0f},
    {1.00f, 1.0f, 0.0f, 0.0f}
}};

constexpr std::array<ColorStop, 3> kCoolToWarmStops = {{
    {0.00f, 0.0f, 0.0f, 1.0f},
    {0.50f, 1.0f, 1.0f, 1.0f},
    {1.00f, 1.0f, 0.0f, 0.0f}
}};

constexpr std::array<ColorStop, 2> kGrayscaleStops = {{
    {0.00f, 0.0f, 0.0f, 0.0f},
    {1.00f, 1.0f, 1.0f, 1.0f}
}};

template <size_t N>
void sampleStops(const std::array<ColorStop, N>& stops, float t, float& r, float& g, float& b) {
    if (N == 0) {
        r = g = b = t;
        return;
    }

    t = std::clamp(t, 0.0f, 1.0f);
    if (t <= stops.front().t) {
        r = stops.front().r;
        g = stops.front().g;
        b = stops.front().b;
        return;
    }
    if (t >= stops.back().t) {
        r = stops.back().r;
        g = stops.back().g;
        b = stops.back().b;
        return;
    }

    for (size_t i = 1; i < N; ++i) {
        if (t > stops[i].t) {
            continue;
        }

        const ColorStop& a = stops[i - 1];
        const ColorStop& c = stops[i];
        const float span = std::max(1e-6f, c.t - a.t);
        const float localT = (t - a.t) / span;
        r = a.r + (c.r - a.r) * localT;
        g = a.g + (c.g - a.g) * localT;
        b = a.b + (c.b - a.b) * localT;
        return;
    }

    r = stops.back().r;
    g = stops.back().g;
    b = stops.back().b;
}

template <size_t N>
void addStopsToTransferFunction(vtkColorTransferFunction* ctf,
                                const std::array<ColorStop, N>& stops,
                                double rangeMin,
                                double rangeMax,
                                double midpoint,
                                double sharpness) {
    for (const auto& stop : stops) {
        const double x = rangeMin + static_cast<double>(stop.t) * (rangeMax - rangeMin);
        ctf->AddRGBPoint(x, stop.r, stop.g, stop.b, midpoint, sharpness);
    }
}

}  // namespace

int ClampColorMapPresetIndex(int index, ColorMapPreset fallback) {
    if (index >= 0 && index < kColorMapPresetCount) {
        return index;
    }
    return ColorMapPresetToIndex(fallback);
}

ColorMapPreset ColorMapPresetFromIndex(int index, ColorMapPreset fallback) {
    const int clamped = ClampColorMapPresetIndex(index, fallback);
    return static_cast<ColorMapPreset>(clamped);
}

int ColorMapPresetToIndex(ColorMapPreset preset) {
    const int index = static_cast<int>(preset);
    if (index >= 0 && index < kColorMapPresetCount) {
        return index;
    }
    return static_cast<int>(ColorMapPreset::Grayscale);
}

const char* GetColorMapPresetLabel(ColorMapPreset preset) {
    return kColorMapLabels[ColorMapPresetToIndex(preset)];
}

const char* const* GetColorMapPresetLabels() {
    return kColorMapLabels.data();
}

void EvaluateColorMap(ColorMapPreset preset, float t, float& r, float& g, float& b) {
    t = std::clamp(t, 0.0f, 1.0f);

    switch (preset) {
    case ColorMapPreset::Grayscale:
        r = g = b = t;
        break;
    case ColorMapPreset::Rainbow:
        sampleStops(kRainbowStops, t, r, g, b);
        break;
    case ColorMapPreset::CoolToWarm:
        sampleStops(kCoolToWarmStops, t, r, g, b);
        break;
    case ColorMapPreset::Viridis:
        sampleStops(kViridisStops, t, r, g, b);
        break;
    case ColorMapPreset::Plasma:
        sampleStops(kPlasmaStops, t, r, g, b);
        break;
    case ColorMapPreset::Count:
    default:
        r = g = b = t;
        break;
    }
}

void ApplyColorMapToLookupTable(vtkLookupTable* lut, ColorMapPreset preset) {
    if (!lut) {
        return;
    }

    int tableSize = lut->GetNumberOfTableValues();
    if (tableSize <= 0) {
        tableSize = 256;
    }
    lut->SetNumberOfTableValues(tableSize);

    for (int i = 0; i < tableSize; ++i) {
        const float t = (tableSize <= 1)
            ? 0.0f
            : static_cast<float>(i) / static_cast<float>(tableSize - 1);
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        EvaluateColorMap(preset, t, r, g, b);
        lut->SetTableValue(i, r, g, b, 1.0);
    }
    lut->Build();
}

void ApplyColorMapToTransferFunction(vtkColorTransferFunction* ctf,
                                     ColorMapPreset preset,
                                     double rangeMin,
                                     double midValue,
                                     double rangeMax,
                                     double midpoint,
                                     double sharpness) {
    if (!ctf) {
        return;
    }

    if (rangeMax <= rangeMin) {
        rangeMax = rangeMin + 1.0;
    }
    if (midValue < rangeMin || midValue > rangeMax) {
        midValue = (rangeMin + rangeMax) * 0.5;
    }
    (void)midValue;

    ctf->RemoveAllPoints();
    switch (preset) {
    case ColorMapPreset::Grayscale:
        addStopsToTransferFunction(ctf, kGrayscaleStops, rangeMin, rangeMax, midpoint, sharpness);
        break;
    case ColorMapPreset::Rainbow:
        addStopsToTransferFunction(ctf, kRainbowStops, rangeMin, rangeMax, midpoint, sharpness);
        break;
    case ColorMapPreset::CoolToWarm:
        addStopsToTransferFunction(ctf, kCoolToWarmStops, rangeMin, rangeMax, midpoint, sharpness);
        break;
    case ColorMapPreset::Viridis:
        addStopsToTransferFunction(ctf, kViridisStops, rangeMin, rangeMax, midpoint, sharpness);
        break;
    case ColorMapPreset::Plasma:
        addStopsToTransferFunction(ctf, kPlasmaStops, rangeMin, rangeMax, midpoint, sharpness);
        break;
    case ColorMapPreset::Count:
    default:
        addStopsToTransferFunction(ctf, kGrayscaleStops, rangeMin, rangeMax, midpoint, sharpness);
        break;
    }
}

}  // namespace common
