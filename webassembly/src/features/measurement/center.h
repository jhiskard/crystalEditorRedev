/**
 * @file features/measurement/center.h
 * @brief Geometric center and center-of-mass measurement helpers.
 */
#pragma once

#include "measurement_mode.h"

#include "core/scene/scene_state.h"

#include <vtkSmartPointer.h>

#include <array>
#include <string>
#include <vector>

class vtkActor;
class vtkActor2D;

namespace features::measurement {

struct CenterVisual {
    std::array<double, 3> center = {0.0, 0.0, 0.0};
    std::string valueText;
    std::vector<vtkSmartPointer<vtkActor>> actors;
    std::vector<vtkSmartPointer<vtkActor2D>> textActors;
};

bool ComputeCenter(
    MeasurementType type,
    const std::vector<const core::scene::AtomRecord*>& atoms,
    std::array<double, 3>& center);

std::string FormatCenter(MeasurementType type, const std::array<double, 3>& center);

bool BuildCenterVisual(
    MeasurementType type,
    const std::vector<const core::scene::AtomRecord*>& atoms,
    CenterVisual& visual);

} // namespace features::measurement
