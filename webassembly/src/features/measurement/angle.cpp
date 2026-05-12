#include "angle.h"

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

std::array<double, 3> PickPerpendicularAxis(const std::array<double, 3>& value) {
    if (std::fabs(value[2]) < 0.8) {
        return {0.0, 0.0, 1.0};
    }
    return {0.0, 1.0, 0.0};
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

std::string FormatAngle(double angleDeg) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << angleDeg << " deg";
    return oss.str();
}

bool BuildAngleVisual(
    const core::scene::AtomRecord& atom1,
    const core::scene::AtomRecord& atom2,
    const core::scene::AtomRecord& atom3,
    AngleVisual& visual) {
    const auto p1 = ToVec3(atom1.cartesian);
    const auto p2 = ToVec3(atom2.cartesian);
    const auto p3 = ToVec3(atom3.cartesian);
    const auto v1 = Subtract(p1, p2);
    const auto v2 = Subtract(p3, p2);

    const double len1 = Norm(v1);
    const double len2 = Norm(v2);
    if (len1 <= 1e-8 || len2 <= 1e-8) {
        return false;
    }

    const auto dir1 = Normalize(v1);
    const auto dir2 = Normalize(v2);
    const double cosine = std::clamp(Dot(dir1, dir2), -1.0, 1.0);
    const double angleRad = std::acos(cosine);
    visual.angleDeg = angleRad * kRadToDeg;
    visual.valueText = FormatAngle(visual.angleDeg);

    auto normal = Cross(dir1, dir2);
    if (Norm(normal) <= 1e-8) {
        normal = Cross(dir1, PickPerpendicularAxis(dir1));
    }
    normal = Normalize(normal);
    auto tangent = Normalize(Cross(normal, dir1));
    if (Dot(tangent, dir2) < 0.0) {
        tangent = Scale(tangent, -1.0);
    }

    const double radius = std::max(0.05, std::min(len1, len2) * 0.35);
    constexpr int kResolution = 48;
    std::vector<std::array<double, 3>> arcPoints;
    arcPoints.reserve(kResolution + 1);
    for (int i = 0; i <= kResolution; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(kResolution);
        const double theta = angleRad * t;
        const auto dir = Add(Scale(dir1, std::cos(theta)), Scale(tangent, std::sin(theta)));
        arcPoints.push_back(Add(p2, Scale(dir, radius)));
    }

    auto bisector = Normalize(Add(dir1, dir2));
    if (Norm(bisector) <= 1e-8) {
        bisector = tangent;
    }
    const auto textPos = Add(p2, Scale(bisector, radius * 1.18));

    visual.lineActor12 = CreateLineActor(atom1.cartesian, atom2.cartesian);
    visual.lineActor23 = CreateLineActor(atom2.cartesian, atom3.cartesian);
    visual.arcActor = CreatePolylineActor(arcPoints);
    visual.textActor = CreateValueTextActor(visual.valueText, textPos);
    return true;
}

} // namespace features::measurement
