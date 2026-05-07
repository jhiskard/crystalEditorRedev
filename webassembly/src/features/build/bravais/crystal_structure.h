#pragma once

#include <array>
#include <string>
#include <vector>

namespace features::build::bravais {

enum class BravaisLatticeType {
    SIMPLE_CUBIC = 0,
    BODY_CENTERED_CUBIC = 1,
    FACE_CENTERED_CUBIC = 2,
    SIMPLE_TETRAGONAL = 3,
    BODY_CENTERED_TETRAGONAL = 4,
    SIMPLE_ORTHORHOMBIC = 5,
    BODY_CENTERED_ORTHORHOMBIC = 6,
    FACE_CENTERED_ORTHORHOMBIC = 7,
    BASE_CENTERED_ORTHORHOMBIC = 8,
    SIMPLE_MONOCLINIC = 9,
    BASE_CENTERED_MONOCLINIC = 10,
    TRICLINIC = 11,
    RHOMBOHEDRAL = 12,
    HEXAGONAL = 13,
};

struct BravaisParameters {
    float a = 1.0f;
    float b = 1.0f;
    float c = 1.0f;
    float alpha = 90.0f;
    float beta = 90.0f;
    float gamma = 90.0f;
};

class CrystalStructureGenerator {
public:
    static void GenerateLatticeVectors(
        BravaisLatticeType type,
        const BravaisParameters& params,
        float cellMatrix[3][3]);

    static std::array<std::array<float, 3>, 3> ComputeLatticeMatrix(
        BravaisLatticeType type,
        const BravaisParameters& params);

    static std::vector<std::array<float, 3>> GenerateAtomPositions(BravaisLatticeType type);

    static const char* GetLatticeDescription(BravaisLatticeType type);
    static const char* GetLatticeName(BravaisLatticeType type);
    static const char* GetCrystalSystemName(BravaisLatticeType type);
    static const char* GetLatticeConstraints(BravaisLatticeType type);

private:
    static constexpr float DegToRad(float degrees) {
        return degrees * 3.14159265358979323846f / 180.0f;
    }
};

} // namespace features::build::bravais
