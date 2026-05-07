#include "periodic_table_ui.h"

#include "periodic_table_controller.h"

#include "core/ui/ui_color_utils.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

namespace features::build::periodic_table {
namespace {

constexpr float kButtonSize = 40.0f;
constexpr float kSpacing = 2.0f;
constexpr float kTotalWidth = (kButtonSize + kSpacing) * 18.0f + kSpacing;

std::string ToUpper(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return value;
}

} // namespace

PeriodicTableUI::PeriodicTableUI(PeriodicTableController& controller)
    : controller_(controller) {
}

void PeriodicTableUI::Render(bool* open) {
    if (open != nullptr && !(*open)) {
        return;
    }

    if (!ImGui::Begin("Periodic Table", open)) {
        ImGui::End();
        return;
    }

    RenderCategoryFilter();
    ImGui::InputText("Search Symbol", searchBuffer_, IM_ARRAYSIZE(searchBuffer_));

    const std::string selected = controller_.SelectedSymbol().empty() ? "None" : controller_.SelectedSymbol();
    ImGui::Text("Selected element: %s", selected.c_str());
    ImGui::Separator();

    RenderMainPeriodicTable();

    if (category_ == 0 || category_ == 9) {
        ImGui::Dummy(ImVec2(1.0f, 10.0f));
        RenderLanthanides();
    }

    if (category_ == 0 || category_ == 10) {
        ImGui::Dummy(ImVec2(1.0f, 10.0f));
        RenderActinides();
    }

    if (controller_.SelectedElementInfo() != nullptr) {
        RenderElementDetails();
    }

    ImGui::TextDisabled("Atom rendering is connected in Phase 3.4. This phase records SceneState updates.");
    ImGui::End();
}

void PeriodicTableUI::RenderCategoryFilter() {
    static const char* kCategoryNames[] = {
        "All Elements",
        "Non-metals",
        "Alkali Metals",
        "Alkaline Earth Metals",
        "Transition Metals",
        "Post-transition Metals",
        "Metalloid",
        "Halogens",
        "Noble Gases",
        "Lanthanide",
        "Actinide",
    };

    if (ImGui::BeginCombo("Classification", kCategoryNames[category_])) {
        for (int i = 0; i < 11; ++i) {
            if (ImGui::Selectable(kCategoryNames[i], category_ == i)) {
                category_ = i;
            }
        }
        ImGui::EndCombo();
    }
}

void PeriodicTableUI::RenderMainPeriodicTable() {
    core::data::ElementDatabase& db = core::data::ElementDatabase::getInstance();
    const auto allSymbols = db.getAllSymbols();

    const float availWidth = ImGui::GetContentRegionAvail().x;
    const float scale = (kTotalWidth > 0.0f) ? (availWidth / kTotalWidth) : 1.0f;
    const float buttonSize = std::max(14.0f, kButtonSize * scale);
    const float spacing = std::max(1.0f, kSpacing * scale);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

    for (int period = 1; period <= 7; ++period) {
        for (int group = 1; group <= 18; ++group) {
            const core::data::ElementInfo* element = nullptr;
            for (const std::string& symbol : allSymbols) {
                const core::data::PeriodicTablePosition pos = db.getElementPosition(symbol);
                if (pos.period == period && pos.group == group) {
                    element = db.getElementInfo(symbol);
                    break;
                }
            }

            if (group > 1) {
                ImGui::SameLine();
            }

            if (element != nullptr && ShouldShowElement(*element) && MatchesFilter(element->symbol)) {
                RenderElementButton(*element, buttonSize);
            } else {
                ImGui::Dummy(ImVec2(buttonSize, buttonSize));
            }
        }
    }

    ImGui::PopStyleVar();
}

void PeriodicTableUI::RenderLanthanides() {
    core::data::ElementDatabase& db = core::data::ElementDatabase::getInstance();
    const auto allSymbols = db.getAllSymbols();

    const float availWidth = ImGui::GetContentRegionAvail().x;
    const float scale = (kTotalWidth > 0.0f) ? (availWidth / kTotalWidth) : 1.0f;
    const float buttonSize = std::max(14.0f, kButtonSize * scale);
    const float spacing = std::max(1.0f, kSpacing * scale);

    ImGui::TextUnformatted("Lanthanides:");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

    for (int group = 3; group <= 17; ++group) {
        const core::data::ElementInfo* element = nullptr;
        for (const std::string& symbol : allSymbols) {
            const core::data::PeriodicTablePosition pos = db.getElementPosition(symbol);
            if (pos.period == 8 && pos.group == group) {
                element = db.getElementInfo(symbol);
                break;
            }
        }

        if (group > 3) {
            ImGui::SameLine();
        }

        if (element != nullptr && MatchesFilter(element->symbol)) {
            RenderElementButton(*element, buttonSize);
        } else {
            ImGui::Dummy(ImVec2(buttonSize, buttonSize));
        }
    }

    ImGui::PopStyleVar();
}

void PeriodicTableUI::RenderActinides() {
    core::data::ElementDatabase& db = core::data::ElementDatabase::getInstance();
    const auto allSymbols = db.getAllSymbols();

    const float availWidth = ImGui::GetContentRegionAvail().x;
    const float scale = (kTotalWidth > 0.0f) ? (availWidth / kTotalWidth) : 1.0f;
    const float buttonSize = std::max(14.0f, kButtonSize * scale);
    const float spacing = std::max(1.0f, kSpacing * scale);

    ImGui::TextUnformatted("Actinides:");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

    for (int group = 3; group <= 17; ++group) {
        const core::data::ElementInfo* element = nullptr;
        for (const std::string& symbol : allSymbols) {
            const core::data::PeriodicTablePosition pos = db.getElementPosition(symbol);
            if (pos.period == 9 && pos.group == group) {
                element = db.getElementInfo(symbol);
                break;
            }
        }

        if (group > 3) {
            ImGui::SameLine();
        }

        if (element != nullptr && MatchesFilter(element->symbol)) {
            RenderElementButton(*element, buttonSize);
        } else {
            ImGui::Dummy(ImVec2(buttonSize, buttonSize));
        }
    }

    ImGui::PopStyleVar();
}

void PeriodicTableUI::RenderElementButton(const core::data::ElementInfo& element, float buttonSize) {
    int pushedColors = 0;
    int pushedVars = 0;

    const ImVec4 baseColor = core::ui::ToImVec4(element.defaultColor);
    const ImVec4 hoveredColor = ImVec4(
        std::min(1.0f, baseColor.x * 1.2f),
        std::min(1.0f, baseColor.y * 1.2f),
        std::min(1.0f, baseColor.z * 1.2f),
        baseColor.w);
    const ImVec4 activeColor = ImVec4(baseColor.x * 0.8f, baseColor.y * 0.8f, baseColor.z * 0.8f, baseColor.w);
    const ImVec4 textColor = core::ui::GetContrastTextColor(baseColor);

    ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
    ++pushedColors;
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
    ++pushedColors;
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
    ++pushedColors;
    ImGui::PushStyleColor(ImGuiCol_Text, textColor);
    ++pushedColors;

    if (controller_.SelectedSymbol() == element.symbol) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ++pushedColors;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
        ++pushedVars;
    }

    char label[20];
    std::snprintf(label, sizeof(label), "%s\n%d", element.symbol.c_str(), element.atomicNumber);
    if (ImGui::Button(label, ImVec2(buttonSize, buttonSize))) {
        controller_.SetSelectedSymbol(element.symbol);
    }

    if (ImGui::IsItemHovered()) {
        RenderElementTooltip(element);
    }

    if (pushedColors > 0) {
        ImGui::PopStyleColor(pushedColors);
    }
    if (pushedVars > 0) {
        ImGui::PopStyleVar(pushedVars);
    }
}

void PeriodicTableUI::RenderElementTooltip(const core::data::ElementInfo& element) {
    ImGui::BeginTooltip();
    ImGui::Text("%s (%s)", element.name.c_str(), element.symbol.c_str());
    ImGui::Text("Atomic Number: %d", element.atomicNumber);
    ImGui::Text("Atomic Mass: %.4f", element.atomicMass);
    ImGui::Text("Covalent Radius: %.2f", element.covalentRadius);
    ImGui::Text("Group: %d, Period: %d", element.groupNumber, element.period);
    ImGui::Text("Classification: %s", element.classification.c_str());
    ImGui::EndTooltip();
}

void PeriodicTableUI::RenderElementDetails() {
    const core::data::ElementInfo* element = controller_.SelectedElementInfo();
    if (element == nullptr) {
        return;
    }

    ImGui::Separator();
    ImGui::Text("%s (%s)", element->name.c_str(), element->symbol.c_str());
    ImGui::Text("Atomic Mass: %.4f", element->atomicMass);
    ImGui::Text("Covalent Radius: %.2f", element->covalentRadius);
    ImGui::Text("Classification: %s", element->classification.c_str());
    ImGui::Text("Structure ID: %d", controller_.ActiveStructureId());
    ImGui::Text("Atom Count: %d", static_cast<int>(controller_.ActiveStructureAtomCount()));

    const bool hasUnitCell = controller_.HasActiveUnitCell();
    if (!hasUnitCell) {
        ImGui::BeginDisabled();
    }

    ImGui::Checkbox("Use Fractional Coordinates", &useFractionalCoords_);

    if (!hasUnitCell) {
        ImGui::EndDisabled();
        useFractionalCoords_ = false;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Fractional coordinates require a unit cell.");
        }
    }

    if (useFractionalCoords_ && hasUnitCell) {
        ImGui::DragFloat3("Fractional (a,b,c)", fractionalPosition_.data(), 0.01f, 0.0f, 1.0f);
        if (ImGui::Button("Add atom")) {
            controller_.AddAtomAtFractional(element->symbol, fractionalPosition_);
        }
    } else {
        ImGui::DragFloat3("Position (X,Y,Z)", cartesianPosition_.data(), 0.1f);
        if (ImGui::Button("Add atom")) {
            controller_.AddAtomAt(element->symbol, cartesianPosition_);
        }
    }
}

bool PeriodicTableUI::ShouldShowElement(const core::data::ElementInfo& element) const {
    if (category_ == 0) {
        return true;
    }

    const char* targetClass = nullptr;
    switch (category_) {
    case 1:
        targetClass = "Non-metals";
        break;
    case 2:
        targetClass = "Alkali Metals";
        break;
    case 3:
        targetClass = "Alkaline Earth Metals";
        break;
    case 4:
        targetClass = "Transition Metals";
        break;
    case 5:
        targetClass = "Post-transition Metals";
        break;
    case 6:
        targetClass = "Metalloid";
        break;
    case 7:
        targetClass = "Halogens";
        break;
    case 8:
        targetClass = "Noble Gases";
        break;
    case 9:
        targetClass = "Lanthanide";
        break;
    case 10:
        targetClass = "Actinide";
        break;
    default:
        return true;
    }

    return element.classification == targetClass;
}

bool PeriodicTableUI::MatchesFilter(const std::string& symbol) const {
    if (searchBuffer_[0] == '\0') {
        return true;
    }

    const std::string query = ToUpper(std::string(searchBuffer_));
    const std::string upperSymbol = ToUpper(symbol);
    return upperSymbol.find(query) != std::string::npos;
}

} // namespace features::build::periodic_table
