#pragma once

#include "crystal_structure.h"

#include <array>

#include <imgui.h>

namespace features::build::bravais {

class BravaisController;

class BravaisLatticeUI {
public:
    explicit BravaisLatticeUI(BravaisController& controller);

    void Render(bool* open);

private:
    void InitializeDefaultParameters();
    void RenderCategoryFilter();
    void RenderLatticeTable();
    void RenderParameterInputs(int latticeIndex);
    void RenderLatticeDescription() const;
    void OnLatticeSelected(int latticeIndex);
    bool IsLatticeVisible(int latticeIndex) const;
    const char* GetLatticeName(int latticeIndex) const;

    BravaisController& controller_;

    int selectedLatticeType_ = -1;
    std::array<BravaisParameters, 14> latticeParams_{};

    bool showCubic_ = true;
    bool showTetragonal_ = false;
    bool showOrthorhombic_ = false;
    bool showMonoclinic_ = false;
    bool showTriclinic_ = false;
    bool showRhombohedral_ = false;
    bool showHexagonal_ = false;

    bool preserveExistingAtoms_ = true;
};

} // namespace features::build::bravais
