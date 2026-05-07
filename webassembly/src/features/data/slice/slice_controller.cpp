#include "slice_controller.h"

#include <algorithm>
#include <utility>

namespace features::data::slice {

SliceController::SliceController(core::scene::SceneState& scene)
    : scene_(scene) {
    scene_.events.onStructureRemoved.Subscribe(
        [this](const core::scene::StructureRemovedEvent& event) { handleStructureRemoved(event); });
}

void SliceController::Show() {
    visible_ = true;
    RenderCurrent();
}

void SliceController::Hide() {
    visible_ = false;
    renderer_.Clear();
}

void SliceController::Clear() {
    renderer_.Clear();
}

bool SliceController::LoadFromFile(const std::string& filePath) {
    std::unique_ptr<features::data::charge_density::ChargeDensity> loaded =
        features::data::charge_density::ChargeDensity::FromFile(filePath);
    if (loaded == nullptr) {
        statusMessage_ = "Failed to load CHGCAR file for slice.";
        return false;
    }

    SetData(std::move(loaded));
    statusMessage_ = "CHGCAR file loaded for slice.";
    return true;
}

bool SliceController::LoadSampleData() {
    std::unique_ptr<features::data::charge_density::ChargeDensity> sample =
        features::data::charge_density::ChargeDensity::CreateSample();
    if (sample == nullptr) {
        statusMessage_ = "Failed to generate sample data for slice.";
        return false;
    }

    SetData(std::move(sample));
    statusMessage_ = "Sample data loaded for slice.";
    return true;
}

void SliceController::ClearData() {
    data_.reset();
    renderer_.Clear();
    boundStructureId_ = -1;
    dataMin_ = 0.0f;
    dataMax_ = 1.0f;
    valueMin_ = 0.0f;
    valueMax_ = 1.0f;
    colorCurveMidpoint_ = 0.5f;
    colorCurveSharpness_ = 0.0f;
    statusMessage_ = "No slice data loaded.";
}

void SliceController::SetData(std::unique_ptr<features::data::charge_density::ChargeDensity> data) {
    data_ = std::move(data);
    if (data_ == nullptr) {
        ClearData();
        return;
    }

    const auto [minVal, maxVal] = data_->ValueRange();
    dataMin_ = minVal;
    dataMax_ = (maxVal > minVal) ? maxVal : (minVal + 1.0f);
    valueMin_ = dataMin_;
    valueMax_ = dataMax_;

    boundStructureId_ = scene_.currentStructureId;
    statusMessage_ = "Slice data loaded.";

    if (visible_) {
        RenderCurrent();
    }
}

void SliceController::SetPlane(SlicePlane plane) {
    plane_ = plane;
    if (visible_) {
        RenderCurrent();
    }
}

void SliceController::SetPosition(float position) {
    position_ = std::clamp(position, 0.0f, 1.0f);
    if (visible_) {
        RenderCurrent();
    }
}

void SliceController::SetMillerIndices(int h, int k, int l) {
    millerH_ = std::clamp(h, 0, 6);
    millerK_ = std::clamp(k, 0, 6);
    millerL_ = std::clamp(l, 0, 6);
    plane_ = SlicePlane::Miller;
    if (visible_) {
        RenderCurrent();
    }
}

void SliceController::SetColorMap(core::data::ColorMapPreset preset) {
    colorMap_ = preset;
    if (visible_) {
        RenderCurrent();
    }
}

void SliceController::SetValueRange(float minValue, float maxValue) {
    if (data_ == nullptr) {
        return;
    }
    valueMin_ = std::clamp(minValue, dataMin_, dataMax_);
    valueMax_ = std::clamp(maxValue, dataMin_, dataMax_);
    if (valueMax_ <= valueMin_) {
        valueMax_ = std::min(dataMax_, valueMin_ + 1e-6f);
    }
    if (visible_) {
        RenderCurrent();
    }
}

void SliceController::SetColorCurve(float midpoint, float sharpness) {
    colorCurveMidpoint_ = std::clamp(midpoint, 0.0f, 1.0f);
    colorCurveSharpness_ = std::clamp(sharpness, 0.0f, 1.0f);
    if (visible_) {
        RenderCurrent();
    }
}

void SliceController::SetQuality(int qualityIndex) {
    qualityIndex_ = std::clamp(qualityIndex, 0, 2);
}

std::array<int, 3> SliceController::GridShape() const {
    if (data_ == nullptr) {
        return {0, 0, 0};
    }
    return data_->GridShape();
}

void SliceController::RenderCurrent() {
    renderer_.Clear();
    if (!visible_) {
        return;
    }
    if (data_ == nullptr) {
        statusMessage_ = "No charge density data loaded.";
        return;
    }

    renderer_.SetPlane(plane_, position_);
    renderer_.SetMillerIndices(millerH_, millerK_, millerL_);
    renderer_.SetColorMap(colorMap_);
    renderer_.SetValueRange(valueMin_, valueMax_);
    renderer_.SetColorCurve(colorCurveMidpoint_, colorCurveSharpness_);
    renderer_.Render(*data_);
    statusMessage_ = "Slice rendered.";
}

void SliceController::handleStructureRemoved(const core::scene::StructureRemovedEvent& event) {
    if (boundStructureId_ >= 0 && event.structureId == boundStructureId_) {
        ClearData();
    }
}

} // namespace features::data::slice
