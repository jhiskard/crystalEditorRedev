#include "measurement_overlay_ui.h"

#include "measurement_controller.h"
#include "measurement_store.h"

#include "core/scene/scene_state.h"

#include <imgui.h>

#include <array>
#include <string>

namespace features::measurement {

void MeasurementOverlayUI::Render(
    MeasurementController& controller,
    MeasurementStore& store,
    bool* showListWindow) {
    if (controller.IsActive()) {
        RenderModeOverlay(controller);
    }
    if (showListWindow != nullptr && *showListWindow) {
        RenderListWindow(store, showListWindow);
    }
}

void MeasurementOverlayUI::RenderModeOverlay(MeasurementController& controller) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 24.0f, viewport->WorkPos.y + 76.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.78f);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("Measurement Mode Overlay", nullptr, flags)) {
        const MeasurementMode mode = controller.CurrentMode();
        const bool centerMode = IsCenterMode(mode);
        const size_t pickedCount = controller.PickedAtomIds().size();
        const size_t targetCount = TargetPickCount(mode);

        ImGui::Text("Measurement: %s", ModeLabel(mode));
        if (centerMode) {
            ImGui::Text("Selected: %zu atoms", pickedCount);
            ImGui::TextUnformatted("Drag selects multiple atoms. Shift/Ctrl adds to selection.");
        } else {
            ImGui::Text("Picked: %zu / %zu atoms", pickedCount, targetCount);
        }

        if (ImGui::Button("Exit")) {
            controller.ExitMode();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            controller.ClearPickedAtoms();
        }
        if (centerMode) {
            ImGui::SameLine();
            ImGui::BeginDisabled(pickedCount < 2);
            if (ImGui::Button("Apply")) {
                controller.ApplyCenterMeasurement();
            }
            ImGui::EndDisabled();
        }

        const ImGuiIO& io = ImGui::GetIO();
        const bool typing = io.WantTextInput || ImGui::IsAnyItemActive();
        if (!typing && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            controller.ExitMode();
        }
        if (!typing && centerMode && pickedCount >= 2 &&
            (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
             ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))) {
            controller.ApplyCenterMeasurement();
        }
    }
    ImGui::End();
}

void MeasurementOverlayUI::RenderListWindow(MeasurementStore& store, bool* showListWindow) {
    if (!ImGui::Begin("Measurement", showListWindow)) {
        ImGui::End();
        return;
    }

    const auto items = store.MeasurementsForStructure(-1);
    if (items.empty()) {
        ImGui::TextUnformatted("No measurements for the active structure.");
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear Active Structure")) {
        if (!items.empty()) {
            store.RemoveByStructure(items.front().structureId);
            ImGui::End();
            return;
        }
    }
    ImGui::Separator();

    if (ImGui::BeginTable("MeasurementList", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Visible", ImGuiTableColumnFlags_WidthFixed, 64.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Remove", ImGuiTableColumnFlags_WidthFixed, 72.0f);
        ImGui::TableHeadersRow();

        for (const MeasurementListItem& item : items) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            bool visible = item.visible;
            const std::string visibleId = "##visible_" + std::to_string(item.id);
            if (ImGui::Checkbox(visibleId.c_str(), &visible)) {
                store.SetVisible(item.id, visible);
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(TypeLabel(item.type));

            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(item.displayName.c_str());

            ImGui::TableSetColumnIndex(3);
            const std::string removeId = "Remove##measurement_" + std::to_string(item.id);
            if (ImGui::SmallButton(removeId.c_str())) {
                store.Remove(item.id);
            }
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

} // namespace features::measurement
