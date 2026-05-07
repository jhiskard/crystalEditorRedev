#pragma once

#include "crystal_structure.h"

#include <vector>

namespace features::build::bravais {

enum class CrystalSystem {
    CUBIC = 0,
    TETRAGONAL = 1,
    ORTHORHOMBIC = 2,
    MONOCLINIC = 3,
    TRICLINIC = 4,
    RHOMBOHEDRAL = 5,
    HEXAGONAL = 6,
};

class CrystalSystemMapper {
public:
    static CrystalSystem GetCrystalSystem(BravaisLatticeType type);
    static std::vector<BravaisLatticeType> GetLatticeTypes(CrystalSystem system);
    static const char* GetCrystalSystemName(CrystalSystem system);
    static const char* GetSymmetryDescription(CrystalSystem system);
};

} // namespace features::build::bravais
