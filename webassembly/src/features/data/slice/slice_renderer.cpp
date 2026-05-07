#include "slice_renderer.h"

#include "core/vtk/vtk_viewer.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

#include <algorithm>
#include <cmath>

namespace features::data::slice {
namespace {

vtkSmartPointer<vtkImageData> BuildImageData(const features::data::charge_density::ChargeDensity& cd) {
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

} // namespace

void SliceRenderer::SetPlane(SlicePlane plane, float position) {
    plane_ = plane;
    position_ = std::clamp(position, 0.0f, 1.0f);
}

void SliceRenderer::SetMillerIndices(int h, int k, int l) {
    millerH_ = std::clamp(h, 0, 6);
    millerK_ = std::clamp(k, 0, 6);
    millerL_ = std::clamp(l, 0, 6);
}

void SliceRenderer::SetColorMap(core::data::ColorMapPreset preset) {
    colorMap_ = preset;
}

void SliceRenderer::SetValueRange(float minValue, float maxValue) {
    valueMin_ = minValue;
    valueMax_ = (maxValue > minValue) ? maxValue : (minValue + 1.0f);
}

void SliceRenderer::SetColorCurve(float midpoint, float sharpness) {
    colorMidpoint_ = std::clamp(midpoint, 0.0f, 1.0f);
    colorSharpness_ = std::clamp(sharpness, 0.0f, 1.0f);
}

void SliceRenderer::Render(const features::data::charge_density::ChargeDensity& cd) {
    Clear();

    vtkSmartPointer<vtkImageData> image = BuildImageData(cd);
    if (image == nullptr) {
        return;
    }

    vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
    switch (plane_) {
    case SlicePlane::XY:
        plane->SetNormal(0.0, 0.0, 1.0);
        plane->SetOrigin(0.0, 0.0, position_);
        break;
    case SlicePlane::XZ:
        plane->SetNormal(0.0, 1.0, 0.0);
        plane->SetOrigin(0.0, position_, 0.0);
        break;
    case SlicePlane::YZ:
        plane->SetNormal(1.0, 0.0, 0.0);
        plane->SetOrigin(position_, 0.0, 0.0);
        break;
    case SlicePlane::Miller:
    default:
        if (millerH_ == 0 && millerK_ == 0 && millerL_ == 0) {
            return;
        }

        double bounds[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        image->GetBounds(bounds);
        const double extentX = std::max(bounds[1] - bounds[0], 1e-8);
        const double extentY = std::max(bounds[3] - bounds[2], 1e-8);
        const double extentZ = std::max(bounds[5] - bounds[4], 1e-8);

        double normal[3] = {
            static_cast<double>(millerH_) / extentX,
            static_cast<double>(millerK_) / extentY,
            static_cast<double>(millerL_) / extentZ,
        };

        const double cFractional = static_cast<double>(millerH_ + millerK_ + millerL_) * static_cast<double>(position_);
        const double offset = cFractional
                            + static_cast<double>(millerH_) * bounds[0] / extentX
                            + static_cast<double>(millerK_) * bounds[2] / extentY
                            + static_cast<double>(millerL_) * bounds[4] / extentZ;

        const double normalNorm2 = normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2];
        if (normalNorm2 <= 1e-8) {
            return;
        }
        const double scale = offset / normalNorm2;
        const double origin[3] = {normal[0] * scale, normal[1] * scale, normal[2] * scale};

        plane->SetNormal(normal);
        plane->SetOrigin(origin);
        break;
    }

    vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
    cutter->SetInputData(image);
    cutter->SetCutFunction(plane);
    cutter->Update();

    if (cutter->GetOutput() == nullptr || cutter->GetOutput()->GetNumberOfPoints() == 0) {
        return;
    }

    vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
    const double rangeMin = static_cast<double>(valueMin_);
    const double rangeMax = static_cast<double>(valueMax_);
    const double mid = (rangeMin + rangeMax) * 0.5;
    core::data::ApplyColorMapToTransferFunction(
        ctf,
        colorMap_,
        rangeMin,
        mid,
        rangeMax,
        static_cast<double>(colorMidpoint_),
        static_cast<double>(colorSharpness_));

    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetNumberOfTableValues(256);
    lut->SetRange(valueMin_, valueMax_);
    for (int i = 0; i < 256; ++i) {
        const double t = (i <= 0) ? 0.0 : static_cast<double>(i) / 255.0;
        const double x = rangeMin + (rangeMax - rangeMin) * t;
        double rgb[3] = {0.0, 0.0, 0.0};
        ctf->GetColor(x, rgb);
        lut->SetTableValue(i, rgb[0], rgb[1], rgb[2], 1.0);
    }
    lut->Build();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(cutter->GetOutputPort());
    mapper->SetLookupTable(lut);
    mapper->SetColorModeToMapScalars();
    mapper->SetScalarModeToUsePointData();
    mapper->SetScalarRange(valueMin_, valueMax_);

    sliceActor_ = vtkSmartPointer<vtkActor>::New();
    sliceActor_->SetMapper(mapper);
    sliceActor_->GetProperty()->SetLineWidth(1.5f);

    core::vtk::VtkViewer::Instance().AddActor(sliceActor_);
    core::vtk::VtkViewer::Instance().RequestRender();
}

void SliceRenderer::Clear() {
    if (sliceActor_ != nullptr) {
        core::vtk::VtkViewer::Instance().RemoveActor(sliceActor_);
        sliceActor_ = nullptr;
    }
    core::vtk::VtkViewer::Instance().RequestRender();
}

} // namespace features::data::slice
