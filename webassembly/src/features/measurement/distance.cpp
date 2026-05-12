#include "distance.h"

#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkCellArray.h>
#include <vtkCoordinate.h>
#include <vtkLine.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace features::measurement {
namespace {

vtkSmartPointer<vtkActor> CreateSegmentedLineActor(
    const std::array<float, 3>& lhs,
    const std::array<float, 3>& rhs) {
    constexpr int kSegmentCount = 20;
    constexpr double kVisibleRatio = 0.55;

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();

    const double vx = static_cast<double>(rhs[0]) - static_cast<double>(lhs[0]);
    const double vy = static_cast<double>(rhs[1]) - static_cast<double>(lhs[1]);
    const double vz = static_cast<double>(rhs[2]) - static_cast<double>(lhs[2]);

    for (int i = 0; i < kSegmentCount; ++i) {
        const double t0 = static_cast<double>(i) / static_cast<double>(kSegmentCount);
        const double t1 = std::min(
            1.0,
            (static_cast<double>(i) + kVisibleRatio) / static_cast<double>(kSegmentCount));
        if (t1 <= t0) {
            continue;
        }

        const vtkIdType id0 = points->InsertNextPoint(
            static_cast<double>(lhs[0]) + vx * t0,
            static_cast<double>(lhs[1]) + vy * t0,
            static_cast<double>(lhs[2]) + vz * t0);
        const vtkIdType id1 = points->InsertNextPoint(
            static_cast<double>(lhs[0]) + vx * t1,
            static_cast<double>(lhs[1]) + vy * t1,
            static_cast<double>(lhs[2]) + vz * t1);

        vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
        line->GetPointIds()->SetId(0, id0);
        line->GetPointIds()->SetId(1, id1);
        lines->InsertNextCell(line);
    }

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetLines(lines);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polyData);

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->SetPickable(false);
    if (vtkProperty* property = actor->GetProperty(); property != nullptr) {
        property->SetColor(0.133, 0.133, 0.133);
        property->SetLineWidth(2.0);
        property->SetAmbient(0.7);
    }
    return actor;
}

vtkSmartPointer<vtkActor2D> CreateValueTextActor(
    const std::string& text,
    const std::array<double, 3>& worldPosition) {
    vtkSmartPointer<vtkTextActor> actor = vtkSmartPointer<vtkTextActor>::New();
    actor->SetInput(text.c_str());
    actor->SetPickable(false);

    if (vtkTextProperty* property = actor->GetTextProperty(); property != nullptr) {
        property->SetFontSize(21);
        property->SetColor(0.0, 0.0, 0.0);
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

} // namespace

double ComputeDistance(const core::scene::AtomRecord& lhs, const core::scene::AtomRecord& rhs) {
    const double dx = static_cast<double>(lhs.cartesian[0]) - static_cast<double>(rhs.cartesian[0]);
    const double dy = static_cast<double>(lhs.cartesian[1]) - static_cast<double>(rhs.cartesian[1]);
    const double dz = static_cast<double>(lhs.cartesian[2]) - static_cast<double>(rhs.cartesian[2]);
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::string FormatDistance(double distance) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4) << distance << " A";
    return oss.str();
}

DistanceVisual BuildDistanceVisual(const core::scene::AtomRecord& lhs, const core::scene::AtomRecord& rhs) {
    DistanceVisual visual;
    visual.distance = ComputeDistance(lhs, rhs);
    visual.valueText = FormatDistance(visual.distance);

    const std::array<double, 3> midpoint = {
        (static_cast<double>(lhs.cartesian[0]) + static_cast<double>(rhs.cartesian[0])) * 0.5,
        (static_cast<double>(lhs.cartesian[1]) + static_cast<double>(rhs.cartesian[1])) * 0.5,
        (static_cast<double>(lhs.cartesian[2]) + static_cast<double>(rhs.cartesian[2])) * 0.5,
    };

    visual.lineActor = CreateSegmentedLineActor(lhs.cartesian, rhs.cartesian);
    visual.textActor = CreateValueTextActor(visual.valueText, midpoint);
    return visual;
}

} // namespace features::measurement
