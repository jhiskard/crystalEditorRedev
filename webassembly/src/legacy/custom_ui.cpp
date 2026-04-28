#include "custom_ui.h"


bool CustomUI::IconButton(const char* btnId, const char* label, ImVec2 btnSize) {
    ImGui::PushID(btnId);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));     // Set padding to 0
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));  // Set transparent button
    bool btnClicked = ImGui::Button(label, btnSize);
    ImGui::PopStyleColor();  // Reset transparent to default
    ImGui::PopStyleVar();    // Reset padding to default
    ImGui::PopID();

    return btnClicked;
}