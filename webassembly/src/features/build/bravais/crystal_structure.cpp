#include "crystal_structure.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace features::build::bravais {

void CrystalStructureGenerator::GenerateLatticeVectors(
    BravaisLatticeType type,
    const BravaisParameters& params,
    float cellMatrix[3][3]) {
    std::memset(cellMatrix, 0, sizeof(float) * 9);

    const float alphaRad = DegToRad(params.alpha);
    const float betaRad = DegToRad(params.beta);
    const float gammaRad = DegToRad(params.gamma);

    const float cosAlpha = std::cos(alphaRad);
    const float cosBeta = std::cos(betaRad);
    const float cosGamma = std::cos(gammaRad);
    const float sinAlpha = std::sin(alphaRad);
    const float sinGamma = std::sin(gammaRad);

    const float safeSinGamma = (std::fabs(sinGamma) < 1e-6f) ? 1e-6f : sinGamma;
    const float term = 1.0f - cosAlpha * cosAlpha - cosBeta * cosBeta - cosGamma * cosGamma +
                       2.0f * cosAlpha * cosBeta * cosGamma;

    switch (type) {
    case BravaisLatticeType::SIMPLE_CUBIC:
    case BravaisLatticeType::SIMPLE_TETRAGONAL:
    case BravaisLatticeType::SIMPLE_ORTHORHOMBIC:
    case BravaisLatticeType::SIMPLE_MONOCLINIC:
    case BravaisLatticeType::TRICLINIC:
    case BravaisLatticeType::HEXAGONAL:
    case BravaisLatticeType::RHOMBOHEDRAL:
        cellMatrix[0][0] = params.a;
        cellMatrix[0][1] = 0.0f;
        cellMatrix[0][2] = 0.0f;

        cellMatrix[1][0] = params.b * cosGamma;
        cellMatrix[1][1] = params.b * safeSinGamma;
        cellMatrix[1][2] = 0.0f;

        cellMatrix[2][0] = params.c * cosBeta;
        cellMatrix[2][1] = params.c * (cosAlpha - cosBeta * cosGamma) / safeSinGamma;
        cellMatrix[2][2] = params.c * std::sqrt(std::max(0.0f, term)) / safeSinGamma;
        break;

    case BravaisLatticeType::BODY_CENTERED_CUBIC:
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
        cellMatrix[0][0] = params.a / 2.0f;
        cellMatrix[0][1] = params.b / 2.0f;
        cellMatrix[0][2] = 0.0f;

        cellMatrix[1][0] = -params.a / 2.0f;
        cellMatrix[1][1] = params.b / 2.0f;
        cellMatrix[1][2] = 0.0f;

        cellMatrix[2][0] = 0.0f;
        cellMatrix[2][1] = params.c * cosAlpha;
        cellMatrix[2][2] = params.c * sinAlpha;
        break;
    }
}

std::array<std::array<float, 3>, 3> CrystalStructureGenerator::ComputeLatticeMatrix(
    BravaisLatticeType type,
    const BravaisParameters& params) {
    std::array<std::array<float, 3>, 3> matrix = {{
        {{0.0f, 0.0f, 0.0f}},
        {{0.0f, 0.0f, 0.0f}},
        {{0.0f, 0.0f, 0.0f}},
    }};
    float cellMatrix[3][3];
    GenerateLatticeVectors(type, params, cellMatrix);
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            matrix[row][col] = cellMatrix[row][col];
        }
    }
    return matrix;
}

std::vector<std::array<float, 3>> CrystalStructureGenerator::GenerateAtomPositions(BravaisLatticeType type) {
    switch (type) {
    case BravaisLatticeType::BODY_CENTERED_CUBIC:
    case BravaisLatticeType::BODY_CENTERED_TETRAGONAL:
    case BravaisLatticeType::BODY_CENTERED_ORTHORHOMBIC:
        return {{
            {0.0f, 0.0f, 0.0f},
            {0.5f, 0.5f, 0.5f},
        }};

    case BravaisLatticeType::FACE_CENTERED_CUBIC:
    case BravaisLatticeType::FACE_CENTERED_ORTHORHOMBIC:
        return {{
            {0.0f, 0.0f, 0.0f},
            {0.5f, 0.5f, 0.0f},
            {0.5f, 0.0f, 0.5f},
            {0.0f, 0.5f, 0.5f},
        }};

    case BravaisLatticeType::BASE_CENTERED_ORTHORHOMBIC:
    case BravaisLatticeType::BASE_CENTERED_MONOCLINIC:
        return {{
            {0.0f, 0.0f, 0.0f},
            {0.5f, 0.5f, 0.0f},
        }};

    default:
        return {{
            {0.0f, 0.0f, 0.0f},
        }};
    }
}

const char* CrystalStructureGenerator::GetLatticeName(BravaisLatticeType type) {
    static const char* kNames[] = {
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
        "Hexagonal",
    };

    const int index = static_cast<int>(type);
    if (index < 0 || index >= 14) {
        return "Unknown";
    }
    return kNames[index];
}

const char* CrystalStructureGenerator::GetCrystalSystemName(BravaisLatticeType type) {
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

const char* CrystalStructureGenerator::GetLatticeConstraints(BravaisLatticeType type) {
    static const char* kConstraints[] = {
        "a = b = c, all angles = 90 deg",
        "a = b = c, all angles = 90 deg",
        "a = b = c, all angles = 90 deg",
        "a = b != c, all angles = 90 deg",
        "a = b != c, all angles = 90 deg",
        "a != b != c, all angles = 90 deg",
        "a != b != c, all angles = 90 deg",
        "a != b != c, all angles = 90 deg",
        "a != b != c, all angles = 90 deg",
        "a != b != c, alpha = gamma = 90 deg, beta != 90 deg",
        "a != b != c, alpha = gamma = 90 deg, beta != 90 deg",
        "a != b != c, alpha != beta != gamma != 90 deg",
        "a = b = c, alpha = beta = gamma != 90 deg",
        "a = b != c, alpha = beta = 90 deg, gamma = 120 deg",
    };

    const int index = static_cast<int>(type);
    if (index < 0 || index >= 14) {
        return "";
    }
    return kConstraints[index];
}

const char* CrystalStructureGenerator::GetLatticeDescription(BravaisLatticeType type) {
    switch (type) {
    case BravaisLatticeType::SIMPLE_CUBIC:
        return "Simple cubic has a = b = c and all angles are 90 deg.";
    case BravaisLatticeType::BODY_CENTERED_CUBIC:
        return "Body-centered cubic has one additional atom at cell center.";
    case BravaisLatticeType::FACE_CENTERED_CUBIC:
        return "Face-centered cubic has atoms at the center of each face.";
    case BravaisLatticeType::SIMPLE_TETRAGONAL:
        return "Simple tetragonal has a = b and c different, with 90 deg angles.";
    case BravaisLatticeType::BODY_CENTERED_TETRAGONAL:
        return "Body-centered tetragonal adds one center atom to tetragonal.";
    case BravaisLatticeType::SIMPLE_ORTHORHOMBIC:
        return "Simple orthorhombic has three different axis lengths and right angles.";
    case BravaisLatticeType::BODY_CENTERED_ORTHORHOMBIC:
        return "Body-centered orthorhombic adds one center atom.";
    case BravaisLatticeType::FACE_CENTERED_ORTHORHOMBIC:
        return "Face-centered orthorhombic adds face-centered atoms.";
    case BravaisLatticeType::BASE_CENTERED_ORTHORHOMBIC:
        return "Base-centered orthorhombic has one additional base-centered atom.";
    case BravaisLatticeType::SIMPLE_MONOCLINIC:
        return "Simple monoclinic has one non-right angle (beta).";
    case BravaisLatticeType::BASE_CENTERED_MONOCLINIC:
        return "Base-centered monoclinic adds one base-centered atom.";
    case BravaisLatticeType::TRICLINIC:
        return "Triclinic is the most general lattice with all sides and angles different.";
    case BravaisLatticeType::RHOMBOHEDRAL:
        return "Rhombohedral has equal side lengths and equal non-right angles.";
    case BravaisLatticeType::HEXAGONAL:
        return "Hexagonal has a = b, alpha = beta = 90 deg, gamma = 120 deg.";
    default:
        return "Select a Bravais lattice to see its description.";
    }
}

} // namespace features::build::bravais
