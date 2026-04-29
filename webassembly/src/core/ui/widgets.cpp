#include "widgets.h"

namespace core::ui {

bool Widgets::IconButton(const char* buttonId,
                         const char* label,
                         ImVec2 buttonSize) {
    ImGui::PushID(buttonId);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    const bool clicked = ImGui::Button(label, buttonSize);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopID();
    return clicked;
}

} // namespace core::ui
