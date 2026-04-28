#pragma once
#include <string>
#include <unordered_map>
#include <vector>
// #include <imgui.h>
#include "color.h"

namespace atoms {
namespace domain {

// ============================================================================
// 원소 분류 열거형 (추가)
// ============================================================================

/**
 * @brief 원소 분류 카테고리
 * 
 * 주기율표 필터링을 위한 원소 분류
 */
enum class ElementClassification {
    ALL_ELEMENTS = 0,
    NON_METALS,
    ALKALI_METALS,
    ALKALINE_EARTH_METALS,
    TRANSITION_METALS,
    POST_TRANSITION_METALS,
    METALLOID,
    HALOGENS,
    NOBLE_GASES,
    LANTHANIDE,
    ACTINIDE
};

/**
 * @brief 원소 분류명 반환
 * @param classification 분류 열거형
 * @return 분류명 문자열
 */
const char* getClassificationName(ElementClassification classification);

// ============================================================================
// 주기율표 위치 정보 (추가)
// ============================================================================

/**
 * @brief 주기율표 위치 정보
 */
struct PeriodicTablePosition {
    int period;      // 주기 (1-7, 란타넘족=8, 악티늄족=9)
    int group;       // 족 (1-18)
    
    PeriodicTablePosition() : period(0), group(0) {}
    PeriodicTablePosition(int p, int g) : period(p), group(g) {}
};

// ============================================================================
// 원소 정보 구조체 (확장)
// ============================================================================

/**
 * @brief 화학 원소 정보 구조체
 * 
 * 기존 atoms_template.cpp의 분산된 원소 데이터를 통합 관리
 */
struct ElementInfo {
    std::string symbol;           // 원소 기호 (예: "H", "C", "O")
    std::string name;             // 원소 이름
    float atomicRadius;           // 원자 반지름 (Å)
    float covalentRadius;         // 공유결합 반지름 (Å)
    // ImVec4 defaultColor;          // Jmol 기본 색상
    Color4f defaultColor;
    int atomicNumber;             // 원자 번호 (Z)
    float atomicMass;             // 원자량 (amu)
    std::string group;            // 원소족 정보 (예: "1", "18")
    int period;                   // 주기 (1-7, 란타넘족=8, 악티늄족=9)
    int groupNumber;              // ⭐ 추가: 족 번호 (1-18)
    std::string classification;   // ⭐ 추가: 원소 분류 ("Non-metals", "Alkali Metals" 등)
    
    /**
     * @brief ElementInfo 생성자 (확장)
     */
    ElementInfo(const std::string& sym, const std::string& nm, 
        float aRadius, float cRadius, 
        // const ImVec4& color,
        const Color4f& color, 
        int atomicNum, float mass, 
        const std::string& grp, int per,
        int groupNum, const std::string& classif);
    
    /**
     * @brief 기본 생성자
     */
    ElementInfo() = default;
};

// ============================================================================
// ElementDatabase 클래스 (확장)
// ============================================================================

/**
 * @brief 원소 데이터베이스 클래스 (싱글턴)
 * 
 * 기존 atoms_template.cpp의 다음 데이터들을 통합:
 * - chemical_symbols 배열
 * - atomic_names 배열  
 * - atomic_masses 배열
 * - covalent_radii 배열
 * - jmol_colors 배열
 * - findElementColor 함수 로직
 */
class ElementDatabase {
public:
    /**
     * @brief 싱글턴 인스턴스 접근
     */
    static ElementDatabase& getInstance();
    
    // ========================================================================
    // 원소 정보 조회
    // ========================================================================
    
    /**
     * @brief 원소 정보 조회
     * @param symbol 원소 기호 (예: "H", "C")
     * @return 원소 정보 포인터 (없으면 nullptr)
     */
    const ElementInfo* getElementInfo(const std::string& symbol) const;
    
    /**
     * @brief 원소 존재 여부 확인
     */
    bool hasElement(const std::string& symbol) const;
    
    // ========================================================================
    // 기본값 제공 (기존 atoms_template.cpp 함수 대체)
    // ========================================================================
    
    /**
     * @brief 원소의 기본 반지름 조회 (기존 getAtomRadius 대체)
     * @param symbol 원소 기호
     * @return 공유결합 반지름 (기본값: 1.0f)
     */
    float getDefaultRadius(const std::string& symbol) const;
    
    /**
     * @brief 원소의 기본 색상 조회 (기존 findElementColor 대체)
     * @param symbol 원소 기호
     * @return Jmol 표준 색상 (기본값: 회색)
     */
    // ImVec4 getDefaultColor(const std::string& symbol) const;
    Color4f getDefaultColor(const std::string& symbol) const;

    /**
     * @brief 원소 이름 조회
     */
    std::string getElementName(const std::string& symbol) const;
    
    /**
     * @brief 원자량 조회
     */
    float getAtomicMass(const std::string& symbol) const;
    
    /**
     * @brief 원자 번호 조회
     */
    int getAtomicNumber(const std::string& symbol) const;
    
    // ========================================================================
    // ⭐ 주기율표 렌더링을 위한 새 메서드들 (추가)
    // ========================================================================
    
    /**
     * @brief 주기율표 위치 조회
     * @param symbol 원소 기호
     * @return 주기율표 위치 정보 (period, group)
     */
    PeriodicTablePosition getElementPosition(const std::string& symbol) const;
    
    /**
     * @brief 원소 분류 조회
     * @param symbol 원소 기호
     * @return 원소 분류 문자열 (예: "Non-metals", "Alkali Metals")
     */
    std::string getElementClassification(const std::string& symbol) const;
    
    /**
     * @brief 분류별 원소 목록 조회
     * @param classification 원소 분류 문자열
     * @return 해당 분류에 속하는 원소 기호 목록
     */
    std::vector<std::string> getElementsByClassification(const std::string& classification) const;
    
    /**
     * @brief 원자번호로 분류 조회 (기존 element_classifications 배열 대체)
     * @param Z 원자 번호
     * @return 원소 분류 문자열
     */
    std::string getClassificationByAtomicNumber(int Z) const;
    
    /**
     * @brief 족 번호 조회
     * @param symbol 원소 기호
     * @return 족 번호 (1-18)
     */
    int getGroupNumber(const std::string& symbol) const;
    
    /**
     * @brief 주기 번호 조회
     * @param symbol 원소 기호
     * @return 주기 번호 (1-7, 란타넘족=8, 악티늄족=9)
     */
    int getPeriodNumber(const std::string& symbol) const;
    
    // ========================================================================
    // 전체 원소 목록
    // ========================================================================
    
    /**
     * @brief 지원하는 모든 원소 기호 목록
     */
    std::vector<std::string> getAllSymbols() const;
    
    /**
     * @brief 등록된 원소 개수
     */
    size_t getElementCount() const;
    
    /**
     * @brief 주기별 원소 목록
     */
    std::vector<std::string> getElementsByPeriod(int period) const;
    
    /**
     * @brief 족별 원소 목록  
     */
    std::vector<std::string> getElementsByGroup(const std::string& group) const;
    
private:
    ElementDatabase();
    ~ElementDatabase() = default;
    
    // 복사 생성자/할당 연산자 삭제 (싱글턴)
    ElementDatabase(const ElementDatabase&) = delete;
    ElementDatabase& operator=(const ElementDatabase&) = delete;
    
    /**
     * @brief 원소 데이터베이스 초기화
     * 
     * 기존 atoms_template.cpp의 정적 배열들을 로드:
     * - chemical_symbols, atomic_names, atomic_masses
     * - covalent_radii, jmol_colors
     */
    void initializeDatabase();
    
    /**
     * @brief 특정 원소 추가 (내부 사용)
     */
    void addElement(const std::string& symbol, const std::string& name, 
        float atomicRadius, float covalentRadius, 
        // const ImVec4& color, 
        const Color4f& color, 
        int atomicNumber, float mass,
        const std::string& group, int period,
        int groupNumber, const std::string& classification);
    
    // ========================================================================
    // 내부 데이터
    // ========================================================================
    std::unordered_map<std::string, ElementInfo> m_elements;
    bool m_initialized = false;
};

} // namespace domain
} // namespace atoms