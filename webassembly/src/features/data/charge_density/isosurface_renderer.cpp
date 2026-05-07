#include "isosurface_renderer.h"

#include "core/vtk/vtk_viewer.h"

#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <algorithm>
#include <cmath>

namespace features::data::charge_density {
namespace {

vtkSmartPointer<vtkImageData> BuildImageData(const ChargeDensity& cd) {
    const std::array<int, 3> grid = cd.GridShape();
    const int nx = grid[0];
    const int ny = grid[1];
    const int nz = grid[2];
    if (nx <= 0 || ny <= 0 || nz <= 0) {
        return nullptr;
    }

    vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
    image->SetDimensions(nx, ny, nz);
    image->SetOrigin(0.0, 0.0, 0.0);
    image->SetSpacing(1.0 / static_cast<double>(nx),
                      1.0 / static_cast<double>(ny),
                      1.0 / static_cast<double>(nz));
    image->AllocateScalars(VTK_FLOAT, 1);

    const std::vector<float>& raw = cd.Data();
    float* ptr = static_cast<float*>(image->GetScalarPointer());
    if (ptr == nullptr || raw.empty()) {
        return image;
    }

    for (int ix = 0; ix < nx; ++ix) {
        for (int iy = 0; iy < ny; ++iy) {
            for (int iz = 0; iz < nz; ++iz) {
                const int vaspIdx = iz + iy * nz + ix * ny * nz;
                const int vtkIdx = ix + iy * nx + iz * nx * ny;
                if (vaspIdx >= 0 && vaspIdx < static_cast<int>(raw.size())) {
                    ptr[vtkIdx] = raw[vaspIdx];
                }
            }
        }
    }

    image->Modified();
    return image;
}

vtkSmartPointer<vtkTransform> BuildLatticeTransform(const ChargeDensity& cd) {
    const auto& lattice = cd.Lattice();
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();

    const double matrix[16] = {
        lattice[0][0], lattice[1][0], lattice[2][0], 0.0,
        lattice[0][1], lattice[1][1], lattice[2][1], 0.0,
        lattice[0][2], lattice[1][2], lattice[2][2], 0.0,
        0.0,           0.0,           0.0,           1.0,
    };
    transform->SetMatrix(matrix);
    return transform;
}

} // namespace

void IsosurfaceRenderer::Render(const ChargeDensity& cd,
                                float isoValue,
                                bool wireframe,
                                float opacity,
                                const std::array<float, 3>& color) {
    const bool rendered = addContourActor(cd, isoValue, wireframe, opacity, color, false, {0.0f, 0.0f, 0.0f});
    if (rendered) {
        core::vtk::VtkViewer::Instance().RequestRender();
    }
}

void IsosurfaceRenderer::RenderSurface(const ChargeDensity& cd,
                                       float isoValue,
                                       const std::array<float, 3>& color,
                                       float opacity,
                                       const std::array<float, 3>& edgeColor) {
    const bool rendered = addContourActor(cd, isoValue, false, opacity, color, true, edgeColor);
    if (rendered) {
        core::vtk::VtkViewer::Instance().RequestRender();
    }
}

void IsosurfaceRenderer::RenderMultiple(const ChargeDensity& cd,
                                        float positiveIsoValue,
                                        float negativeIsoValue,
                                        bool wireframe,
                                        float opacity,
                                        const std::array<float, 3>& positiveColor,
                                        const std::array<float, 3>& negativeColor) {
    bool rendered = false;
    rendered |= addContourActor(cd, positiveIsoValue, wireframe, opacity, positiveColor);
    if (std::abs(positiveIsoValue - negativeIsoValue) > 1e-6f) {
        rendered |= addContourActor(cd, negativeIsoValue, wireframe, opacity, negativeColor);
    }
    if (rendered) {
        core::vtk::VtkViewer::Instance().RequestRender();
    }
}

void IsosurfaceRenderer::Clear() {
    bool removed = false;
    for (const auto& actor : actors_) {
        if (actor != nullptr) {
            core::vtk::VtkViewer::Instance().RemoveActor(actor);
            removed = true;
        }
    }
    actors_.clear();
    contours_.clear();
    if (removed) {
        core::vtk::VtkViewer::Instance().RequestRender();
    }
}

bool IsosurfaceRenderer::addContourActor(const ChargeDensity& cd,
                                         float isoValue,
                                         bool wireframe,
                                         float opacity,
                                         const std::array<float, 3>& color,
                                         bool showEdges,
                                         const std::array<float, 3>& edgeColor) {
    vtkSmartPointer<vtkImageData> image = BuildImageData(cd);
    if (image == nullptr) {
        return false;
    }

    vtkSmartPointer<vtkContourFilter> contour = vtkSmartPointer<vtkContourFilter>::New();
    contour->SetInputData(image);
    contour->SetValue(0, isoValue);
    contour->Update();

    vtkPolyData* output = contour->GetOutput();
    if (output == nullptr || output->GetNumberOfPoints() == 0) {
        return false;
    }

    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter =
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter->SetInputData(output);
    transformFilter->SetTransform(BuildLatticeTransform(cd));
    transformFilter->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(transformFilter->GetOutputPort());
    mapper->ScalarVisibilityOff();

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(color[0], color[1], color[2]);
    actor->GetProperty()->SetOpacity(std::clamp(opacity, 0.01f, 1.0f));
    actor->GetProperty()->SetRepresentation(wireframe ? VTK_WIREFRAME : VTK_SURFACE);
    actor->GetProperty()->SetEdgeVisibility(showEdges ? 1 : 0);
    if (showEdges) {
        actor->GetProperty()->SetEdgeColor(edgeColor[0], edgeColor[1], edgeColor[2]);
        actor->GetProperty()->SetLineWidth(1.0f);
    }

    core::vtk::VtkViewer::Instance().AddActor(actor);
    contours_.push_back(contour);
    actors_.push_back(actor);
    return true;
}

} // namespace features::data::charge_density
