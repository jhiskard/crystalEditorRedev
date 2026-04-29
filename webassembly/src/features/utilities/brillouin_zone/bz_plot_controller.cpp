/**
 * @file features/utilities/brillouin_zone/bz_plot_controller.cpp
 * @brief Brillouin Zone feature controller.
 */
#include "bz_plot_controller.h"

#include "core/vtk/vtk_viewer.h"
#include "special_points.h"

#include <spdlog/spdlog.h>

#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkCellArray.h>
#include <vtkCoordinate.h>
#include <vtkGlyph3D.h>
#include <vtkLineSource.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <algorithm>
#include <cmath>

namespace features::utilities::bz {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

BZPlotController::BZPlotController(core::scene::SceneState& scene)
    : scene_(scene) {
}

bool BZPlotController::TryGetCurrentCellInfo(CellInfo& out) const {
    if (!hasCurrentCell_) {
        return false;
    }

    out = currentCell_;
    return true;
}

void BZPlotController::SetCellInfoForTesting(const CellInfo& cell) {
    currentCell_ = cell;
    hasCurrentCell_ = cell.valid;
}

bool BZPlotController::ResolveCellInfo(const std::string& path, CellInfo& outCell) const {
    if (hasCurrentCell_ && currentCell_.valid) {
        outCell = currentCell_;
        return true;
    }

    if (path == "TEST" || path == "__test__") {
        CellInfo test;
        test.valid = true;

        const double a = 1.0;
        const double twoPi = 2.0 * kPi;
        test.matrix = {{{a, 0.0, 0.0}, {0.0, a, 0.0}, {0.0, 0.0, a}}};
        test.invmatrix = {{{twoPi / a, 0.0, 0.0}, {0.0, twoPi / a, 0.0}, {0.0, 0.0, twoPi / a}}};

        outCell = test;
        return true;
    }

    return false;
}

void BZPlotController::Clear() {
    layer_.clear();
    showing_ = false;
    core::vtk::VtkViewer::Instance().RequestRender();
}

bool BZPlotController::Show(
    const std::string& path,
    int npoints,
    bool showVectors,
    bool showLabels,
    std::string& outErrorMessage) {
    outErrorMessage.clear();

    CellInfo cellInfo;
    if (!ResolveCellInfo(path, cellInfo)) {
        outErrorMessage = "Cell info is not wired yet. It will be connected in Phase 3.4.";
        return false;
    }

    if (!cellInfo.valid) {
        outErrorMessage = "Cell info is invalid.";
        return false;
    }

    double cell[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    double icell[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            cell[i][j] = cellInfo.matrix[i][j];
            icell[i][j] = cellInfo.invmatrix[i][j];
        }
    }

    const BZVerticesResult bzData = BZCalculator::calculateBZVertices(icell);
    if (!bzData.success) {
        outErrorMessage = "BZ calculation failed: " + bzData.errorMessage;
        return false;
    }

    Clear();
    if (!RenderCompleteBZPlot(bzData, cell, icell, showVectors, showLabels, path, npoints)) {
        outErrorMessage = "Failed to render BZ plot actors.";
        return false;
    }

    layer_.show();
    showing_ = true;
    lastResult_ = bzData;

    core::vtk::VtkViewer::Instance().FitViewToVisibleProps();
    return true;
}

bool BZPlotController::RenderCompleteBZPlot(
    const BZVerticesResult& bzData,
    const double cell[3][3],
    const double icell[3][3],
    bool showVectors,
    bool showLabels,
    const std::string& path,
    int npoints) {
    if (!bzData.success || bzData.facets.empty()) {
        return false;
    }

    RenderIBZLines(bzData);

    if (showVectors) {
        RenderReciprocalVectors(icell);
    }

    if (!path.empty() && npoints > 1) {
        std::string normalizedPath = path;
        if (normalizedPath == "All" || normalizedPath == "all" || normalizedPath == "ALL") {
            const std::string latticeType = SpecialPointsDatabase::detectLatticeType(cell);
            normalizedPath = SpecialPointsDatabase::getDefaultPath(latticeType);
        }

        const auto pathSegments = PathParser::parse(normalizedPath);
        if (!pathSegments.empty()) {
            const std::string latticeType = SpecialPointsDatabase::detectLatticeType(cell);
            const auto specialFrac = SpecialPointsDatabase::getSpecialPoints(latticeType);

            std::map<std::string, std::array<double, 3>> specialCart;
            for (const auto& kv : specialFrac) {
                specialCart[kv.first] = SpecialPointsDatabase::fractionalToCartesian(kv.second, icell);
            }

            const auto kpoints = KpointsInterpolator::generateKpoints(pathSegments, specialCart, npoints);
            if (!kpoints.empty()) {
                RenderBandpath(kpoints);
                RenderKpoints(kpoints, std::max(0.01, 0.02 * CalculateMaxReciprocalVectorLength(icell)));
            }

            if (showLabels) {
                std::map<std::string, std::array<double, 3>> labelsToShow;
                for (const auto& segment : pathSegments) {
                    for (const auto& label : segment) {
                        const auto it = specialCart.find(label);
                        if (it != specialCart.end()) {
                            labelsToShow[label] = it->second;
                        }
                    }
                }
                RenderSpecialPointLabels(labelsToShow);
            }
        }
    }

    return layer_.getTotalActorCount() > 0;
}

void BZPlotController::RenderIBZLines(const BZVerticesResult& bzData) {
    const double black[3] = {0.0, 0.0, 0.0};

    for (const auto& facet : bzData.facets) {
        if (facet.vertices.size() < 2) {
            continue;
        }

        const size_t numVertices = facet.vertices.size();
        for (size_t i = 0; i < numVertices; ++i) {
            const size_t next = (i + 1) % numVertices;
            const double p1[3] = {facet.vertices[i][0], facet.vertices[i][1], facet.vertices[i][2]};
            const double p2[3] = {facet.vertices[next][0], facet.vertices[next][1], facet.vertices[next][2]};

            auto actor = CreateLineActor(p1, p2, black, 2.6);
            if (actor) {
                layer_.addIBZLineActor(actor);
            }
        }
    }
}

void BZPlotController::RenderReciprocalVectors(const double icell[3][3]) {
    const double origin[3] = {0.0, 0.0, 0.0};
    const double black[3] = {0.0, 0.0, 0.0};
    const char* labels[3] = {"b1", "b2", "b3"};

    const double maxLen = CalculateMaxReciprocalVectorLength(icell);
    const double shaftRadius = std::max(0.01, maxLen * 0.01);

    for (int i = 0; i < 3; ++i) {
        const double end[3] = {icell[i][0], icell[i][1], icell[i][2]};
        auto arrow = CreateArrowActor(origin, end, black, shaftRadius);
        if (arrow) {
            layer_.addReciprocalVectorActor(arrow);
        }

        const double labelPos[3] = {end[0] * 1.02, end[1] * 1.02, end[2] * 1.02};
        auto label = CreateTextActor2D(labels[i], labelPos, 20, black);
        if (label) {
            layer_.addLabelActor(label);
        }
    }
}

void BZPlotController::RenderBandpath(const std::vector<std::array<double, 3>>& kpoints) {
    if (kpoints.size() < 2) {
        return;
    }

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    for (const auto& kp : kpoints) {
        points->InsertNextPoint(kp[0], kp[1], kp[2]);
    }

    vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
    polyLine->GetPointIds()->SetNumberOfIds(static_cast<vtkIdType>(kpoints.size()));
    for (size_t i = 0; i < kpoints.size(); ++i) {
        polyLine->GetPointIds()->SetId(static_cast<vtkIdType>(i), static_cast<vtkIdType>(i));
    }

    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(polyLine);

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetLines(cells);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polyData);

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    actor->GetProperty()->SetLineWidth(4.0);

    layer_.addBandpathActor(actor);
}

void BZPlotController::RenderKpoints(const std::vector<std::array<double, 3>>& kpoints, double radius) {
    if (kpoints.empty()) {
        return;
    }

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    for (const auto& kp : kpoints) {
        points->InsertNextPoint(kp[0], kp[1], kp[2]);
    }

    vtkSmartPointer<vtkPolyData> pointsPolyData = vtkSmartPointer<vtkPolyData>::New();
    pointsPolyData->SetPoints(points);

    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetRadius(radius);
    sphereSource->SetThetaResolution(16);
    sphereSource->SetPhiResolution(16);

    vtkSmartPointer<vtkGlyph3D> glyph = vtkSmartPointer<vtkGlyph3D>::New();
    glyph->SetInputData(pointsPolyData);
    glyph->SetSourceConnection(sphereSource->GetOutputPort());
    glyph->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(glyph->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 0.0, 0.0);

    layer_.addKpointActor(actor);
}

void BZPlotController::RenderSpecialPointLabels(
    const std::map<std::string, std::array<double, 3>>& specialPoints) {
    const double black[3] = {0.0, 0.0, 0.0};

    for (const auto& kv : specialPoints) {
        double labelPos[3] = {kv.second[0] * 1.01, kv.second[1] * 1.01, kv.second[2] * 1.01};

        std::string display = kv.first;
        if (display == "Gamma") {
            display = "G";
        }

        auto labelActor = CreateTextActor2D(display, labelPos, 20, black);
        if (labelActor) {
            layer_.addLabelActor(labelActor);
        }
    }
}

vtkSmartPointer<vtkActor> BZPlotController::CreateLineActor(
    const double p1[3],
    const double p2[3],
    const double color[3],
    double width) {
    vtkSmartPointer<vtkLineSource> lineSource = vtkSmartPointer<vtkLineSource>::New();
    lineSource->SetPoint1(p1);
    lineSource->SetPoint2(p2);
    lineSource->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(lineSource->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(color[0], color[1], color[2]);
    actor->GetProperty()->SetLineWidth(width);
    return actor;
}

vtkSmartPointer<vtkActor> BZPlotController::CreateArrowActor(
    const double start[3],
    const double end[3],
    const double color[3],
    double shaftRadius) {
    double direction[3] = {end[0] - start[0], end[1] - start[1], end[2] - start[2]};
    const double length = std::sqrt(
        direction[0] * direction[0] +
        direction[1] * direction[1] +
        direction[2] * direction[2]);

    if (length < 1e-12) {
        return nullptr;
    }

    vtkSmartPointer<vtkArrowSource> arrowSource = vtkSmartPointer<vtkArrowSource>::New();
    arrowSource->SetTipLength(0.10);
    arrowSource->SetTipRadius(shaftRadius * 3.0);
    arrowSource->SetShaftRadius(shaftRadius);

    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Translate(start);

    double xAxis[3] = {1.0, 0.0, 0.0};
    double normalizedDir[3] = {direction[0] / length, direction[1] / length, direction[2] / length};
    double rotationAxis[3] = {
        xAxis[1] * normalizedDir[2] - xAxis[2] * normalizedDir[1],
        xAxis[2] * normalizedDir[0] - xAxis[0] * normalizedDir[2],
        xAxis[0] * normalizedDir[1] - xAxis[1] * normalizedDir[0]};

    const double rotationAxisLength = std::sqrt(
        rotationAxis[0] * rotationAxis[0] +
        rotationAxis[1] * rotationAxis[1] +
        rotationAxis[2] * rotationAxis[2]);

    const double dotProduct =
        xAxis[0] * normalizedDir[0] +
        xAxis[1] * normalizedDir[1] +
        xAxis[2] * normalizedDir[2];

    if (rotationAxisLength > 1e-6) {
        double angle = std::acos(std::clamp(dotProduct, -1.0, 1.0));
        angle = angle * 180.0 / kPi;
        transform->RotateWXYZ(
            angle,
            rotationAxis[0] / rotationAxisLength,
            rotationAxis[1] / rotationAxisLength,
            rotationAxis[2] / rotationAxisLength);
    } else if (dotProduct < 0) {
        transform->RotateY(180.0);
    }

    transform->Scale(length, 1.0, 1.0);

    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter->SetInputConnection(arrowSource->GetOutputPort());
    transformFilter->SetTransform(transform);
    transformFilter->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(transformFilter->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(color[0], color[1], color[2]);
    return actor;
}

vtkSmartPointer<vtkActor2D> BZPlotController::CreateTextActor2D(
    const std::string& text,
    const double position[3],
    int fontSize,
    const double color[3]) {
    vtkSmartPointer<vtkTextActor> actor = vtkSmartPointer<vtkTextActor>::New();
    actor->SetInput(text.c_str());

    vtkTextProperty* property = actor->GetTextProperty();
    if (property != nullptr) {
        property->SetFontSize(fontSize);
        property->SetColor(color[0], color[1], color[2]);
        property->SetBackgroundColor(1.0, 1.0, 1.0);
        property->SetBackgroundOpacity(0.7);
        property->SetFrame(true);
        property->SetFrameColor(0.0, 0.0, 0.0);
        property->SetJustificationToCentered();
        property->SetVerticalJustificationToCentered();
    }

    vtkCoordinate* coord = actor->GetPositionCoordinate();
    coord->SetCoordinateSystemToWorld();
    coord->SetValue(position[0], position[1], position[2]);

    return actor;
}

double BZPlotController::CalculateMaxReciprocalVectorLength(const double icell[3][3]) {
    double maxLength = 0.0;
    for (int i = 0; i < 3; ++i) {
        const double len = std::sqrt(
            icell[i][0] * icell[i][0] +
            icell[i][1] * icell[i][1] +
            icell[i][2] * icell[i][2]);
        maxLength = std::max(maxLength, len);
    }

    return maxLength;
}

} // namespace features::utilities::bz
