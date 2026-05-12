#include "structure_import_ui.h"

#include "structure_import_controller.h"

#include <imgui.h>

#include <cfloat>
#include <cstdio>
#include <string>

namespace features::file {
namespace {

constexpr const char* kReplacePopupTitle = "Clear Current Data and Import?";
constexpr const char* kErrorPopupTitle = "Import Structure Failed";
constexpr const char* kProgressPopupTitle = "Loading structure file";

} // namespace

void StructureImportUI::Render(StructureImportController& controller) {
    if (controller.ConsumeReplacePopupOpen()) {
        ImGui::OpenPopup(kReplacePopupTitle);
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2(520.0f, 0.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::BeginPopupModal(kReplacePopupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Viewer is not empty.");
        ImGui::TextWrapped("Do you want to clear all current data and continue importing?");

        if (ImGui::Button("Yes")) {
            controller.ConfirmReplace();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No")) {
            controller.CancelReplace();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    const ImportProgressState& progress = controller.Progress();
    const char* progressPopupTitle = progress.title.empty() ? kProgressPopupTitle : progress.title.c_str();
    if (progress.visible) {
        ImGui::OpenPopup(progressPopupTitle);
    }
    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(progressPopupTitle,
                               nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        if (!progress.text.empty()) {
            ImGui::TextWrapped("%s", progress.text.c_str());
        }
        char progressText[32];
        std::snprintf(progressText, sizeof(progressText), "%.1f%%", progress.progress * 100.0f);
        ImGui::ProgressBar(progress.progress, ImVec2(360.0f, 0.0f), progressText);
        if (!progress.visible) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (controller.ConsumeErrorPopupOpen()) {
        ImGui::OpenPopup(kErrorPopupTitle);
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2(540.0f, 0.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::BeginPopupModal(kErrorPopupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const std::string& title = controller.ErrorTitle();
        if (!title.empty()) {
            ImGui::TextUnformatted(title.c_str());
            ImGui::Separator();
        }
        const std::string& message = controller.ErrorMessage();
        if (message.empty()) {
            ImGui::TextWrapped("The selected structure file could not be imported.");
        } else {
            ImGui::TextWrapped("%s", message.c_str());
        }
        if (ImGui::Button("OK")) {
            controller.AcknowledgeError();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (controller.ConsumeWarningPopupOpen()) {
        ImGui::OpenPopup(controller.WarningTitle().empty() ? "Structure Import Notice" : controller.WarningTitle().c_str());
    }
    const char* warningPopupTitle = controller.WarningTitle().empty()
        ? "Structure Import Notice"
        : controller.WarningTitle().c_str();
    if (ImGui::BeginPopupModal(warningPopupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const std::string& message = controller.WarningMessage();
        if (!message.empty()) {
            ImGui::TextWrapped("%s", message.c_str());
        }
        if (ImGui::Button("OK")) {
            controller.AcknowledgeWarning();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

} // namespace features::file
