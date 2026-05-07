#include "crystal_system.h"

namespace features::build::bravais {

CrystalSystem CrystalSystemMapper::GetCrystalSystem(BravaisLatticeType type) {
    switch (type) {
    case BravaisLatticeType::SIMPLE_CUBIC:
    case BravaisLatticeType::BODY_CENTERED_CUBIC:
    case BravaisLatticeType::FACE_CENTERED_CUBIC:
        return CrystalSystem::CUBIC;

    case BravaisLatticeType::SIMPLE_TETRAGONAL:
    case BravaisLatticeType::BODY_CENTERED_TETRAGONAL:
        return CrystalSystem::TETRAGONAL;

    case BravaisLatticeType::SIMPLE_ORTHORHOMBIC:
    case BravaisLatticeType::BODY_CENTERED_ORTHORHOMBIC:
    case BravaisLatticeType::FACE_CENTERED_ORTHORHOMBIC:
    case BravaisLatticeType::BASE_CENTERED_ORTHORHOMBIC:
        return CrystalSystem::ORTHORHOMBIC;

    case BravaisLatticeType::SIMPLE_MONOCLINIC:
    case BravaisLatticeType::BASE_CENTERED_MONOCLINIC:
        return CrystalSystem::MONOCLINIC;

    case BravaisLatticeType::TRICLINIC:
        return CrystalSystem::TRICLINIC;

    case BravaisLatticeType::RHOMBOHEDRAL:
        return CrystalSystem::RHOMBOHEDRAL;

    case BravaisLatticeType::HEXAGONAL:
        return CrystalSystem::HEXAGONAL;

    default:
        return CrystalSystem::TRICLINIC;
    }
}

std::vector<BravaisLatticeType> CrystalSystemMapper::GetLatticeTypes(CrystalSystem system) {
    switch (system) {
    case CrystalSystem::CUBIC:
        return {
            BravaisLatticeType::SIMPLE_CUBIC,
            BravaisLatticeType::BODY_CENTERED_CUBIC,
            BravaisLatticeType::FACE_CENTERED_CUBIC,
        };
    case CrystalSystem::TETRAGONAL:
        return {
            BravaisLatticeType::SIMPLE_TETRAGONAL,
            BravaisLatticeType::BODY_CENTERED_TETRAGONAL,
        };
    case CrystalSystem::ORTHORHOMBIC:
        return {
            BravaisLatticeType::SIMPLE_ORTHORHOMBIC,
            BravaisLatticeType::BODY_CENTERED_ORTHORHOMBIC,
            BravaisLatticeType::FACE_CENTERED_ORTHORHOMBIC,
            BravaisLatticeType::BASE_CENTERED_ORTHORHOMBIC,
        };
    case CrystalSystem::MONOCLINIC:
        return {
            BravaisLatticeType::SIMPLE_MONOCLINIC,
            BravaisLatticeType::BASE_CENTERED_MONOCLINIC,
        };
    case CrystalSystem::TRICLINIC:
        return {BravaisLatticeType::TRICLINIC};
    case CrystalSystem::RHOMBOHEDRAL:
        return {BravaisLatticeType::RHOMBOHEDRAL};
    case CrystalSystem::HEXAGONAL:
        return {BravaisLatticeType::HEXAGONAL};
    default:
        return {};
    }
}

const char* CrystalSystemMapper::GetCrystalSystemName(CrystalSystem system) {
    switch (system) {
    case CrystalSystem::CUBIC:
        return "Cubic";
    case CrystalSystem::TETRAGONAL:
        return "Tetragonal";
    case CrystalSystem::ORTHORHOMBIC:
        return "Orthorhombic";
    case CrystalSystem::MONOCLINIC:
        return "Monoclinic";
    case CrystalSystem::TRICLINIC:
        return "Triclinic";
    case CrystalSystem::RHOMBOHEDRAL:
        return "Rhombohedral";
    case CrystalSystem::HEXAGONAL:
        return "Hexagonal";
    default:
        return "Unknown";
    }
}

const char* CrystalSystemMapper::GetSymmetryDescription(CrystalSystem system) {
    switch (system) {
    case CrystalSystem::CUBIC:
        return "Three equal axes at right angles.";
    case CrystalSystem::TETRAGONAL:
        return "a = b, c is unique, all angles are right.";
    case CrystalSystem::ORTHORHOMBIC:
        return "Three different axes, all angles are right.";
    case CrystalSystem::MONOCLINIC:
        return "One non-right angle (beta) with three different axes.";
    case CrystalSystem::TRICLINIC:
        return "All axes and all angles can be different.";
    case CrystalSystem::RHOMBOHEDRAL:
        return "Equal axes and equal non-right angles.";
    case CrystalSystem::HEXAGONAL:
        return "a = b, gamma is 120 deg, alpha = beta = 90 deg.";
    default:
        return "";
    }
}

} // namespace features::build::bravais
