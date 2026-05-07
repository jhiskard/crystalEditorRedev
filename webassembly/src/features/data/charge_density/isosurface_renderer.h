#pragma once

#include "charge_density.h"

#include <array>
#include <vector>

#include <vtkActor.h>
#include <vtkContourFilter.h>
#include <vtkSmartPointer.h>

namespace features::data::charge_density {

class IsosurfaceRenderer {
public:
    void Render(const ChargeDensity& cd,
                float isoValue,
                bool wireframe,
                float opacity = 0.7f,
                const std::array<float, 3>& color = {0.05f, 0.60f, 0.95f});
    void RenderSurface(const ChargeDensity& cd,
                       float isoValue,
                       const std::array<float, 3>& color,
                       float opacity,
                       const std::array<float, 3>& edgeColor);
    void RenderMultiple(const ChargeDensity& cd,
                        float positiveIsoValue,
                        float negativeIsoValue,
                        bool wireframe,
                        float opacity,
                        const std::array<float, 3>& positiveColor,
                        const std::array<float, 3>& negativeColor);
    void Clear();

private:
    bool addContourActor(const ChargeDensity& cd,
                         float isoValue,
                         bool wireframe,
                         float opacity,
                         const std::array<float, 3>& color,
                         bool showEdges = false,
                         const std::array<float, 3>& edgeColor = {0.0f, 0.0f, 0.0f});

    std::vector<vtkSmartPointer<vtkContourFilter>> contours_;
    std::vector<vtkSmartPointer<vtkActor>> actors_;
};

} // namespace features::data::charge_density
