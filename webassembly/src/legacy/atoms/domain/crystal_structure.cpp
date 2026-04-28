// webassembly/src/atoms/domain/crystal_structure.cpp
#include "crystal_structure.h"
#include "../../config/log_config.h"
#include <cmath>
#include <cstring>

namespace atoms {
namespace domain {

// ============================================================================
// 격자 벡터 생성
// ============================================================================

void CrystalStructureGenerator::generateLatticeVectors(
    BravaisLatticeType type,
    const BravaisParameters& params,
    float cellMatrix[3][3]
) {
    SPDLOG_DEBUG("Generating PRIMITIVE lattice vectors for type={}, a={:.3f}, b={:.3f}, c={:.3f}, "
                 "alpha={:.1f}, beta={:.1f}, gamma={:.1f}",
                 static_cast<int>(type), params.a, params.b, params.c,
                 params.alpha, params.beta, params.gamma);
    
    // 행렬 초기화
    std::memset(cellMatrix, 0, sizeof(float) * 9);
    
    // 각도를 라디안으로 변환
    float alpha_rad = degToRad(params.alpha);
    float beta_rad = degToRad(params.beta);
    float gamma_rad = degToRad(params.gamma);
    
    float cos_alpha = std::cos(alpha_rad);
    float cos_beta = std::cos(beta_rad);
    float cos_gamma = std::cos(gamma_rad);
    float sin_alpha = std::sin(alpha_rad);
    float sin_gamma = std::sin(gamma_rad);
    
    // 🔧 변수를 switch 밖으로 이동하여 초기화 에러 방지
    float term = 1.0f - cos_alpha*cos_alpha - cos_beta*cos_beta - cos_gamma*cos_gamma
                      + 2.0f*cos_alpha*cos_beta*cos_gamma;
    
    // Primitive cell로 변환
    // 🔧 행 벡터 형식: cellMatrix[row][col] = row번째 벡터의 col번째 성분
    switch (type) {
        case BravaisLatticeType::SIMPLE_CUBIC:
        case BravaisLatticeType::SIMPLE_TETRAGONAL:
        case BravaisLatticeType::SIMPLE_ORTHORHOMBIC:
        case BravaisLatticeType::SIMPLE_MONOCLINIC:
        case BravaisLatticeType::TRICLINIC:
        case BravaisLatticeType::HEXAGONAL:
        case BravaisLatticeType::RHOMBOHEDRAL:
            // Simple 격자는 이미 Primitive
            // v1 = (a, 0, 0)
            cellMatrix[0][0] = params.a;
            cellMatrix[0][1] = 0.0f;
            cellMatrix[0][2] = 0.0f;
            
            // v2 = (b*cos(gamma), b*sin(gamma), 0)
            cellMatrix[1][0] = params.b * cos_gamma;
            cellMatrix[1][1] = params.b * sin_gamma;
            cellMatrix[1][2] = 0.0f;
            
            // v3 = (c*cos(beta), c*(cos(alpha)-cos(beta)*cos(gamma))/sin(gamma), c*sqrt(...))
            cellMatrix[2][0] = params.c * cos_beta;
            cellMatrix[2][1] = params.c * (cos_alpha - cos_beta * cos_gamma) / sin_gamma;
            cellMatrix[2][2] = params.c * std::sqrt(std::max(0.0f, term)) / sin_gamma;
            break;
            
        case BravaisLatticeType::BODY_CENTERED_CUBIC:
            // BCC primitive: v1=a/2*(-1,1,1), v2=a/2*(1,-1,1), v3=a/2*(1,1,-1)
            cellMatrix[0][0] = -params.a / 2.0f;
            cellMatrix[0][1] = params.a / 2.0f;
            cellMatrix[0][2] = params.a / 2.0f;
            
            cellMatrix[1][0] = params.a / 2.0f;
            cellMatrix[1][1] = -params.a / 2.0f;
            cellMatrix[1][2] = params.a / 2.0f;
            
            cellMatrix[2][0] = params.a / 2.0f;
            cellMatrix[2][1] = params.a / 2.0f;
            cellMatrix[2][2] = -params.a / 2.0f;
            break;
            
        case BravaisLatticeType::FACE_CENTERED_CUBIC:
            // FCC primitive: v1=a/2*(0,1,1), v2=a/2*(1,0,1), v3=a/2*(1,1,0)
            cellMatrix[0][0] = 0.0f;
            cellMatrix[0][1] = params.a / 2.0f;
            cellMatrix[0][2] = params.a / 2.0f;
            
            cellMatrix[1][0] = params.a / 2.0f;
            cellMatrix[1][1] = 0.0f;
            cellMatrix[1][2] = params.a / 2.0f;
            
            cellMatrix[2][0] = params.a / 2.0f;
            cellMatrix[2][1] = params.a / 2.0f;
            cellMatrix[2][2] = 0.0f;
            break;
            
        case BravaisLatticeType::BODY_CENTERED_TETRAGONAL:
            // BCT primitive (ASE): 0.5 * [[-a, a, c], [a, -a, c], [a, a, -c]]
            cellMatrix[0][0] = -params.a / 2.0f;
            cellMatrix[0][1] = params.a / 2.0f;
            cellMatrix[0][2] = params.c / 2.0f;
            
            cellMatrix[1][0] = params.a / 2.0f;
            cellMatrix[1][1] = -params.a / 2.0f;
            cellMatrix[1][2] = params.c / 2.0f;
            
            cellMatrix[2][0] = params.a / 2.0f;
            cellMatrix[2][1] = params.a / 2.0f;
            cellMatrix[2][2] = -params.c / 2.0f;
            break;
            
        case BravaisLatticeType::BODY_CENTERED_ORTHORHOMBIC:
            // BCO primitive: v1=1/2*(-a,b,c), v2=1/2*(a,-b,c), v3=1/2*(a,b,-c)
            cellMatrix[0][0] = -params.a / 2.0f;
            cellMatrix[0][1] = params.b / 2.0f;
            cellMatrix[0][2] = params.c / 2.0f;
            
            cellMatrix[1][0] = params.a / 2.0f;
            cellMatrix[1][1] = -params.b / 2.0f;
            cellMatrix[1][2] = params.c / 2.0f;
            
            cellMatrix[2][0] = params.a / 2.0f;
            cellMatrix[2][1] = params.b / 2.0f;
            cellMatrix[2][2] = -params.c / 2.0f;
            break;
            
        case BravaisLatticeType::FACE_CENTERED_ORTHORHOMBIC:
            // FCO primitive: v1=1/2*(0,b,c), v2=1/2*(a,0,c), v3=1/2*(a,b,0)
            cellMatrix[0][0] = 0.0f;
            cellMatrix[0][1] = params.b / 2.0f;
            cellMatrix[0][2] = params.c / 2.0f;
            
            cellMatrix[1][0] = params.a / 2.0f;
            cellMatrix[1][1] = 0.0f;
            cellMatrix[1][2] = params.c / 2.0f;
            
            cellMatrix[2][0] = params.a / 2.0f;
            cellMatrix[2][1] = params.b / 2.0f;
            cellMatrix[2][2] = 0.0f;
            break;
            
        case BravaisLatticeType::BASE_CENTERED_ORTHORHOMBIC:
            // Base-centered (C-centered) primitive: v1=1/2*(a,-b,0), v2=1/2*(a,b,0), v3=(0,0,c)
            cellMatrix[0][0] = params.a / 2.0f;
            cellMatrix[0][1] = -params.b / 2.0f;
            cellMatrix[0][2] = 0.0f;
            
            cellMatrix[1][0] = params.a / 2.0f;
            cellMatrix[1][1] = params.b / 2.0f;
            cellMatrix[1][2] = 0.0f;
            
            cellMatrix[2][0] = 0.0f;
            cellMatrix[2][1] = 0.0f;
            cellMatrix[2][2] = params.c;
            break;
            
        case BravaisLatticeType::BASE_CENTERED_MONOCLINIC:
            // Base-centered monoclinic primitive (ASE MCLC)
            // v1 = (a/2,  b/2, 0)
            // v2 = (-a/2, b/2, 0)
            // v3 = (0, c*cos(alpha), c*sin(alpha))
            cellMatrix[0][0] = params.a / 2.0f;
            cellMatrix[0][1] = params.b / 2.0f;
            cellMatrix[0][2] = 0.0f;
            
            cellMatrix[1][0] = -params.a / 2.0f;
            cellMatrix[1][1] = params.b / 2.0f;
            cellMatrix[1][2] = 0.0f;
            
            cellMatrix[2][0] = 0.0f;
            cellMatrix[2][1] = params.c * cos_alpha;
            cellMatrix[2][2] = params.c * sin_alpha;
            break;
    }
    
    SPDLOG_DEBUG("Generated PRIMITIVE lattice vectors (row vector format):");
    SPDLOG_DEBUG("  v1 = [{:.3f}, {:.3f}, {:.3f}]", 
                 cellMatrix[0][0], cellMatrix[0][1], cellMatrix[0][2]);
    SPDLOG_DEBUG("  v2 = [{:.3f}, {:.3f}, {:.3f}]", 
                 cellMatrix[1][0], cellMatrix[1][1], cellMatrix[1][2]);
    SPDLOG_DEBUG("  v3 = [{:.3f}, {:.3f}, {:.3f}]", 
                 cellMatrix[2][0], cellMatrix[2][1], cellMatrix[2][2]);
}

// ============================================================================
// 원자 위치 생성
// ============================================================================

std::vector<std::array<float, 3>> CrystalStructureGenerator::generateAtomPositions(
    BravaisLatticeType type
) {
    std::vector<std::array<float, 3>> positions;
    
    // Primitive cell은 항상 원점에 원자 1개만 가짐
    // (Centered 격자의 추가 원자들은 primitive 변환으로 제거됨)
    positions.push_back({0.0f, 0.0f, 0.0f});
    
    SPDLOG_DEBUG("Generated {} atom position(s) for PRIMITIVE lattice type {}",
                 positions.size(), static_cast<int>(type));
    
    return positions;
}

// ============================================================================
// 격자 정보 반환
// ============================================================================

const char* CrystalStructureGenerator::getLatticeName(BravaisLatticeType type) {
    static const char* names[] = {
        "Simple Cubic (SC)",
        "Body-Centered Cubic (BCC)",
        "Face-Centered Cubic (FCC)",
        "Simple Tetragonal (ST)",
        "Body-Centered Tetragonal (BCT)",
        "Simple Orthorhombic (SO)",
        "Body-Centered Orthorhombic (BCO)",
        "Face-Centered Orthorhombic (FCO)",
        "Base-Centered Orthorhombic (BCO/C)",
        "Simple Monoclinic (SM)",
        "Base-Centered Monoclinic (BCM)",
        "Triclinic",
        "Rhombohedral",
        "Hexagonal"
    };
    
    int index = static_cast<int>(type);
    if (index >= 0 && index < 14) {
        return names[index];
    }
    return "Unknown";
}

const char* CrystalStructureGenerator::getCrystalSystemName(BravaisLatticeType type) {
    switch (type) {
        case BravaisLatticeType::SIMPLE_CUBIC:
        case BravaisLatticeType::BODY_CENTERED_CUBIC:
        case BravaisLatticeType::FACE_CENTERED_CUBIC:
            return "Cubic";
            
        case BravaisLatticeType::SIMPLE_TETRAGONAL:
        case BravaisLatticeType::BODY_CENTERED_TETRAGONAL:
            return "Tetragonal";
            
        case BravaisLatticeType::SIMPLE_ORTHORHOMBIC:
        case BravaisLatticeType::BODY_CENTERED_ORTHORHOMBIC:
        case BravaisLatticeType::FACE_CENTERED_ORTHORHOMBIC:
        case BravaisLatticeType::BASE_CENTERED_ORTHORHOMBIC:
            return "Orthorhombic";
            
        case BravaisLatticeType::SIMPLE_MONOCLINIC:
        case BravaisLatticeType::BASE_CENTERED_MONOCLINIC:
            return "Monoclinic";
            
        case BravaisLatticeType::TRICLINIC:
            return "Triclinic";
            
        case BravaisLatticeType::RHOMBOHEDRAL:
            return "Rhombohedral";
            
        case BravaisLatticeType::HEXAGONAL:
            return "Hexagonal";
            
        default:
            return "Unknown";
    }
}

const char* CrystalStructureGenerator::getLatticeConstraints(BravaisLatticeType type) {
    static const char* constraints[] = {
        "a = b = c, all angles = 90°",           // Simple Cubic
        "a = b = c, all angles = 90°",           // BCC
        "a = b = c, all angles = 90°",           // FCC
        "a = b ≠ c, all angles = 90°",           // Simple Tetragonal
        "a = b ≠ c, all angles = 90°",           // BCT
        "a ≠ b ≠ c, all angles = 90°",           // Simple Orthorhombic
        "a ≠ b ≠ c, all angles = 90°",           // BCO
        "a ≠ b ≠ c, all angles = 90°",           // FCO
        "a ≠ b ≠ c, all angles = 90°",           // Base-Centered Orthorhombic
        "a ≠ b ≠ c, α = γ = 90°, β ≠ 90°",       // Simple Monoclinic
        "a ≠ b ≠ c, α = γ = 90°, β ≠ 90°",       // BCM
        "a ≠ b ≠ c, α ≠ β ≠ γ ≠ 90°",            // Triclinic
        "a = b = c, α = β = γ ≠ 90°",            // Rhombohedral
        "a = b ≠ c, α = β = 90°, γ = 120°"       // Hexagonal
    };
    
    int index = static_cast<int>(type);
    if (index >= 0 && index < 14) {
        return constraints[index];
    }
    return "";
}

const char* CrystalStructureGenerator::getLatticeDescription(BravaisLatticeType type) {
    switch (type) {
        case BravaisLatticeType::SIMPLE_CUBIC:
            return "Simple cubic has identical lattice parameters (a = b = c) and all angles "
                   "are 90°. The primitive cell contains one atom at the corner. "
                   "Examples: Po (polonium).";
            
        case BravaisLatticeType::BODY_CENTERED_CUBIC:
            return "Body-centered cubic has identical lattice parameters with atoms at each "
                   "corner and one at the center of the cube. Examples: Fe, Cr, W, Mo, V.";
            
        case BravaisLatticeType::FACE_CENTERED_CUBIC:
            return "Face-centered cubic has atoms at each corner and at the center of each "
                   "face. Examples: Al, Cu, Au, Ag, Ni, Pb.";
            
        case BravaisLatticeType::SIMPLE_TETRAGONAL:
            return "Simple tetragonal has two equal lattice parameters (a = b) and one "
                   "different (c), with all angles at 90°. Examples: TiO2 (rutile), SnO2.";
            
        case BravaisLatticeType::BODY_CENTERED_TETRAGONAL:
            return "Body-centered tetragonal has the same lattice parameter constraints as "
                   "simple tetragonal, with an additional atom at the body center. "
                   "Examples: In, Sn (white tin).";
            
        case BravaisLatticeType::SIMPLE_ORTHORHOMBIC:
            return "Simple orthorhombic has three different lattice parameters (a ≠ b ≠ c) "
                   "with all angles at 90°. Examples: S (sulfur), KNO3.";
            
        case BravaisLatticeType::BODY_CENTERED_ORTHORHOMBIC:
            return "Body-centered orthorhombic has three different lattice parameters with "
                   "an additional atom at the body center. Examples: U, I2.";
            
        case BravaisLatticeType::FACE_CENTERED_ORTHORHOMBIC:
            return "Face-centered orthorhombic has three different lattice parameters with "
                   "additional atoms at face centers. Examples: CaCO3 (aragonite).";
            
        case BravaisLatticeType::BASE_CENTERED_ORTHORHOMBIC:
            return "Base-centered orthorhombic has three different lattice parameters with "
                   "an additional atom at the base center. Examples: CaSO4·2H2O (gypsum).";
            
        case BravaisLatticeType::SIMPLE_MONOCLINIC:
            return "Simple monoclinic has three different lattice parameters with two angles "
                   "at 90° and one (β) not equal to 90°. Examples: monosodium glutamate.";
            
        case BravaisLatticeType::BASE_CENTERED_MONOCLINIC:
            return "Base-centered monoclinic has the same angle constraints as simple "
                   "monoclinic with an additional atom at the base center. Examples: CaSO4.";
            
        case BravaisLatticeType::TRICLINIC:
            return "Triclinic is the most general lattice type with all three lattice "
                   "parameters and angles being different. Examples: CuSO4·5H2O, K2Cr2O7.";
            
        case BravaisLatticeType::RHOMBOHEDRAL:
            return "Rhombohedral (trigonal) has all lattice parameters equal (a = b = c) "
                   "and all angles equal but not 90°. Examples: Bi, As, calcite.";
            
        case BravaisLatticeType::HEXAGONAL:
            return "Hexagonal has two equal lattice parameters (a = b) and one different (c), "
                   "with angles α = β = 90° and γ = 120°. Examples: Mg, Zn, graphite, ice.";
            
        default:
            return "Select a Bravais lattice to see its description.";
    }
}

} // namespace domain
} // namespace atoms
