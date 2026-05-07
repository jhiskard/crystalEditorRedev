#pragma once

#include "crystal_structure.h"
#include "crystal_system.h"

#include "core/scene/scene_state.h"

namespace features::build::bravais {

class BravaisController {
public:
    explicit BravaisController(core::scene::SceneState& scene);

    void Apply(BravaisLatticeType type, const BravaisParameters& params, bool preserveExistingAtoms);

    BravaisLatticeType CurrentType() const { return currentType_; }
    const BravaisParameters& CurrentParams() const { return params_; }
    CrystalSystem CurrentSystem() const;

private:
    int32_t EnsureActiveStructure();

    core::scene::SceneState& scene_;
    BravaisLatticeType currentType_ = BravaisLatticeType::SIMPLE_CUBIC;
    BravaisParameters params_;
    bool preserveExistingAtoms_ = true;
};

} // namespace features::build::bravais
