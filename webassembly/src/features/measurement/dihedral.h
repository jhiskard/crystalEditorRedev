/**
 * @file features/measurement/dihedral.h
 * @brief Dihedral measurement geometry and VTK actors.
 */
#pragma once

#include "core/scene/scene_state.h"

#include <vtkSmartPointer.h>

#include <string>

class vtkActor;
class vtkActor2D;

namespace features::measurement {

struct DihedralVisual {
    double dihedralDeg = 0.0;
    std::string valueText;
    vtkSmartPointer<vtkActor> lineActor12;
    vtkSmartPointer<vtkActor> lineActor23;
    vtkSmartPointer<vtkActor> lineActor34;
    vtkSmartPointer<vtkActor> helperPlaneActor1;
    vtkSmartPointer<vtkActor> helperPlaneActor2;
    vtkSmartPointer<vtkActor> helperArcActor;
    vtkSmartPointer<vtkActor2D> textActor;
};

std::string FormatDihedral(double angleDeg);
bool BuildDihedralVisual(
    const core::scene::AtomRecord& atom1,
    const core::scene::AtomRecord& atom2,
    const core::scene::AtomRecord& atom3,
    const core::scene::AtomRecord& atom4,
    DihedralVisual& visual);

} // namespace features::measurement
