#include "charge_density_controller.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace features::data::charge_density {
namespace {

std::string ExtractFileName(const std::string& filePath) {
    const size_t slash = filePath.find_last_of("/\\");
    if (slash == std::string::npos) {
        return filePath.empty() ? std::string("CHGCAR") : filePath;
    }
    const std::string name = filePath.substr(slash + 1);
    return name.empty() ? std::string("CHGCAR") : name;
}

} // namespace

ChargeDensityController::ChargeDensityController(core::scene::SceneState& scene)
    : scene_(scene) {
    scene_.events.onStructureRemoved.Subscribe(
        [this](const core::scene::StructureRemovedEvent& event) { handleStructureRemoved(event); });
}

void ChargeDensityController::Show(Mode mode) {
    mode_ = mode;
    RenderCurrentMode();
}

void ChargeDensityController::Clear() {
    isosurfaceRenderer_.Clear();
    volumeRenderer_.Clear();
    mode_ = Mode::None;
}

bool ChargeDensityController::LoadFromFile(const std::string& filePath) {
    std::unique_ptr<ChargeDensity> loaded = ChargeDensity::FromFile(filePath);
    if (loaded == nullptr) {
        statusMessage_ = "Failed to load CHGCAR file.";
        return false;
    }

    pushDataEntry(ExtractFileName(filePath), std::move(loaded));
    statusMessage_ = "CHGCAR file loaded successfully.";
    return true;
}

bool ChargeDensityController::LoadSampleData() {
    std::unique_ptr<ChargeDensity> sample = ChargeDensity::CreateSample();
    if (sample == nullptr) {
        statusMessage_ = "Failed to generate sample charge density data.";
        return false;
    }

    pushDataEntry("Sample Data", std::move(sample));
    statusMessage_ = "Sample charge density data loaded.";
    return true;
}

void ChargeDensityController::ClearData() {
    isosurfaceRenderer_.Clear();
    volumeRenderer_.Clear();
    data_.reset();
    dataEntries_.clear();
    activeDataIndex_ = -1;
    boundStructureId_ = -1;

    dataMin_ = 0.0f;
    dataMax_ = 1.0f;
    valueMin_ = 0.0f;
    valueMax_ = 1.0f;
    window_ = 1.0f;
    level_ = 0.5f;
    isoValue_ = 0.5f;
    positiveIsoValue_ = 0.2f;
    negativeIsoValue_ = -0.2f;
    showPrimaryIsosurface_ = true;
    showMultipleIsosurfaces_ = false;
    statusMessage_ = "No charge density data loaded.";
}

void ChargeDensityController::SetData(std::unique_ptr<ChargeDensity> data) {
    if (data == nullptr) {
        ClearData();
        return;
    }
    dataEntries_.clear();
    activeDataIndex_ = -1;
    pushDataEntry("Charge Density", std::move(data));
}

void ChargeDensityController::SetNamedData(const std::string& name, std::unique_ptr<ChargeDensity> data) {
    if (data == nullptr) {
        ClearData();
        return;
    }
    dataEntries_.clear();
    activeDataIndex_ = -1;
    pushDataEntry(name.empty() ? std::string("Charge Density") : name, std::move(data));
}

void ChargeDensityController::SetNamedDataEntries(
    std::vector<std::pair<std::string, std::unique_ptr<ChargeDensity>>> entries) {
    isosurfaceRenderer_.Clear();
    volumeRenderer_.Clear();
    data_.reset();
    dataEntries_.clear();
    activeDataIndex_ = -1;

    for (auto& entry : entries) {
        if (entry.second == nullptr) {
            continue;
        }
        std::string entryName = entry.first.empty() ? std::string("Grid") : std::move(entry.first);
        dataEntries_.emplace_back(std::move(entryName), std::move(entry.second));
    }

    if (dataEntries_.empty()) {
        ClearData();
        return;
    }

    setActiveDataByIndex(0);
    statusMessage_ = (dataEntries_.size() > 1)
        ? "XSF grid data loaded. Select Mesh to switch grids."
        : "XSF grid data loaded.";
}

std::unique_ptr<ChargeDensity> ChargeDensityController::CloneActiveData() const {
    if (data_ == nullptr) {
        return nullptr;
    }
    return std::make_unique<ChargeDensity>(*data_);
}

void ChargeDensityController::SetIsoValue(float value) {
    if (data_ == nullptr) {
        return;
    }
    isoValue_ = std::clamp(value, dataMin_, dataMax_);
    if (mode_ == Mode::Isosurface || mode_ == Mode::Surface) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetWireframe(bool enabled) {
    wireframe_ = enabled;
    if (mode_ == Mode::Isosurface) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetColorMap(core::data::ColorMapPreset preset) {
    colorMap_ = preset;
    if (mode_ == Mode::Surface || mode_ == Mode::Volumetric) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetOpacity(float opacity) {
    SetSurfaceOpacity(opacity);
}

void ChargeDensityController::SetValueRange(float minValue, float maxValue) {
    if (data_ == nullptr) {
        return;
    }

    valueMin_ = std::clamp(minValue, dataMin_, dataMax_);
    valueMax_ = std::clamp(maxValue, dataMin_, dataMax_);
    if (valueMax_ <= valueMin_) {
        valueMax_ = std::min(dataMax_, valueMin_ + 1e-6f);
    }

    window_ = std::max(1e-6f, valueMax_ - valueMin_);
    level_ = (valueMin_ + valueMax_) * 0.5f;
    if (isoValue_ < valueMin_ || isoValue_ > valueMax_) {
        isoValue_ = level_;
    }

    if (mode_ == Mode::Surface || mode_ == Mode::Volumetric) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetShowMultipleIsosurfaces(bool enabled) {
    showMultipleIsosurfaces_ = enabled;
    if (mode_ == Mode::Isosurface) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetPositiveIsoValue(float value) {
    if (data_ == nullptr) {
        return;
    }
    positiveIsoValue_ = std::clamp(value, dataMin_, dataMax_);
    if (mode_ == Mode::Isosurface) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetNegativeIsoValue(float value) {
    if (data_ == nullptr) {
        return;
    }
    negativeIsoValue_ = std::clamp(value, dataMin_, dataMax_);
    if (mode_ == Mode::Isosurface) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetPrimaryIsosurfaceVisible(bool visible) {
    showPrimaryIsosurface_ = visible;
    if (mode_ == Mode::Isosurface) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetIsoColor(const std::array<float, 4>& rgba) {
    isoColor_[0] = std::clamp(rgba[0], 0.0f, 1.0f);
    isoColor_[1] = std::clamp(rgba[1], 0.0f, 1.0f);
    isoColor_[2] = std::clamp(rgba[2], 0.0f, 1.0f);
    isoColor_[3] = std::clamp(rgba[3], 0.01f, 1.0f);
    if (mode_ == Mode::Isosurface) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetPositiveIsoColor(const std::array<float, 4>& rgba) {
    positiveIsoColor_[0] = std::clamp(rgba[0], 0.0f, 1.0f);
    positiveIsoColor_[1] = std::clamp(rgba[1], 0.0f, 1.0f);
    positiveIsoColor_[2] = std::clamp(rgba[2], 0.0f, 1.0f);
    positiveIsoColor_[3] = std::clamp(rgba[3], 0.01f, 1.0f);
    if (mode_ == Mode::Isosurface) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetNegativeIsoColor(const std::array<float, 4>& rgba) {
    negativeIsoColor_[0] = std::clamp(rgba[0], 0.0f, 1.0f);
    negativeIsoColor_[1] = std::clamp(rgba[1], 0.0f, 1.0f);
    negativeIsoColor_[2] = std::clamp(rgba[2], 0.0f, 1.0f);
    negativeIsoColor_[3] = std::clamp(rgba[3], 0.01f, 1.0f);
    if (mode_ == Mode::Isosurface) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetVolumeVisibility(bool visible) {
    volumeVisibility_ = visible;
    if (mode_ == Mode::Surface || mode_ == Mode::Volumetric) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetDirectApply(bool enabled) {
    directApply_ = enabled;
}

void ChargeDensityController::SetColorCurve(float midpoint, float sharpness) {
    colorCurveMidpoint_ = std::clamp(midpoint, 0.0f, 1.0f);
    colorCurveSharpness_ = std::clamp(sharpness, 0.0f, 1.0f);
    if (mode_ == Mode::Surface || mode_ == Mode::Volumetric) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetWindowLevel(float window, float level) {
    if (data_ != nullptr) {
        const float maxWindow = std::max(0.0f, dataMax_ - dataMin_);
        window_ = std::clamp(window, 0.0f, maxWindow);
        level_ = std::clamp(level, dataMin_, dataMax_);
        syncWindowLevelToRange();
        isoValue_ = std::clamp(level_, dataMin_, dataMax_);
    } else {
        window_ = std::max(0.0f, window);
        level_ = level;
    }

    if (mode_ == Mode::Surface || mode_ == Mode::Volumetric) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetQuality(int qualityIndex) {
    qualityIndex_ = std::clamp(qualityIndex, 0, 2);
    if (mode_ == Mode::Volumetric) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetSurfaceColor(const std::array<float, 3>& rgb) {
    surfaceColor_[0] = std::clamp(rgb[0], 0.0f, 1.0f);
    surfaceColor_[1] = std::clamp(rgb[1], 0.0f, 1.0f);
    surfaceColor_[2] = std::clamp(rgb[2], 0.0f, 1.0f);
    if (mode_ == Mode::Surface) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetSurfaceOpacity(float opacity) {
    surfaceOpacity_ = std::clamp(opacity, 0.01f, 1.0f);
    if (mode_ == Mode::Surface) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetSurfaceEdgeColor(const std::array<float, 3>& rgb) {
    surfaceEdgeColor_[0] = std::clamp(rgb[0], 0.0f, 1.0f);
    surfaceEdgeColor_[1] = std::clamp(rgb[1], 0.0f, 1.0f);
    surfaceEdgeColor_[2] = std::clamp(rgb[2], 0.0f, 1.0f);
    if (mode_ == Mode::Surface) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetSampleDistanceIndex(int index) {
    sampleDistanceIndex_ = std::clamp(index, 0, 5);
    if (mode_ == Mode::Volumetric) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetVolumeOpacityMin(float opacity) {
    volumeOpacityMin_ = std::clamp(opacity, 0.0f, 0.8f);
    if (volumeOpacityMin_ > volumeOpacityMax_) {
        volumeOpacityMax_ = volumeOpacityMin_;
    }
    if (mode_ == Mode::Volumetric) {
        RenderCurrentMode();
    }
}

void ChargeDensityController::SetVolumeOpacityMax(float opacity) {
    volumeOpacityMax_ = std::clamp(opacity, 0.0f, 0.8f);
    if (volumeOpacityMax_ < volumeOpacityMin_) {
        volumeOpacityMin_ = volumeOpacityMax_;
    }
    if (mode_ == Mode::Volumetric) {
        RenderCurrentMode();
    }
}

std::vector<std::string> ChargeDensityController::DataNames() const {
    std::vector<std::string> names;
    names.reserve(dataEntries_.size());
    for (const auto& entry : dataEntries_) {
        names.push_back(entry.first);
    }
    return names;
}

bool ChargeDensityController::SelectDataIndex(int index) {
    if (!setActiveDataByIndex(index)) {
        return false;
    }
    statusMessage_ = "Mesh data switched.";
    return true;
}

std::array<int, 3> ChargeDensityController::GridShape() const {
    if (data_ == nullptr) {
        return {0, 0, 0};
    }
    return data_->GridShape();
}

void ChargeDensityController::RenderCurrentMode() {
    isosurfaceRenderer_.Clear();
    volumeRenderer_.Clear();

    if (mode_ == Mode::None) {
        statusMessage_ = "Charge density view is hidden.";
        return;
    }
    if (data_ == nullptr) {
        statusMessage_ = "No charge density data loaded.";
        return;
    }

    switch (mode_) {
    case Mode::Isosurface: {
        bool rendered = false;
        if (showPrimaryIsosurface_) {
            isosurfaceRenderer_.Render(
                *data_, isoValue_, wireframe_, isoColor_[3], {isoColor_[0], isoColor_[1], isoColor_[2]});
            rendered = true;
        }
        if (showMultipleIsosurfaces_) {
            isosurfaceRenderer_.RenderMultiple(*data_,
                                               positiveIsoValue_,
                                               negativeIsoValue_,
                                               wireframe_,
                                               std::max(positiveIsoColor_[3], negativeIsoColor_[3]),
                                               {positiveIsoColor_[0], positiveIsoColor_[1], positiveIsoColor_[2]},
                                               {negativeIsoColor_[0], negativeIsoColor_[1], negativeIsoColor_[2]});
            rendered = true;
        }
        statusMessage_ = rendered ? "Isosurface mode rendered." : "Isosurface mode hidden.";
        break;
    }
    case Mode::Surface:
        if (!volumeVisibility_) {
            statusMessage_ = "Surface mode hidden (Visibility off).";
            break;
        }
        isosurfaceRenderer_.RenderSurface(
            *data_,
            std::clamp(level_, dataMin_, dataMax_),
            surfaceColor_,
            surfaceOpacity_,
            surfaceEdgeColor_);
        statusMessage_ = "Surface mode rendered.";
        break;
    case Mode::Volumetric:
        if (!volumeVisibility_) {
            statusMessage_ = "Volumetric mode hidden (Visibility off).";
            break;
        }
        volumeRenderer_.Render(*data_,
                               colorMap_,
                               valueMin_,
                               valueMax_,
                               colorCurveMidpoint_,
                               colorCurveSharpness_,
                               sampleDistanceIndex_,
                               volumeOpacityMin_,
                               volumeOpacityMax_,
                               qualityIndex_);
        statusMessage_ = "Volumetric mode rendered.";
        break;
    case Mode::None:
    default:
        break;
    }
}

bool ChargeDensityController::setActiveDataByIndex(int index) {
    if (index < 0 || index >= static_cast<int>(dataEntries_.size())) {
        return false;
    }
    if (dataEntries_[static_cast<size_t>(index)].second == nullptr) {
        return false;
    }

    activeDataIndex_ = index;
    data_ = std::make_unique<ChargeDensity>(*dataEntries_[static_cast<size_t>(index)].second);
    const auto [minVal, maxVal] = data_->ValueRange();
    dataMin_ = minVal;
    dataMax_ = (maxVal > minVal) ? maxVal : (minVal + 1.0f);
    window_ = std::max(0.0f, dataMax_ - dataMin_);
    level_ = (dataMin_ + dataMax_) * 0.5f;
    syncWindowLevelToRange();
    isoValue_ = std::clamp(level_, dataMin_, dataMax_);

    const float absPeak = std::max(std::abs(dataMin_), std::abs(dataMax_));
    positiveIsoValue_ = std::clamp(absPeak * 0.1f, dataMin_, dataMax_);
    negativeIsoValue_ = std::clamp(-absPeak * 0.1f, dataMin_, dataMax_);

    boundStructureId_ = scene_.currentStructureId;
    if (mode_ != Mode::None) {
        RenderCurrentMode();
    }
    return true;
}

void ChargeDensityController::pushDataEntry(const std::string& name, std::unique_ptr<ChargeDensity> data) {
    if (data == nullptr) {
        return;
    }
    dataEntries_.push_back({name.empty() ? std::string("CHGCAR") : name, std::move(data)});
    setActiveDataByIndex(static_cast<int>(dataEntries_.size() - 1));
}

void ChargeDensityController::syncWindowLevelToRange() {
    if (data_ == nullptr) {
        valueMin_ = 0.0f;
        valueMax_ = 1.0f;
        return;
    }

    const float half = std::max(0.0f, window_) * 0.5f;
    valueMin_ = level_ - half;
    valueMax_ = level_ + half;
    valueMin_ = std::clamp(valueMin_, dataMin_, dataMax_);
    valueMax_ = std::clamp(valueMax_, dataMin_, dataMax_);
    if (valueMax_ <= valueMin_) {
        valueMax_ = std::min(dataMax_, valueMin_ + 1e-6f);
    }
}

void ChargeDensityController::handleStructureRemoved(const core::scene::StructureRemovedEvent& event) {
    if (boundStructureId_ >= 0 && event.structureId == boundStructureId_) {
        ClearData();
    }
}

} // namespace features::data::charge_density
