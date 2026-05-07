#include "charge_density_ui.h"

#include "charge_density_controller.h"
#include "../slice/slice_controller.h"

#include "core/data/colormap.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace features::data::charge_density {
namespace {

constexpr const char* kModeItems = "Isosurface\0Surface\0Volumetric\0";
constexpr const char* kQualityLabels[] = {"Low", "Medium", "High"};
constexpr const char* kSampleDistanceLabels[] = {"0.1x", "0.5x", "1.0x", "2.0x", "5.0x", "10.0x"};

} // namespace

ChargeDensityUI::ChargeDensityUI(ChargeDensityController& controller,
                                 features::data::slice::SliceController* sliceController)
    : controller_(controller),
      sliceController_(sliceController) {
    hadData_ = controller_.HasData();
    SyncFromController();
}

void ChargeDensityUI::Render(bool* open) {
    if (open != nullptr && !(*open)) {
        return;
    }

    if (!ImGui::Begin("Charge Density Viewer", open)) {
        ImGui::End();
        return;
    }

    if (controller_.HasData() != hadData_) {
        hadData_ = controller_.HasData();
        SyncFromController();
    } else {
        const bool hasPendingEdits = sharedSettingsDirty_ || surfaceSettingsDirty_ || volumetricSettingsDirty_;
        if (!hasPendingEdits || directApplyInput_) {
            SyncFromController();
        }
    }

    UpdateAnimation();

    RenderFileSection();
    ImGui::Separator();
    RenderInfoSection();
    ImGui::Separator();
    RenderModeSection();

    ImGui::End();
}

void ChargeDensityUI::SyncFromController() {
    const Mode mode = controller_.CurrentMode();
    modeIndex_ = std::clamp(static_cast<int>(mode) - 1, 0, 2);

    isoValueInput_ = controller_.IsoValue();
    showIsosurface_ = controller_.PrimaryIsosurfaceVisible();
    showMultipleIsosurfaces_ = controller_.ShowMultipleIsosurfaces();
    positiveIsoValueInput_ = controller_.PositiveIsoValue();
    negativeIsoValueInput_ = controller_.NegativeIsoValue();
    wireframeInput_ = controller_.Wireframe();

    const auto& isoColor = controller_.IsoColor();
    isoColorInput_[0] = isoColor[0];
    isoColorInput_[1] = isoColor[1];
    isoColorInput_[2] = isoColor[2];
    isoColorInput_[3] = isoColor[3];

    const auto& posColor = controller_.PositiveIsoColor();
    positiveIsoColorInput_[0] = posColor[0];
    positiveIsoColorInput_[1] = posColor[1];
    positiveIsoColorInput_[2] = posColor[2];
    positiveIsoColorInput_[3] = posColor[3];

    const auto& negColor = controller_.NegativeIsoColor();
    negativeIsoColorInput_[0] = negColor[0];
    negativeIsoColorInput_[1] = negColor[1];
    negativeIsoColorInput_[2] = negColor[2];
    negativeIsoColorInput_[3] = negColor[3];

    volumeVisibilityInput_ = controller_.VolumeVisibility();
    directApplyInput_ = controller_.DirectApply();
    colorMapIndex_ = core::data::ColorMapPresetToIndex(controller_.ColorMap());
    colorCurveMidpointInput_ = controller_.ColorCurveMidpoint();
    colorCurveSharpnessInput_ = controller_.ColorCurveSharpness();
    windowInput_ = controller_.Window();
    levelInput_ = controller_.Level();
    qualityIndexInput_ = controller_.Quality();

    const auto& surfaceColor = controller_.SurfaceColor();
    surfaceColorInput_[0] = surfaceColor[0];
    surfaceColorInput_[1] = surfaceColor[1];
    surfaceColorInput_[2] = surfaceColor[2];
    surfaceOpacityInput_ = controller_.SurfaceOpacity();

    const auto& surfaceEdgeColor = controller_.SurfaceEdgeColor();
    surfaceEdgeColorInput_[0] = surfaceEdgeColor[0];
    surfaceEdgeColorInput_[1] = surfaceEdgeColor[1];
    surfaceEdgeColorInput_[2] = surfaceEdgeColor[2];

    sampleDistanceIndexInput_ = controller_.SampleDistanceIndex();
    volumeOpacityMinInput_ = controller_.VolumeOpacityMin();
    volumeOpacityMaxInput_ = controller_.VolumeOpacityMax();

    activeDataIndex_ = controller_.ActiveDataIndex();
}

void ChargeDensityUI::RenderFileSection() {
    ImGui::InputText("CHGCAR Path", filePathInput_, sizeof(filePathInput_));
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        if (controller_.LoadFromFile(filePathInput_)) {
            if (controller_.CurrentMode() == Mode::None) {
                controller_.Show(Mode::Isosurface);
            }
            SyncFromController();
            SyncSliceSharedSettings();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        controller_.ClearData();
        controller_.Clear();
        SyncFromController();
        SyncSliceSharedSettings();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Sample Data")) {
        if (controller_.LoadSampleData()) {
            if (controller_.CurrentMode() == Mode::None) {
                controller_.Show(Mode::Isosurface);
            }
            SyncFromController();
            SyncSliceSharedSettings();
        }
    }

    const std::vector<std::string> meshNames = controller_.DataNames();
    if (meshNames.size() > 1) {
        std::vector<const char*> labels;
        labels.reserve(meshNames.size());
        for (const std::string& name : meshNames) {
            labels.push_back(name.c_str());
        }
        int selected = std::clamp(activeDataIndex_, 0, static_cast<int>(meshNames.size() - 1));
        ImGui::Text("Mesh");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::Combo("##MeshSelector", &selected, labels.data(), static_cast<int>(labels.size()))) {
            if (controller_.SelectDataIndex(selected)) {
                SyncFromController();
                SyncSliceSharedSettings();
            }
        }
    }

    const std::string status = controller_.StatusMessage();
    if (!status.empty()) {
        ImGui::TextWrapped("%s", status.c_str());
    }

    if (!controller_.HasData()) {
        ImGui::TextDisabled("No charge density data loaded.");
    }
}

void ChargeDensityUI::RenderInfoSection() const {
    if (!ImGui::TreeNodeEx("Data Information", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    const std::array<int, 3> grid = controller_.GridShape();
    ImGui::Text("Grid Size: %d x %d x %d", grid[0], grid[1], grid[2]);
    ImGui::Text("Value Range:");
    ImGui::Text("  Min: %.4e", controller_.DataMin());
    ImGui::Text("  Max: %.4e", controller_.DataMax());
    ImGui::TreePop();
}

void ChargeDensityUI::RenderModeSection() {
    int selectedMode = modeIndex_;
    if (ImGui::Combo("Mode", &selectedMode, kModeItems)) {
        modeIndex_ = selectedMode;
        controller_.Show(static_cast<Mode>(modeIndex_ + 1));
        SyncFromController();
        SyncSliceSharedSettings();
    }

    if (!controller_.HasData()) {
        return;
    }

    const Mode mode = controller_.CurrentMode();
    if (mode == Mode::Isosurface) {
        RenderIsosurfaceControls();
    } else if (mode == Mode::Surface) {
        RenderSharedOptionsTable();
        ImGui::Separator();
        RenderSurfaceOptions();
    } else if (mode == Mode::Volumetric) {
        RenderSharedOptionsTable();
        ImGui::Separator();
        RenderVolumetricOptions();
    }
}

void ChargeDensityUI::RenderIsosurfaceControls() {
    if (ImGui::Checkbox("Show Isosurface", &showIsosurface_)) {
        controller_.SetPrimaryIsosurfaceVisible(showIsosurface_);
    }

    if (showIsosurface_) {
        if (ImGui::SliderFloat("Iso Value", &isoValueInput_, controller_.DataMin(), controller_.DataMax(), "%.4e")) {
            controller_.SetIsoValue(isoValueInput_);
        }

        const float valueSpan = controller_.DataMax() - controller_.DataMin();
        if (valueSpan > 0.0f) {
            float percentage = (isoValueInput_ - controller_.DataMin()) / valueSpan * 100.0f;
            if (ImGui::SliderFloat("Level (%)", &percentage, 0.0f, 100.0f, "%.1f%%")) {
                isoValueInput_ = controller_.DataMin() + valueSpan * percentage / 100.0f;
                controller_.SetIsoValue(isoValueInput_);
            }
        }

        if (ImGui::ColorEdit4("Color##iso", isoColorInput_,
                              ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview)) {
            controller_.SetIsoColor({isoColorInput_[0], isoColorInput_[1], isoColorInput_[2], isoColorInput_[3]});
        }

        if (ImGui::Checkbox("Wireframe", &wireframeInput_)) {
            controller_.SetWireframe(wireframeInput_);
        }
    }

    if (ImGui::Checkbox("Show +/- Isosurfaces", &showMultipleIsosurfaces_)) {
        controller_.SetShowMultipleIsosurfaces(showMultipleIsosurfaces_);
    }

    if (showMultipleIsosurfaces_) {
        const float dataMin = controller_.DataMin();
        const float dataMax = controller_.DataMax();

        const float posMin = std::max(0.0f, dataMin);
        const float posMax = std::max(posMin, dataMax);
        if (ImGui::SliderFloat("+ Value", &positiveIsoValueInput_, posMin, posMax, "%.4e")) {
            controller_.SetPositiveIsoValue(positiveIsoValueInput_);
        }
        if (ImGui::ColorEdit4("+ Color", positiveIsoColorInput_,
                              ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview)) {
            controller_.SetPositiveIsoColor(
                {positiveIsoColorInput_[0], positiveIsoColorInput_[1], positiveIsoColorInput_[2], positiveIsoColorInput_[3]});
        }

        const float negMax = std::min(0.0f, dataMax);
        const float negMin = std::min(dataMin, negMax);
        if (ImGui::SliderFloat("- Value", &negativeIsoValueInput_, negMin, negMax, "%.4e")) {
            controller_.SetNegativeIsoValue(negativeIsoValueInput_);
        }
        if (ImGui::ColorEdit4("- Color", negativeIsoColorInput_,
                              ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview)) {
            controller_.SetNegativeIsoColor(
                {negativeIsoColorInput_[0], negativeIsoColorInput_[1], negativeIsoColorInput_[2], negativeIsoColorInput_[3]});
        }
    }

    if (ImGui::Checkbox("Animate", &animate_)) {
        // state only; animated updates happen in UpdateAnimation()
    }
    if (animate_) {
        ImGui::SliderFloat("Animation Speed", &animateSpeed_, 0.05f, 2.0f, "%.2f");
    }
}

void ChargeDensityUI::RenderSharedOptionsTable() {
    const float maxWindow = std::max(0.0f, controller_.DataMax() - controller_.DataMin());
    bool changed = false;

    if (ImGui::BeginTable("SurfaceVolumeShared", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Option", ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Visibility");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##SharedVisibility", &volumeVisibilityInput_)) {
            changed = true;
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Direct Apply");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##SharedDirectApply", &directApplyInput_)) {
            controller_.SetDirectApply(directApplyInput_);
            if (directApplyInput_) {
                ApplySharedSettings();
            }
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Colormap");
        ImGui::TableNextColumn();
        const char* const* labels = core::data::GetColorMapPresetLabels();
        if (ImGui::Combo("##SharedColormap", &colorMapIndex_, labels, core::data::kColorMapPresetCount)) {
            changed = true;
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Color Curve");
        ImGui::TableNextColumn();
        changed |= ImGui::SliderFloat("Midpoint##SharedCurve", &colorCurveMidpointInput_, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::SliderFloat("Sharpness##SharedCurve", &colorCurveSharpnessInput_, 0.0f, 1.0f, "%.2f");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Level/Window");
        ImGui::TableNextColumn();
        changed |= ImGui::SliderFloat("Level##SharedWindowLevel",
                                      &levelInput_,
                                      controller_.DataMin(),
                                      controller_.DataMax(),
                                      "%.4e");
        changed |= ImGui::SliderFloat("Window##SharedWindowLevel", &windowInput_, 0.0f, maxWindow, "%.4e");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Quality");
        ImGui::TableNextColumn();
        changed |= ImGui::SliderInt("##SharedQuality", &qualityIndexInput_, 0, 2);
        ImGui::SameLine();
        ImGui::Text("%s", kQualityLabels[std::clamp(qualityIndexInput_, 0, 2)]);

        ImGui::EndTable();
    }

    if (changed) {
        sharedSettingsDirty_ = true;
        if (directApplyInput_) {
            ApplySharedSettings();
        }
    }

    if (!directApplyInput_ && sharedSettingsDirty_) {
        if (ImGui::Button("Apply Shared Settings")) {
            ApplySharedSettings();
        }
    }
}

void ChargeDensityUI::RenderSurfaceOptions() {
    bool changed = false;

    if (ImGui::BeginTable("SurfaceOnly", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Option", ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Color");
        ImGui::TableNextColumn();
        changed |= ImGui::ColorEdit3("##SurfaceColor", surfaceColorInput_, ImGuiColorEditFlags_Float);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Opacity");
        ImGui::TableNextColumn();
        changed |= ImGui::SliderFloat("##SurfaceOpacity", &surfaceOpacityInput_, 0.01f, 1.0f, "%.3f");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Edge Color");
        ImGui::TableNextColumn();
        changed |= ImGui::ColorEdit3("##SurfaceEdgeColor", surfaceEdgeColorInput_, ImGuiColorEditFlags_Float);

        ImGui::EndTable();
    }

    if (changed) {
        surfaceSettingsDirty_ = true;
        if (directApplyInput_) {
            ApplySurfaceSettings();
        }
    }
    if (!directApplyInput_ && surfaceSettingsDirty_) {
        if (ImGui::Button("Apply Surface Settings")) {
            ApplySurfaceSettings();
        }
    }
}

void ChargeDensityUI::RenderVolumetricOptions() {
    bool changed = false;

    if (ImGui::BeginTable("VolumetricOnly", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Option", ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Sample Distance");
        ImGui::TableNextColumn();
        changed |= ImGui::SliderInt("##VolumeSampleDistance", &sampleDistanceIndexInput_, 0, 5);
        ImGui::SameLine();
        ImGui::Text("%s", kSampleDistanceLabels[std::clamp(sampleDistanceIndexInput_, 0, 5)]);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Opacity (Min)");
        ImGui::TableNextColumn();
        changed |= ImGui::DragFloat("##VolumeOpacityMin", &volumeOpacityMinInput_, 0.005f, 0.0f, 0.8f, "%.3f");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Opacity (Max)");
        ImGui::TableNextColumn();
        changed |= ImGui::DragFloat("##VolumeOpacityMax", &volumeOpacityMaxInput_, 0.005f, 0.0f, 0.8f, "%.3f");

        ImGui::EndTable();
    }

    if (changed) {
        volumetricSettingsDirty_ = true;
        if (directApplyInput_) {
            ApplyVolumetricSettings();
        }
    }
    if (!directApplyInput_ && volumetricSettingsDirty_) {
        if (ImGui::Button("Apply Volumetric Settings")) {
            ApplyVolumetricSettings();
        }
    }
}

void ChargeDensityUI::ApplySharedSettings() {
    controller_.SetVolumeVisibility(volumeVisibilityInput_);
    controller_.SetDirectApply(directApplyInput_);
    controller_.SetColorMap(core::data::ColorMapPresetFromIndex(colorMapIndex_));
    controller_.SetColorCurve(colorCurveMidpointInput_, colorCurveSharpnessInput_);
    controller_.SetWindowLevel(windowInput_, levelInput_);
    controller_.SetQuality(qualityIndexInput_);
    sharedSettingsDirty_ = false;
    SyncSliceSharedSettings();
}

void ChargeDensityUI::ApplySurfaceSettings() {
    controller_.SetSurfaceColor({surfaceColorInput_[0], surfaceColorInput_[1], surfaceColorInput_[2]});
    controller_.SetSurfaceOpacity(surfaceOpacityInput_);
    controller_.SetSurfaceEdgeColor({surfaceEdgeColorInput_[0], surfaceEdgeColorInput_[1], surfaceEdgeColorInput_[2]});
    surfaceSettingsDirty_ = false;
}

void ChargeDensityUI::ApplyVolumetricSettings() {
    controller_.SetSampleDistanceIndex(sampleDistanceIndexInput_);
    controller_.SetVolumeOpacityMin(volumeOpacityMinInput_);
    controller_.SetVolumeOpacityMax(volumeOpacityMaxInput_);
    volumetricSettingsDirty_ = false;
}

void ChargeDensityUI::SyncSliceSharedSettings() const {
    if (sliceController_ == nullptr) {
        return;
    }
    if (!controller_.VolumeVisibility() && sliceController_->IsVisible()) {
        sliceController_->Hide();
    }
    sliceController_->SetColorMap(controller_.ColorMap());
    sliceController_->SetValueRange(controller_.ValueMin(), controller_.ValueMax());
    sliceController_->SetColorCurve(controller_.ColorCurveMidpoint(), controller_.ColorCurveSharpness());
    sliceController_->SetQuality(controller_.Quality());
}

void ChargeDensityUI::UpdateAnimation() {
    if (!animate_ || !controller_.HasData()) {
        return;
    }
    if (controller_.CurrentMode() != Mode::Isosurface) {
        return;
    }

    const float minValue = controller_.DataMin();
    const float maxValue = controller_.DataMax();
    const float span = maxValue - minValue;
    if (span <= 0.0f) {
        return;
    }

    const float t = static_cast<float>(ImGui::GetTime()) * animateSpeed_;
    const float phase = std::sin(t) * 0.5f + 0.5f;
    isoValueInput_ = minValue + span * phase;
    controller_.SetIsoValue(isoValueInput_);
}

} // namespace features::data::charge_density
