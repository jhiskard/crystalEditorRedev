#include "volume_renderer.h"

#include "core/vtk/vtk_viewer.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

#include <algorithm>
#include <array>

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

} // namespace

void VolumeRenderer::Render(const ChargeDensity& cd,
                            core::data::ColorMapPreset preset,
                            float rangeMin,
                            float rangeMax,
                            float colorMidpoint,
                            float colorSharpness,
                            int sampleDistanceIndex,
                            float opacityMin,
                            float opacityMax,
                            int qualityIndex) {
    Clear();

    vtkSmartPointer<vtkImageData> image = BuildImageData(cd);
    if (image == nullptr) {
        return;
    }

    if (rangeMax <= rangeMin) {
        rangeMax = rangeMin + 1.0f;
    }
    colorMidpoint = std::clamp(colorMidpoint, 0.0f, 1.0f);
    colorSharpness = std::clamp(colorSharpness, 0.0f, 1.0f);
    opacityMin = std::clamp(opacityMin, 0.0f, 0.8f);
    opacityMax = std::clamp(opacityMax, 0.0f, 0.8f);
    if (opacityMax < opacityMin) {
        opacityMin = opacityMax;
    }

    vtkSmartPointer<vtkSmartVolumeMapper> mapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
    mapper->SetInputData(image);
    mapper->SetBlendModeToComposite();
    mapper->SetAutoAdjustSampleDistances(0);

    static constexpr std::array<double, 6> kSampleDistanceRatios = {0.1, 0.5, 1.0, 2.0, 5.0, 10.0};
    sampleDistanceIndex = std::clamp(sampleDistanceIndex, 0, static_cast<int>(kSampleDistanceRatios.size() - 1));
    qualityIndex = std::clamp(qualityIndex, 0, 2);

    const double spacing[3] = {
        image->GetSpacing()[0],
        image->GetSpacing()[1],
        image->GetSpacing()[2],
    };
    const double baseSpacing = std::max(1e-6, (spacing[0] + spacing[1] + spacing[2]) / 3.0);
    const double qualityMultiplier = (qualityIndex == 0) ? 1.6 : (qualityIndex == 1 ? 1.0 : 0.6);
    mapper->SetSampleDistance(baseSpacing * kSampleDistanceRatios[static_cast<size_t>(sampleDistanceIndex)] * qualityMultiplier);

    vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
    const double mid = (static_cast<double>(rangeMin) + static_cast<double>(rangeMax)) * 0.5;
    core::data::ApplyColorMapToTransferFunction(
        ctf,
        preset,
        static_cast<double>(rangeMin),
        mid,
        static_cast<double>(rangeMax),
        static_cast<double>(colorMidpoint),
        static_cast<double>(colorSharpness));

    vtkSmartPointer<vtkPiecewiseFunction> otf = vtkSmartPointer<vtkPiecewiseFunction>::New();
    otf->AddPoint(rangeMin, opacityMin);
    otf->AddPoint((rangeMin + mid) * 0.5, opacityMin);
    otf->AddPoint(mid, (opacityMin + opacityMax) * 0.5f);
    otf->AddPoint((mid + rangeMax) * 0.5, opacityMax);
    otf->AddPoint(rangeMax, opacityMax);

    vtkSmartPointer<vtkVolumeProperty> property = vtkSmartPointer<vtkVolumeProperty>::New();
    property->SetColor(ctf);
    property->SetScalarOpacity(otf);
    property->SetInterpolationTypeToLinear();
    property->ShadeOff();

    volume_ = vtkSmartPointer<vtkVolume>::New();
    volume_->SetMapper(mapper);
    volume_->SetProperty(property);

    vtkRenderer* renderer = core::vtk::VtkViewer::Instance().GetRenderer();
    if (renderer != nullptr) {
        renderer->AddVolume(volume_);
    }
    core::vtk::VtkViewer::Instance().RequestRender();
}

void VolumeRenderer::Clear() {
    if (volume_ != nullptr) {
        vtkRenderer* renderer = core::vtk::VtkViewer::Instance().GetRenderer();
        if (renderer != nullptr) {
            renderer->RemoveVolume(volume_);
        }
        volume_ = nullptr;
        core::vtk::VtkViewer::Instance().RequestRender();
    }
}

} // namespace features::data::charge_density
