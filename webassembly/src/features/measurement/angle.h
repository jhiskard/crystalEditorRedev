/**
 * @file features/measurement/angle.h
 * @brief Angle measurement geometry and VTK actors.
 */
#pragma once

#include "core/scene/scene_state.h"

#include <vtkSmartPointer.h>

#include <string>

class vtkActor;
class vtkActor2D;

namespace features::measurement {

struct AngleVisual {
    double angleDeg = 0.0;
    std::string valueText;
    vtkSmartPointer<vtkActor> lineActor12;
    vtkSmartPointer<vtkActor> lineActor23;
    vtkSmartPointer<vtkActor> arcActor;
    vtkSmartPointer<vtkActor2D> textActor;
};

std::string FormatAngle(double angleDeg);
bool BuildAngleVisual(
    const core::scene::AtomRecord& atom1,
    const core::scene::AtomRecord& atom2,
    const core::scene::AtomRecord& atom3,
    AngleVisual& visual,
    double arcRadiusScale = 1.0);

} // namespace features::measurement
