/**
 * @file features/measurement/distance.h
 * @brief Distance measurement geometry and VTK actors.
 */
#pragma once

#include "core/scene/scene_state.h"

#include <vtkSmartPointer.h>

#include <string>

class vtkActor;
class vtkActor2D;

namespace features::measurement {

struct DistanceVisual {
    double distance = 0.0;
    std::string valueText;
    vtkSmartPointer<vtkActor> lineActor;
    vtkSmartPointer<vtkActor2D> textActor;
};

double ComputeDistance(const core::scene::AtomRecord& lhs, const core::scene::AtomRecord& rhs);
std::string FormatDistance(double distance);
DistanceVisual BuildDistanceVisual(const core::scene::AtomRecord& lhs, const core::scene::AtomRecord& rhs);

} // namespace features::measurement
