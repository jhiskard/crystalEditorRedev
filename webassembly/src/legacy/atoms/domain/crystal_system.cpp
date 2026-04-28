// webassembly/src/atoms/domain/crystal_system.cpp
#include "crystal_system.h"

namespace atoms {
namespace domain {

// ============================================================================
// 결정계 매핑
// ============================================================================

CrystalSystem CrystalSystemMapper::getCrystalSystem(BravaisLatticeType type) {
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
            return CrystalSystem::TRICLINIC; // 기본값
    }
}

std::vector<BravaisLatticeType> CrystalSystemMapper::getLatticeTypes(CrystalSystem system) {
    std::vector<BravaisLatticeType> types;
    
    switch (system) {
        case CrystalSystem::CUBIC:
            types.push_back(BravaisLatticeType::SIMPLE_CUBIC);
            types.push_back(BravaisLatticeType::BODY_CENTERED_CUBIC);
            types.push_back(BravaisLatticeType::FACE_CENTERED_CUBIC);
            break;
            
        case CrystalSystem::TETRAGONAL:
            types.push_back(BravaisLatticeType::SIMPLE_TETRAGONAL);
            types.push_back(BravaisLatticeType::BODY_CENTERED_TETRAGONAL);
            break;
            
        case CrystalSystem::ORTHORHOMBIC:
            types.push_back(BravaisLatticeType::SIMPLE_ORTHORHOMBIC);
            types.push_back(BravaisLatticeType::BODY_CENTERED_ORTHORHOMBIC);
            types.push_back(BravaisLatticeType::FACE_CENTERED_ORTHORHOMBIC);
            types.push_back(BravaisLatticeType::BASE_CENTERED_ORTHORHOMBIC);
            break;
            
        case CrystalSystem::MONOCLINIC:
            types.push_back(BravaisLatticeType::SIMPLE_MONOCLINIC);
            types.push_back(BravaisLatticeType::BASE_CENTERED_MONOCLINIC);
            break;
            
        case CrystalSystem::TRICLINIC:
            types.push_back(BravaisLatticeType::TRICLINIC);
            break;
            
        case CrystalSystem::RHOMBOHEDRAL:
            types.push_back(BravaisLatticeType::RHOMBOHEDRAL);
            break;
            
        case CrystalSystem::HEXAGONAL:
            types.push_back(BravaisLatticeType::HEXAGONAL);
            break;
    }
    
    return types;
}

const char* CrystalSystemMapper::getCrystalSystemName(CrystalSystem system) {
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

const char* CrystalSystemMapper::getSymmetryDescription(CrystalSystem system) {
    switch (system) {
        case CrystalSystem::CUBIC:
            return "Highest symmetry: Three equal axes at 90° angles. "
                   "Point group symmetry: m3̄m (Oh).";
                   
        case CrystalSystem::TETRAGONAL:
            return "One unique axis (c) perpendicular to two equal axes (a = b). "
                   "Point group symmetry: 4/mmm (D4h).";
                   
        case CrystalSystem::ORTHORHOMBIC:
            return "Three mutually perpendicular axes of different lengths. "
                   "Point group symmetry: mmm (D2h).";
                   
        case CrystalSystem::MONOCLINIC:
            return "One unique axis (b) at an angle β to the other two axes. "
                   "Point group symmetry: 2/m (C2h).";
                   
        case CrystalSystem::TRICLINIC:
            return "Lowest symmetry: All axes and angles are different. "
                   "Point group symmetry: 1̄ (Ci).";
                   
        case CrystalSystem::RHOMBOHEDRAL:
            return "Three equal axes at equal angles (not 90°). "
                   "Point group symmetry: 3̄m (D3d).";
                   
        case CrystalSystem::HEXAGONAL:
            return "One unique axis (c) perpendicular to two equal axes at 120°. "
                   "Point group symmetry: 6/mmm (D6h).";
                   
        default:
            return "";
    }
}

} // namespace domain
} // namespace atoms