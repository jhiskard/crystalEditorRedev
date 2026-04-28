// webassembly/src/atoms/domain/crystal_structure.h
#pragma once

#include <array>
#include <vector>
#include <string>

namespace atoms {
namespace domain {

/**
 * @brief Bravais 격자 유형 열거형 (14가지)
 */
enum class BravaisLatticeType {
    SIMPLE_CUBIC = 0,              // 단순 입방 (SC)
    BODY_CENTERED_CUBIC = 1,       // 체심 입방 (BCC)
    FACE_CENTERED_CUBIC = 2,       // 면심 입방 (FCC)
    SIMPLE_TETRAGONAL = 3,         // 단순 정방 (ST)
    BODY_CENTERED_TETRAGONAL = 4,  // 체심 정방 (BCT)
    SIMPLE_ORTHORHOMBIC = 5,       // 단순 직방 (SO)
    BODY_CENTERED_ORTHORHOMBIC = 6,// 체심 직방 (BCO)
    FACE_CENTERED_ORTHORHOMBIC = 7,// 면심 직방 (FCO)
    BASE_CENTERED_ORTHORHOMBIC = 8,// 저심 직방 (BCO/C)
    SIMPLE_MONOCLINIC = 9,         // 단순 단사 (SM)
    BASE_CENTERED_MONOCLINIC = 10, // 저심 단사 (BCM)
    TRICLINIC = 11,                // 삼사정계
    RHOMBOHEDRAL = 12,             // 삼방정계 (능면체)
    HEXAGONAL = 13                 // 육방정계
};

/**
 * @brief Bravais 격자 파라미터 구조체
 */
struct BravaisParameters {
    float a = 1.0f;      // 격자 상수 a (Angstrom)
    float b = 1.0f;      // 격자 상수 b (Angstrom)
    float c = 1.0f;      // 격자 상수 c (Angstrom)
    float alpha = 90.0f; // 각도 α (degree)
    float beta = 90.0f;  // 각도 β (degree)
    float gamma = 90.0f; // 각도 γ (degree)
};

/**
 * @brief 결정 구조 생성기
 * 
 * Bravais 격자 유형과 파라미터로부터 단위 격자 벡터와 
 * 기본 원자 위치를 생성하는 순수 함수 집합
 */
class CrystalStructureGenerator {
public:
    /**
     * @brief Bravais 격자 유형과 파라미터로부터 단위 격자 벡터 생성
     * 
     * 격자 파라미터 (a, b, c, α, β, γ)를 3x3 격자 벡터 행렬로 변환
     * 
     * @param type Bravais 격자 유형
     * @param params 격자 파라미터 (길이와 각도)
     * @param cellMatrix 출력: 3x3 격자 벡터 행렬 (열 벡터 형식)
     * 
     * @note 격자 벡터는 다음과 같이 배열됨:
     *       cellMatrix[i][j] = i번째 성분의 j번째 벡터
     *       즉, 열 벡터 형식: [v1 v2 v3]
     */
    static void generateLatticeVectors(
        BravaisLatticeType type,
        const BravaisParameters& params,
        float cellMatrix[3][3]
    );
    
    /**
     * @brief Bravais 격자 유형에 따른 기본 원자 위치 생성
     * 
     * 각 격자 유형의 특성에 맞는 원자 위치를 분수 좌표로 반환
     * - Simple: (0,0,0) 하나
     * - Body-centered: (0,0,0), (0.5,0.5,0.5) 두 개
     * - Face-centered: (0,0,0), (0.5,0.5,0), (0.5,0,0.5), (0,0.5,0.5) 네 개
     * - Base-centered: (0,0,0), (0.5,0.5,0) 두 개
     * 
     * @param type Bravais 격자 유형
     * @return 원자 위치 벡터 (분수 좌표, 0~1 범위)
     */
    static std::vector<std::array<float, 3>> generateAtomPositions(
        BravaisLatticeType type
    );
    
    /**
     * @brief 격자 유형에 대한 상세 설명 반환
     * 
     * @param type Bravais 격자 유형
     * @return 격자 구조에 대한 설명 문자열
     */
    static const char* getLatticeDescription(BravaisLatticeType type);
    
    /**
     * @brief 격자 유형의 이름 반환
     * 
     * @param type Bravais 격자 유형
     * @return 격자 이름 (예: "Simple Cubic (SC)")
     */
    static const char* getLatticeName(BravaisLatticeType type);
    
    /**
     * @brief 격자 유형이 속한 결정계 이름 반환
     * 
     * @param type Bravais 격자 유형
     * @return 결정계 이름 (예: "Cubic", "Tetragonal")
     */
    static const char* getCrystalSystemName(BravaisLatticeType type);
    
    /**
     * @brief 격자 유형의 제약 조건 문자열 반환
     * 
     * @param type Bravais 격자 유형
     * @return 제약 조건 문자열 (예: "a = b = c, all angles = 90°")
     */
    static const char* getLatticeConstraints(BravaisLatticeType type);

private:
    // 각도를 라디안으로 변환하는 헬퍼 함수
    static constexpr float degToRad(float degrees) {
        return degrees * 3.14159265358979323846f / 180.0f;
    }
};

} // namespace domain
} // namespace atoms