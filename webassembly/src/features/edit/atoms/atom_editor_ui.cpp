#include "atom_editor_ui.h"

#include "atoms_controller.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <string>

namespace features::edit::atoms {
namespace {

std::string ToUpper(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return value;
}

bool IsBoundaryAtom(const core::scene::AtomRecord& atom) {
    return atom.group == "Boundary";
}

const char* TypeLabel(const core::scene::AtomRecord& atom) {
    return IsBoundaryAtom(atom) ? "SURR" : "ORIG";
}

float CoordValue(const core::scene::AtomRecord& atom, int axis, bool fractional) {
    return fractional ? atom.fractional[axis] : atom.cartesian[axis];
}

} // namespace

AtomEditorUI::AtomEditorUI(AtomsController& controller)
    : controller_(controller) {
}

void AtomEditorUI::Render(bool* open) {
    if (open != nullptr && !(*open)) {
        return;
    }

    if (!ImGui::Begin("Created Atoms", open)) {
        ImGui::End();
        return;
    }

    RenderToolbar();
    RenderSelectionTools();
    RenderTable();
    RenderTransformTools();
    RenderRadiusTools();

    ImGui::End();
}

void AtomEditorUI::RenderToolbar() {
    const size_t atomCount = controller_.ActiveAtomCount();
    ImGui::Text("Total atoms: %d", static_cast<int>(atomCount));

    if (ImGui::Checkbox("Fractional coordinates", &useFractionalCoords_)) {
        // State only.
    }
    ImGui::SameLine();

    boundaryAtomsEnabled_ = controller_.BoundaryAtomsEnabled();
    bool boundaryToggle = boundaryAtomsEnabled_;
    if (ImGui::Checkbox("Boundary atoms", &boundaryToggle)) {
        controller_.SetBoundaryAtomsEnabled(boundaryToggle);
        boundaryAtomsEnabled_ = boundaryToggle;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Edit mode##atoms", &editMode_);

    ImGui::Separator();
}

void AtomEditorUI::RenderSelectionTools() {
    if (ImGui::Button("Select all")) {
        controller_.SelectAll();
    }
    ImGui::SameLine();
    if (ImGui::Button("Select none")) {
        controller_.SelectNone();
    }
    ImGui::SameLine();
    if (ImGui::Button("Invert selection")) {
        controller_.InvertSelection();
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete selected")) {
        controller_.RemoveSelectedAtoms();
    }
}

void AtomEditorUI::RenderTransformTools() {
    ImGui::Separator();
    if (useFractionalCoords_) {
        ImGui::InputFloat3("Delta (a,b,c)", translateDelta_.data(), "%.4f");
    } else {
        ImGui::InputFloat3("Delta (X,Y,Z)", translateDelta_.data(), "%.4f");
    }
    if (ImGui::Button("Apply translation")) {
        controller_.TranslateSelected(translateDelta_, useFractionalCoords_);
    }
}

void AtomEditorUI::RenderRadiusTools() {
    ImGui::Separator();
    ImGui::InputFloat("Selected radius", &selectedRadius_, 0.01f, 0.1f, "%.3f");
    if (ImGui::Button("Apply radius")) {
        controller_.SetRadiusSelected(selectedRadius_);
    }
    ImGui::Separator();
}

void AtomEditorUI::RenderTable() {
    RefreshRows();

    ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit |
                                 ImGuiTableFlags_Borders |
                                 ImGuiTableFlags_RowBg |
                                 ImGuiTableFlags_Reorderable |
                                 ImGuiTableFlags_Sortable;
    if (rows_.size() > 20) {
        tableFlags |= ImGuiTableFlags_ScrollY;
    }

    const int columnCount = editMode_ ? 9 : 8;
    if (!ImGui::BeginTable(
            "CreatedAtomsTable",
            columnCount,
            tableFlags,
            ImVec2(0.0f, rows_.size() > 20 ? 560.0f : 0.0f))) {
        return;
    }

    ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 52.0f, 0);
    ImGui::TableSetupColumn("Selected", ImGuiTableColumnFlags_WidthFixed, 84.0f, 1);
    if (editMode_) {
        ImGui::TableSetupColumn("Edit", ImGuiTableColumnFlags_WidthFixed, 64.0f, 2);
    }
    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 64.0f, 3);
    ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_WidthFixed, 88.0f, 4);
    ImGui::TableSetupColumn(useFractionalCoords_ ? "a" : "X", ImGuiTableColumnFlags_WidthStretch, 0.0f, 5);
    ImGui::TableSetupColumn(useFractionalCoords_ ? "b" : "Y", ImGuiTableColumnFlags_WidthStretch, 0.0f, 6);
    ImGui::TableSetupColumn(useFractionalCoords_ ? "c" : "Z", ImGuiTableColumnFlags_WidthStretch, 0.0f, 7);
    ImGui::TableSetupColumn("Radius", ImGuiTableColumnFlags_WidthFixed, 90.0f, 8);
    ImGui::TableHeadersRow();

    sortSpecs_ = ImGui::TableGetSortSpecs();
    if (sortSpecs_ != nullptr && sortSpecs_->SpecsCount > 0 && sortSpecs_->SpecsDirty) {
        const ImGuiTableColumnSortSpecs& spec = sortSpecs_->Specs[0];
        sortColumn_ = static_cast<int>(spec.ColumnUserID);
        sortAscending_ = (spec.SortDirection == ImGuiSortDirection_Ascending);
        SortRows();
        sortSpecs_->SpecsDirty = false;
    }

    for (const RowView& row : rows_) {
        const core::scene::AtomRecord* atom = controller_.AtomAt(row.index);
        if (atom == nullptr) {
            continue;
        }

        const bool isBoundary = IsBoundaryAtom(*atom);
        int dataColumn = 2;

        ImGui::PushID(static_cast<int>(row.index));
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%d", static_cast<int>(row.index + 1));

        ImGui::TableSetColumnIndex(1);
        if (isBoundary) {
            ImGui::BeginDisabled();
            bool dummy = false;
            ImGui::Checkbox("##selected_disabled", &dummy);
            ImGui::EndDisabled();
        } else {
            bool selected = atom->selected;
            if (ImGui::Checkbox("##selected", &selected)) {
                controller_.SetSelectedByIndex(row.index, selected);
            }
        }

        if (editMode_) {
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(isBoundary ? "N/A" : "Edit");
            dataColumn = 3;
        }

        ImGui::TableSetColumnIndex(dataColumn + 0);
        ImGui::TextUnformatted(TypeLabel(*atom));

        ImGui::TableSetColumnIndex(dataColumn + 1);
        if (editMode_ && !isBoundary) {
            char symbolBuffer[16];
            std::snprintf(symbolBuffer, sizeof(symbolBuffer), "%s", atom->symbol.c_str());
            if (ImGui::InputText("##symbol", symbolBuffer, sizeof(symbolBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                controller_.SetSymbolByIndex(row.index, std::string(symbolBuffer));
            }
        } else {
            ImGui::TextUnformatted(atom->symbol.c_str());
        }

        std::array<float, 3> coord = {
            CoordValue(*atom, 0, useFractionalCoords_),
            CoordValue(*atom, 1, useFractionalCoords_),
            CoordValue(*atom, 2, useFractionalCoords_),
        };

        for (int axis = 0; axis < 3; ++axis) {
            ImGui::TableSetColumnIndex(dataColumn + 2 + axis);
            if (editMode_ && !isBoundary) {
                char fieldId[24];
                std::snprintf(fieldId, sizeof(fieldId), "##coord_%d", axis);
                float value = coord[axis];
                if (ImGui::InputFloat(fieldId, &value, 0.0f, 0.0f, "%.4f")) {
                    coord[axis] = value;
                    controller_.SetPositionByIndex(row.index, coord, useFractionalCoords_);
                }
            } else {
                ImGui::Text("%.4f", coord[axis]);
            }
        }

        ImGui::TableSetColumnIndex(dataColumn + 5);
        if (editMode_ && !isBoundary) {
            float radius = atom->radius;
            if (ImGui::InputFloat("##radius", &radius, 0.01f, 0.1f, "%.4f")) {
                controller_.SetRadiusByIndex(row.index, radius);
            }
        } else {
            ImGui::Text("%.4f", atom->radius);
        }

        ImGui::PopID();
    }

    ImGui::EndTable();
}

void AtomEditorUI::RefreshRows() {
    rows_.clear();
    const auto* atoms = controller_.ActiveAtoms();
    if (atoms == nullptr) {
        return;
    }

    rows_.reserve(atoms->size());
    for (size_t i = 0; i < atoms->size(); ++i) {
        rows_.push_back(RowView{i});
    }

    SortRows();
}

void AtomEditorUI::SortRows() {
    const auto* atoms = controller_.ActiveAtoms();
    if (atoms == nullptr || rows_.size() <= 1) {
        return;
    }

    std::sort(rows_.begin(), rows_.end(), [this, atoms](const RowView& lhs, const RowView& rhs) {
        const core::scene::AtomRecord& a = (*atoms)[lhs.index];
        const core::scene::AtomRecord& b = (*atoms)[rhs.index];

        auto compareNumber = [this](float left, float right) {
            if (left == right) {
                return false;
            }
            return sortAscending_ ? (left < right) : (left > right);
        };
        auto compareBool = [this](bool left, bool right) {
            if (left == right) {
                return false;
            }
            return sortAscending_ ? (!left && right) : (left && !right);
        };
        auto compareText = [this](const std::string& left, const std::string& right) {
            const std::string l = ToUpper(left);
            const std::string r = ToUpper(right);
            if (l == r) {
                return false;
            }
            return sortAscending_ ? (l < r) : (l > r);
        };

        switch (sortColumn_) {
        case 0:
            return sortAscending_ ? (lhs.index < rhs.index) : (lhs.index > rhs.index);
        case 1:
            return compareBool(a.selected, b.selected);
        case 2:
            return compareBool(!IsBoundaryAtom(a), !IsBoundaryAtom(b));
        case 3:
            return compareText(TypeLabel(a), TypeLabel(b));
        case 4:
            return compareText(a.symbol, b.symbol);
        case 5:
            return compareNumber(CoordValue(a, 0, useFractionalCoords_), CoordValue(b, 0, useFractionalCoords_));
        case 6:
            return compareNumber(CoordValue(a, 1, useFractionalCoords_), CoordValue(b, 1, useFractionalCoords_));
        case 7:
            return compareNumber(CoordValue(a, 2, useFractionalCoords_), CoordValue(b, 2, useFractionalCoords_));
        case 8:
            return compareNumber(a.radius, b.radius);
        default:
            return sortAscending_ ? (lhs.index < rhs.index) : (lhs.index > rhs.index);
        }
    });
}

} // namespace features::edit::atoms
