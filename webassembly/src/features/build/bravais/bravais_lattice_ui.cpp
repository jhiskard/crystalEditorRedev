#include "bravais_lattice_ui.h"

#include "bravais_controller.h"
#include "crystal_system.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <string>

namespace features::build::bravais {

BravaisLatticeUI::BravaisLatticeUI(BravaisController& controller)
    : controller_(controller) {
    InitializeDefaultParameters();
}

void BravaisLatticeUI::InitializeDefaultParameters() {
    for (int i = 0; i < 14; ++i) {
        BravaisParameters& params = latticeParams_[i];
        params = BravaisParameters{};

        const BravaisLatticeType type = static_cast<BravaisLatticeType>(i);
        switch (type) {
        case BravaisLatticeType::SIMPLE_CUBIC:
        case BravaisLatticeType::BODY_CENTERED_CUBIC:
        case BravaisLatticeType::FACE_CENTERED_CUBIC:
            params.a = 1.0f;
            params.b = 1.0f;
            params.c = 1.0f;
            params.alpha = 90.0f;
            params.beta = 90.0f;
            params.gamma = 90.0f;
            break;

        case BravaisLatticeType::SIMPLE_TETRAGONAL:
        case BravaisLatticeType::BODY_CENTERED_TETRAGONAL:
            params.a = 1.0f;
            params.b = 1.0f;
            params.c = 1.5f;
            params.alpha = 90.0f;
            params.beta = 90.0f;
            params.gamma = 90.0f;
            break;

        case BravaisLatticeType::SIMPLE_ORTHORHOMBIC:
        case BravaisLatticeType::BODY_CENTERED_ORTHORHOMBIC:
        case BravaisLatticeType::FACE_CENTERED_ORTHORHOMBIC:
        case BravaisLatticeType::BASE_CENTERED_ORTHORHOMBIC:
            params.a = 1.0f;
            params.b = 1.2f;
            params.c = 1.5f;
            params.alpha = 90.0f;
            params.beta = 90.0f;
            params.gamma = 90.0f;
            break;

        case BravaisLatticeType::SIMPLE_MONOCLINIC:
        case BravaisLatticeType::BASE_CENTERED_MONOCLINIC:
            params.a = 1.0f;
            params.b = 1.2f;
            params.c = 1.5f;
            params.alpha = 90.0f;
            params.beta = 100.0f;
            params.gamma = 90.0f;
            break;

        case BravaisLatticeType::TRICLINIC:
            params.a = 1.0f;
            params.b = 1.2f;
            params.c = 1.5f;
            params.alpha = 85.0f;
            params.beta = 95.0f;
            params.gamma = 100.0f;
            break;

        case BravaisLatticeType::RHOMBOHEDRAL:
            params.a = 1.0f;
            params.b = 1.0f;
            params.c = 1.0f;
            params.alpha = 80.0f;
            params.beta = 80.0f;
            params.gamma = 80.0f;
            break;

        case BravaisLatticeType::HEXAGONAL:
            params.a = 1.0f;
            params.b = 1.0f;
            params.c = 1.6f;
            params.alpha = 90.0f;
            params.beta = 90.0f;
            params.gamma = 120.0f;
            break;
        }
    }
}

void BravaisLatticeUI::Render(bool* open) {
    if (open != nullptr && !(*open)) {
        return;
    }

    if (!ImGui::Begin("Bravais Lattice Templates", open)) {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Bravais Lattice Templates");
    RenderCategoryFilter();

    ImGui::Separator();
    RenderLatticeTable();

    ImGui::Separator();
    ImGui::Checkbox("Preserve Existing Atoms", &preserveExistingAtoms_);

    const bool canApply = (selectedLatticeType_ >= 0 && selectedLatticeType_ < 14);
    if (!canApply) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Apply")) {
        const BravaisLatticeType type = static_cast<BravaisLatticeType>(selectedLatticeType_);
        controller_.Apply(type, latticeParams_[selectedLatticeType_], preserveExistingAtoms_);
    }
    if (!canApply) {
        ImGui::EndDisabled();
    }

    ImGui::TextDisabled("Phase 3.3 updates SceneState cell/atoms; visual atom renderer is connected in Phase 3.4.");

    if (selectedLatticeType_ >= 0) {
        ImGui::Separator();
        RenderLatticeDescription();
    }

    ImGui::End();
}

void BravaisLatticeUI::RenderCategoryFilter() {
    ImGui::TextUnformatted("Filter by Crystal System:");

    struct FilterItem {
        const char* label;
        bool* value;
    };

    const std::array<FilterItem, 7> filters = {{
        {"Cubic", &showCubic_},
        {"Tetragonal", &showTetragonal_},
        {"Orthorhombic", &showOrthorhombic_},
        {"Monoclinic", &showMonoclinic_},
        {"Triclinic", &showTriclinic_},
        {"Rhombohedral", &showRhombohedral_},
        {"Hexagonal", &showHexagonal_},
    }};

    const ImGuiStyle& style = ImGui::GetStyle();
    const float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

    for (size_t i = 0; i < filters.size(); ++i) {
        ImGui::Checkbox(filters[i].label, filters[i].value);

        if (i + 1 >= filters.size()) {
            continue;
        }

        const float nextWidth = ImGui::GetFrameHeight() +
                                style.ItemInnerSpacing.x +
                                ImGui::CalcTextSize(filters[i + 1].label).x +
                                style.FramePadding.x * 2.0f;
        const float nextX2 = ImGui::GetItemRectMax().x + style.ItemSpacing.x + nextWidth;
        if (nextX2 < windowVisibleX2) {
            ImGui::SameLine();
        }
    }
}

void BravaisLatticeUI::RenderLatticeTable() {
    const float availWidth = ImGui::GetContentRegionAvail().x;
    const float buttonWidth = std::max(220.0f, availWidth * 0.30f);

    if (ImGui::BeginTable("BravaisLatticeTable", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Lattice", ImGuiTableColumnFlags_WidthFixed, buttonWidth);
        ImGui::TableSetupColumn("Parameters", ImGuiTableColumnFlags_WidthStretch);

        for (int i = 0; i < 14; ++i) {
            if (!IsLatticeVisible(i)) {
                continue;
            }

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            if (ImGui::Button(GetLatticeName(i), ImVec2(-FLT_MIN, 0.0f))) {
                OnLatticeSelected(i);
            }

            ImGui::TableNextColumn();
            RenderParameterInputs(i);
        }

        ImGui::EndTable();
    }
}

void BravaisLatticeUI::RenderParameterInputs(int latticeIndex) {
    BravaisParameters& params = latticeParams_[latticeIndex];
    const BravaisLatticeType type = static_cast<BravaisLatticeType>(latticeIndex);

    auto inputFloat = [&](const char* label, float* value, const char* format = "%.3f") {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(95.0f);
        return ImGui::InputFloat((std::string("##") + label + std::to_string(latticeIndex)).c_str(), value, 0.0f, 0.0f, format);
    };

    switch (type) {
    case BravaisLatticeType::SIMPLE_CUBIC:
    case BravaisLatticeType::BODY_CENTERED_CUBIC:
    case BravaisLatticeType::FACE_CENTERED_CUBIC:
        if (inputFloat("a:", &params.a)) {
            params.b = params.a;
            params.c = params.a;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(a = b = c, all angles = 90 deg)");
        break;

    case BravaisLatticeType::SIMPLE_TETRAGONAL:
    case BravaisLatticeType::BODY_CENTERED_TETRAGONAL:
        if (inputFloat("a:", &params.a)) {
            params.b = params.a;
        }
        ImGui::SameLine();
        inputFloat("c:", &params.c);
        ImGui::SameLine();
        ImGui::TextDisabled("(a = b != c)");
        break;

    case BravaisLatticeType::SIMPLE_ORTHORHOMBIC:
    case BravaisLatticeType::BODY_CENTERED_ORTHORHOMBIC:
    case BravaisLatticeType::FACE_CENTERED_ORTHORHOMBIC:
    case BravaisLatticeType::BASE_CENTERED_ORTHORHOMBIC:
        inputFloat("a:", &params.a);
        ImGui::SameLine();
        inputFloat("b:", &params.b);
        ImGui::SameLine();
        inputFloat("c:", &params.c);
        ImGui::SameLine();
        ImGui::TextDisabled("(all angles = 90 deg)");
        break;

    case BravaisLatticeType::SIMPLE_MONOCLINIC:
    case BravaisLatticeType::BASE_CENTERED_MONOCLINIC:
        inputFloat("a:", &params.a);
        ImGui::SameLine();
        inputFloat("b:", &params.b);
        ImGui::SameLine();
        inputFloat("c:", &params.c);
        ImGui::SameLine();
        inputFloat("beta:", &params.beta, "%.1f");
        ImGui::SameLine();
        ImGui::TextDisabled("(alpha = gamma = 90 deg)");
        break;

    case BravaisLatticeType::TRICLINIC:
        inputFloat("a:", &params.a);
        ImGui::SameLine();
        inputFloat("b:", &params.b);
        ImGui::SameLine();
        inputFloat("c:", &params.c);
        ImGui::NewLine();
        inputFloat("alpha:", &params.alpha, "%.1f");
        ImGui::SameLine();
        inputFloat("beta:", &params.beta, "%.1f");
        ImGui::SameLine();
        inputFloat("gamma:", &params.gamma, "%.1f");
        break;

    case BravaisLatticeType::RHOMBOHEDRAL:
        if (inputFloat("a:", &params.a)) {
            params.b = params.a;
            params.c = params.a;
        }
        ImGui::SameLine();
        if (inputFloat("alpha:", &params.alpha, "%.1f")) {
            params.beta = params.alpha;
            params.gamma = params.alpha;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(a = b = c, alpha = beta = gamma)");
        break;

    case BravaisLatticeType::HEXAGONAL:
        if (inputFloat("a:", &params.a)) {
            params.b = params.a;
            params.alpha = 90.0f;
            params.beta = 90.0f;
            params.gamma = 120.0f;
        }
        ImGui::SameLine();
        inputFloat("c:", &params.c);
        ImGui::SameLine();
        ImGui::TextDisabled("(a = b, gamma = 120 deg)");
        break;
    }
}

void BravaisLatticeUI::RenderLatticeDescription() const {
    const BravaisLatticeType type = static_cast<BravaisLatticeType>(selectedLatticeType_);
    ImGui::Text("Selected: %s", CrystalStructureGenerator::GetLatticeName(type));
    if (ImGui::TreeNode("Lattice Description")) {
        ImGui::TextWrapped("%s", CrystalStructureGenerator::GetLatticeDescription(type));
        ImGui::TreePop();
    }
}

void BravaisLatticeUI::OnLatticeSelected(int latticeIndex) {
    selectedLatticeType_ = latticeIndex;
}

bool BravaisLatticeUI::IsLatticeVisible(int latticeIndex) const {
    const BravaisLatticeType type = static_cast<BravaisLatticeType>(latticeIndex);
    const CrystalSystem system = CrystalSystemMapper::GetCrystalSystem(type);

    switch (system) {
    case CrystalSystem::CUBIC:
        return showCubic_;
    case CrystalSystem::TETRAGONAL:
        return showTetragonal_;
    case CrystalSystem::ORTHORHOMBIC:
        return showOrthorhombic_;
    case CrystalSystem::MONOCLINIC:
        return showMonoclinic_;
    case CrystalSystem::TRICLINIC:
        return showTriclinic_;
    case CrystalSystem::RHOMBOHEDRAL:
        return showRhombohedral_;
    case CrystalSystem::HEXAGONAL:
        return showHexagonal_;
    default:
        return false;
    }
}

const char* BravaisLatticeUI::GetLatticeName(int latticeIndex) const {
    const BravaisLatticeType type = static_cast<BravaisLatticeType>(latticeIndex);
    return CrystalStructureGenerator::GetLatticeName(type);
}

} // namespace features::build::bravais
