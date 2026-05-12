#include "center.h"

#include "core/data/element_database.h"

#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkCoordinate.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace features::measurement {
namespace {

double MarkerRadius(const core::scene::AtomRecord& atom) {
    return std::max(0.06, static_cast<double>(atom.radius) * 0.55 + 0.02);
}

vtkSmartPointer<vtkActor> CreateSphereActor(
    const std::array<double, 3>& center,
    double radius,
    const std::array<double, 3>& color,
    bool wireframe,
    double opacity) {
    vtkSmartPointer<vtkSphereSource> source = vtkSmartPointer<vtkSphereSource>::New();
    source->SetCenter(center[0], center[1], center[2]);
    source->SetRadius(radius);
    source->SetThetaResolution(24);
    source->SetPhiResolution(24);
    source->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(source->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->SetPickable(false);
    if (vtkProperty* property = actor->GetProperty(); property != nullptr) {
        property->SetColor(color[0], color[1], color[2]);
        property->SetOpacity(opacity);
        property->SetLineWidth(2.0);
        property->SetAmbient(0.8);
        if (wireframe) {
            property->SetRepresentationToWireframe();
        }
    }
    return actor;
}

vtkSmartPointer<vtkActor2D> CreateValueTextActor(
    const std::string& text,
    const std::array<double, 3>& worldPosition,
    int fontSize,
    const std::array<double, 3>& color) {
    vtkSmartPointer<vtkTextActor> actor = vtkSmartPointer<vtkTextActor>::New();
    actor->SetInput(text.c_str());
    actor->SetPickable(false);

    if (vtkTextProperty* property = actor->GetTextProperty(); property != nullptr) {
        property->SetFontSize(fontSize);
        property->SetColor(color[0], color[1], color[2]);
        property->SetBackgroundColor(1.0, 1.0, 1.0);
        property->SetBackgroundOpacity(0.7);
        property->SetFrame(true);
        property->SetFrameColor(0.0, 0.0, 0.0);
        property->SetJustificationToCentered();
        property->SetVerticalJustificationToCentered();
    }

    if (vtkCoordinate* coord = actor->GetPositionCoordinate(); coord != nullptr) {
        coord->SetCoordinateSystemToWorld();
        coord->SetValue(worldPosition[0], worldPosition[1], worldPosition[2]);
    }
    return actor;
}

vtkSmartPointer<vtkActor2D> CreatePlusTextActor(
    const std::array<double, 3>& worldPosition,
    double radius,
    const std::array<double, 3>& color) {
    const int fontSize = std::clamp(static_cast<int>(std::lround(radius * 36.0)), 18, 72);
    vtkSmartPointer<vtkTextActor> actor = vtkSmartPointer<vtkTextActor>::New();
    actor->SetInput("+");
    actor->SetPickable(false);
    if (vtkTextProperty* property = actor->GetTextProperty(); property != nullptr) {
        property->SetFontSize(fontSize);
        property->SetBold(true);
        property->SetColor(color[0], color[1], color[2]);
        property->SetJustificationToCentered();
        property->SetVerticalJustificationToCentered();
    }
    if (vtkCoordinate* coord = actor->GetPositionCoordinate(); coord != nullptr) {
        coord->SetCoordinateSystemToWorld();
        coord->SetValue(worldPosition[0], worldPosition[1], worldPosition[2]);
    }
    return actor;
}

} // namespace

bool ComputeCenter(
    MeasurementType type,
    const std::vector<const core::scene::AtomRecord*>& atoms,
    std::array<double, 3>& center) {
    if (atoms.size() < 2) {
        return false;
    }

    center = {0.0, 0.0, 0.0};
    if (type == MeasurementType::GeometricCenter) {
        for (const core::scene::AtomRecord* atom : atoms) {
            if (atom == nullptr) {
                continue;
            }
            center[0] += static_cast<double>(atom->cartesian[0]);
            center[1] += static_cast<double>(atom->cartesian[1]);
            center[2] += static_cast<double>(atom->cartesian[2]);
        }
        const double scale = 1.0 / static_cast<double>(atoms.size());
        center[0] *= scale;
        center[1] *= scale;
        center[2] *= scale;
        return true;
    }

    if (type == MeasurementType::CenterOfMass) {
        double totalMass = 0.0;
        for (const core::scene::AtomRecord* atom : atoms) {
            if (atom == nullptr) {
                continue;
            }
            double mass = static_cast<double>(
                core::data::ElementDatabase::getInstance().getAtomicMass(atom->symbol));
            if (mass <= 0.0) {
                mass = 1.0;
            }
            center[0] += static_cast<double>(atom->cartesian[0]) * mass;
            center[1] += static_cast<double>(atom->cartesian[1]) * mass;
            center[2] += static_cast<double>(atom->cartesian[2]) * mass;
            totalMass += mass;
        }
        if (totalMass <= 1e-12) {
            return false;
        }
        center[0] /= totalMass;
        center[1] /= totalMass;
        center[2] /= totalMass;
        return true;
    }

    return false;
}

std::string FormatCenter(MeasurementType type, const std::array<double, 3>& center) {
    const char* prefix = type == MeasurementType::CenterOfMass ? "Center(mass)" : "Center(geom.)";
    std::ostringstream oss;
    oss << prefix << " " << std::fixed << std::setprecision(4)
        << "(" << center[0] << ", " << center[1] << ", " << center[2] << ")";
    return oss.str();
}

bool BuildCenterVisual(
    MeasurementType type,
    const std::vector<const core::scene::AtomRecord*>& atoms,
    CenterVisual& visual) {
    if (!ComputeCenter(type, atoms, visual.center)) {
        return false;
    }
    visual.valueText = FormatCenter(type, visual.center);

    double markerRadiusSum = 0.0;
    for (const core::scene::AtomRecord* atom : atoms) {
        if (atom != nullptr) {
            markerRadiusSum += MarkerRadius(*atom);
        }
    }
    const double markerRadius = std::max(0.08, markerRadiusSum / static_cast<double>(atoms.size()));
    visual.actors.push_back(CreateSphereActor(visual.center, markerRadius, {1.0, 0.0, 0.0}, false, 0.40));
    visual.textActors.push_back(CreatePlusTextActor(visual.center, markerRadius, {1.0, 0.0, 0.0}));

    const std::array<double, 3> labelPos = {
        visual.center[0],
        visual.center[1],
        visual.center[2] - std::max(0.04, markerRadius * 1.35),
    };
    visual.textActors.push_back(CreateValueTextActor(visual.valueText, labelPos, 21, {0.0, 0.0, 0.0}));

    for (const core::scene::AtomRecord* atom : atoms) {
        if (atom == nullptr) {
            continue;
        }
        const std::array<double, 3> atomCenter = {
            static_cast<double>(atom->cartesian[0]),
            static_cast<double>(atom->cartesian[1]),
            static_cast<double>(atom->cartesian[2]),
        };
        const double atomRadius = MarkerRadius(*atom);
        visual.actors.push_back(CreateSphereActor(atomCenter, atomRadius, {0.55, 0.55, 0.55}, true, 0.75));
        visual.textActors.push_back(CreatePlusTextActor(atomCenter, atomRadius, {0.0, 0.0, 0.0}));
    }

    return true;
}

} // namespace features::measurement
