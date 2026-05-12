#include "dihedral.h"

#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkCellArray.h>
#include <vtkCoordinate.h>
#include <vtkLine.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>

namespace features::measurement {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kRadToDeg = 180.0 / kPi;

std::array<double, 3> ToVec3(const std::array<float, 3>& value) {
    return {
        static_cast<double>(value[0]),
        static_cast<double>(value[1]),
        static_cast<double>(value[2]),
    };
}

std::array<double, 3> Add(const std::array<double, 3>& lhs, const std::array<double, 3>& rhs) {
    return {lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2]};
}

std::array<double, 3> Subtract(const std::array<double, 3>& lhs, const std::array<double, 3>& rhs) {
    return {lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2]};
}

std::array<double, 3> Scale(const std::array<double, 3>& value, double scale) {
    return {value[0] * scale, value[1] * scale, value[2] * scale};
}

double Dot(const std::array<double, 3>& lhs, const std::array<double, 3>& rhs) {
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}

std::array<double, 3> Cross(const std::array<double, 3>& lhs, const std::array<double, 3>& rhs) {
    return {
        lhs[1] * rhs[2] - lhs[2] * rhs[1],
        lhs[2] * rhs[0] - lhs[0] * rhs[2],
        lhs[0] * rhs[1] - lhs[1] * rhs[0],
    };
}

double Norm(const std::array<double, 3>& value) {
    return std::sqrt(Dot(value, value));
}

std::array<double, 3> Normalize(const std::array<double, 3>& value) {
    const double norm = Norm(value);
    if (norm <= 1e-12) {
        return {0.0, 0.0, 0.0};
    }
    return Scale(value, 1.0 / norm);
}

std::array<double, 3> ProjectToPlane(
    const std::array<double, 3>& value,
    const std::array<double, 3>& normalUnit) {
    return Subtract(value, Scale(normalUnit, Dot(value, normalUnit)));
}

std::array<double, 3> RotateAroundAxis(
    const std::array<double, 3>& value,
    const std::array<double, 3>& axisUnit,
    double angleRad) {
    const double c = std::cos(angleRad);
    const double s = std::sin(angleRad);
    return Add(
        Add(Scale(value, c), Scale(Cross(axisUnit, value), s)),
        Scale(axisUnit, Dot(axisUnit, value) * (1.0 - c)));
}

vtkSmartPointer<vtkActor> CreateLineActor(
    const std::array<float, 3>& lhs,
    const std::array<float, 3>& rhs) {
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    const vtkIdType id0 = points->InsertNextPoint(lhs[0], lhs[1], lhs[2]);
    const vtkIdType id1 = points->InsertNextPoint(rhs[0], rhs[1], rhs[2]);

    vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
    line->GetPointIds()->SetId(0, id0);
    line->GetPointIds()->SetId(1, id1);

    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    lines->InsertNextCell(line);

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

vtkSmartPointer<vtkActor> CreatePolylineActor(const std::vector<std::array<double, 3>>& pointsIn) {
    if (pointsIn.size() < 2) {
        return nullptr;
    }

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(static_cast<vtkIdType>(pointsIn.size()));
    for (vtkIdType i = 0; i < static_cast<vtkIdType>(pointsIn.size()); ++i) {
        points->SetPoint(i, pointsIn[static_cast<size_t>(i)].data());
    }

    vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
    polyLine->GetPointIds()->SetNumberOfIds(static_cast<vtkIdType>(pointsIn.size()));
    for (vtkIdType i = 0; i < static_cast<vtkIdType>(pointsIn.size()); ++i) {
        polyLine->GetPointIds()->SetId(i, i);
    }

    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    lines->InsertNextCell(polyLine);

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

vtkSmartPointer<vtkActor> CreatePlaneActor(
    const std::array<std::array<double, 3>, 4>& quad,
    const std::array<double, 3>& color) {
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(4);
    for (vtkIdType i = 0; i < 4; ++i) {
        points->SetPoint(i, quad[static_cast<size_t>(i)].data());
    }

    vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
    vtkIdType ids[4] = {0, 1, 2, 3};
    polys->InsertNextCell(4, ids);

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetPolys(polys);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polyData);

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->SetPickable(false);
    if (vtkProperty* property = actor->GetProperty(); property != nullptr) {
        property->SetColor(color[0], color[1], color[2]);
        property->SetOpacity(0.20);
        property->SetRepresentationToSurface();
        property->SetEdgeVisibility(false);
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

std::string FormatDihedral(double angleDeg) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << angleDeg << " deg";
    return oss.str();
}

bool BuildDihedralVisual(
    const core::scene::AtomRecord& atom1,
    const core::scene::AtomRecord& atom2,
    const core::scene::AtomRecord& atom3,
    const core::scene::AtomRecord& atom4,
    DihedralVisual& visual) {
    const auto p1 = ToVec3(atom1.cartesian);
    const auto p2 = ToVec3(atom2.cartesian);
    const auto p3 = ToVec3(atom3.cartesian);
    const auto p4 = ToVec3(atom4.cartesian);

    const auto b1 = Subtract(p2, p1);
    const auto b2 = Subtract(p3, p2);
    const auto b3 = Subtract(p4, p3);
    const double b2Len = Norm(b2);
    if (b2Len <= 1e-8) {
        return false;
    }

    const auto n12 = Cross(b1, b2);
    const auto n23 = Cross(b2, b3);
    if (Norm(n12) <= 1e-8 || Norm(n23) <= 1e-8) {
        return false;
    }

    const auto axis23 = Normalize(b2);
    const double x = Dot(n12, n23);
    const double y = b2Len * Dot(b1, Cross(b2, b3));
    const double angleRad = std::atan2(y, x);
    visual.dihedralDeg = angleRad * kRadToDeg;
    visual.valueText = FormatDihedral(visual.dihedralDeg);

    const auto proj1 = ProjectToPlane(b1, axis23);
    const auto proj3 = ProjectToPlane(b3, axis23);
    const double proj1Len = Norm(proj1);
    const double proj3Len = Norm(proj3);
    if (proj1Len <= 1e-8 || proj3Len <= 1e-8) {
        return false;
    }

    const auto startDir = Normalize(proj1);
    const auto endDir = Normalize(proj3);
    auto orthoDir = Normalize(Cross(axis23, startDir));
    if (Dot(orthoDir, endDir) < 0.0) {
        orthoDir = Scale(orthoDir, -1.0);
    }

    const auto mid23 = Scale(Add(p2, p3), 0.5);
    const double arcRadius = std::max(0.05, std::min(proj1Len, proj3Len) * 0.6);
    const double planeRadius = std::max(0.05, arcRadius);
    const double halfLen = std::max(0.08, b2Len * 0.55);
    const auto axisOffset = Scale(axis23, halfLen);
    const auto planeOffset1 = Scale(startDir, planeRadius);
    const auto planeOffset3 = Scale(endDir, planeRadius);

    const std::array<std::array<double, 3>, 4> planeA = {{
        Add(Subtract(mid23, axisOffset), planeOffset1),
        Add(Add(mid23, axisOffset), planeOffset1),
        Subtract(Add(mid23, axisOffset), planeOffset1),
        Subtract(Subtract(mid23, axisOffset), planeOffset1),
    }};
    const std::array<std::array<double, 3>, 4> planeB = {{
        Add(Subtract(mid23, axisOffset), planeOffset3),
        Add(Add(mid23, axisOffset), planeOffset3),
        Subtract(Add(mid23, axisOffset), planeOffset3),
        Subtract(Subtract(mid23, axisOffset), planeOffset3),
    }};

    constexpr int kResolution = 48;
    std::vector<std::array<double, 3>> arcPoints;
    arcPoints.reserve(kResolution + 1);
    for (int i = 0; i <= kResolution; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(kResolution);
        const double theta = angleRad * t;
        const auto dir = RotateAroundAxis(startDir, axis23, theta);
        arcPoints.push_back(Add(mid23, Scale(dir, arcRadius)));
    }

    const auto textDir = Normalize(RotateAroundAxis(startDir, axis23, angleRad * 0.5));
    const auto textPos = Add(
        Add(mid23, Scale(textDir, arcRadius * 1.18)),
        Scale(axis23, std::max(0.02, arcRadius * 0.12)));

    visual.lineActor12 = CreateLineActor(atom1.cartesian, atom2.cartesian);
    visual.lineActor23 = CreateLineActor(atom2.cartesian, atom3.cartesian);
    visual.lineActor34 = CreateLineActor(atom3.cartesian, atom4.cartesian);
    visual.helperPlaneActor1 = CreatePlaneActor(planeA, {0.0, 0.0, 1.0});
    visual.helperPlaneActor2 = CreatePlaneActor(planeB, {125.0 / 255.0, 0.0, 1.0});
    visual.helperArcActor = CreatePolylineActor(arcPoints);
    visual.textActor = CreateValueTextActor(visual.valueText, textPos);
    return true;
}

} // namespace features::measurement
