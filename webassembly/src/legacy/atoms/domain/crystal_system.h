// webassembly/src/atoms/domain/crystal_system.h
#pragma once

#include "crystal_structure.h"
#include <vector>

namespace atoms {
namespace domain {

/**
 * @brief 결정계 열거형 (7가지)
 * 
 * 14개의 Bravais 격자는 7개의 결정계로 분류됨
 */
enum class CrystalSystem {
    CUBIC,          // 입방정계 (3개 격자)
    TETRAGONAL,     // 정방정계 (2개 격자)
    ORTHORHOMBIC,   // 직방정계 (4개 격자)
    MONOCLINIC,     // 단사정계 (2개 격자)
    TRICLINIC,      // 삼사정계 (1개 격자)
    RHOMBOHEDRAL,   // 삼방정계 (1개 격자)
    HEXAGONAL       // 육방정계 (1개 격자)
};

/**
 * @brief 결정계 매핑 유틸리티
 * 
 * Bravais 격자 유형과 결정계 간의 매핑 관계를 관리
 */
class CrystalSystemMapper {
public:
    /**
     * @brief Bravais 격자 유형으로부터 결정계 결정
     * 
     * @param type Bravais 격자 유형
     * @return 해당하는 결정계
     */
    static CrystalSystem getCrystalSystem(BravaisLatticeType type);
    
    /**
     * @brief 특정 결정계에 속하는 모든 Bravais 격자 유형 반환
     * 
     * @param system 결정계
     * @return 해당 결정계에 속하는 Bravais 격자 유형 벡터
     * 
     * @example
     *   auto cubicLattices = CrystalSystemMapper::getLatticeTypes(CrystalSystem::CUBIC);
     *   // 반환: {SIMPLE_CUBIC, BODY_CENTERED_CUBIC, FACE_CENTERED_CUBIC}
     */
    static std::vector<BravaisLatticeType> getLatticeTypes(CrystalSystem system);
    
    /**
     * @brief 결정계 이름 반환
     * 
     * @param system 결정계
     * @return 결정계 이름 문자열
     */
    static const char* getCrystalSystemName(CrystalSystem system);
    
    /**
     * @brief 결정계의 대칭 특성 설명 반환
     * 
     * @param system 결정계
     * @return 대칭 특성 설명 문자열
     */
    static const char* getSymmetryDescription(CrystalSystem system);
};

} // namespace domain
} // namespace atoms