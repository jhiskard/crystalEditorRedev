#pragma once
#include <string>
#include <unordered_map>
#include <vector>
// #include <imgui.h>
#include "color.h"

namespace core {
namespace data {

// ============================================================================
// ?먯냼 遺꾨쪟 ?닿굅??(異붽?)
// ============================================================================

/**
 * @brief ?먯냼 遺꾨쪟 移댄뀒怨좊━
 * 
 * 二쇨린?⑦몴 ?꾪꽣留곸쓣 ?꾪븳 ?먯냼 遺꾨쪟
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
 * @brief ?먯냼 遺꾨쪟紐?諛섑솚
 * @param classification 遺꾨쪟 ?닿굅??
 * @return 遺꾨쪟紐?臾몄옄??
 */
const char* getClassificationName(ElementClassification classification);

// ============================================================================
// 二쇨린?⑦몴 ?꾩튂 ?뺣낫 (異붽?)
// ============================================================================

/**
 * @brief 二쇨린?⑦몴 ?꾩튂 ?뺣낫
 */
struct PeriodicTablePosition {
    int period;      // 二쇨린 (1-7, ???섏”=8, ?낇떚?꾩”=9)
    int group;       // 議?(1-18)
    
    PeriodicTablePosition() : period(0), group(0) {}
    PeriodicTablePosition(int p, int g) : period(p), group(g) {}
};

// ============================================================================
// ?먯냼 ?뺣낫 援ъ“泥?(?뺤옣)
// ============================================================================

/**
 * @brief ?뷀븰 ?먯냼 ?뺣낫 援ъ“泥?
 * 
 * 湲곗〈 atoms_template.cpp??遺꾩궛???먯냼 ?곗씠?곕? ?듯빀 愿由?
 */
struct ElementInfo {
    std::string symbol;           // ?먯냼 湲고샇 (?? "H", "C", "O")
    std::string name;             // ?먯냼 ?대쫫
    float atomicRadius;           // ?먯옄 諛섏?由?(횇)
    float covalentRadius;         // 怨듭쑀寃고빀 諛섏?由?(횇)
    // ImVec4 defaultColor;          // Jmol 湲곕낯 ?됱긽
    Color4f defaultColor;
    int atomicNumber;             // ?먯옄 踰덊샇 (Z)
    float atomicMass;             // ?먯옄??(amu)
    std::string group;            // ?먯냼議??뺣낫 (?? "1", "18")
    int period;                   // 二쇨린 (1-7, ???섏”=8, ?낇떚?꾩”=9)
    int groupNumber;              // 狩?異붽?: 議?踰덊샇 (1-18)
    std::string classification;   // 狩?異붽?: ?먯냼 遺꾨쪟 ("Non-metals", "Alkali Metals" ??
    
    /**
     * @brief ElementInfo ?앹꽦??(?뺤옣)
     */
    ElementInfo(const std::string& sym, const std::string& nm, 
        float aRadius, float cRadius, 
        // const ImVec4& color,
        const Color4f& color, 
        int atomicNum, float mass, 
        const std::string& grp, int per,
        int groupNum, const std::string& classif);
    
    /**
     * @brief 湲곕낯 ?앹꽦??
     */
    ElementInfo() = default;
};

// ============================================================================
// ElementDatabase ?대옒??(?뺤옣)
// ============================================================================

/**
 * @brief ?먯냼 ?곗씠?곕쿋?댁뒪 ?대옒??(?깃???
 * 
 * 湲곗〈 atoms_template.cpp???ㅼ쓬 ?곗씠?곕뱾???듯빀:
 * - chemical_symbols 諛곗뿴
 * - atomic_names 諛곗뿴  
 * - atomic_masses 諛곗뿴
 * - covalent_radii 諛곗뿴
 * - jmol_colors 諛곗뿴
 * - findElementColor ?⑥닔 濡쒖쭅
 */
class ElementDatabase {
public:
    /**
     * @brief ?깃????몄뒪?댁뒪 ?묎렐
     */
    static ElementDatabase& getInstance();
    
    // ========================================================================
    // ?먯냼 ?뺣낫 議고쉶
    // ========================================================================
    
    /**
     * @brief ?먯냼 ?뺣낫 議고쉶
     * @param symbol ?먯냼 湲고샇 (?? "H", "C")
     * @return ?먯냼 ?뺣낫 ?ъ씤??(?놁쑝硫?nullptr)
     */
    const ElementInfo* getElementInfo(const std::string& symbol) const;
    
    /**
     * @brief ?먯냼 議댁옱 ?щ? ?뺤씤
     */
    bool hasElement(const std::string& symbol) const;
    
    // ========================================================================
    // 湲곕낯媛??쒓났 (湲곗〈 atoms_template.cpp ?⑥닔 ?泥?
    // ========================================================================
    
    /**
     * @brief ?먯냼??湲곕낯 諛섏?由?議고쉶 (湲곗〈 getAtomRadius ?泥?
     * @param symbol ?먯냼 湲고샇
     * @return 怨듭쑀寃고빀 諛섏?由?(湲곕낯媛? 1.0f)
     */
    float getDefaultRadius(const std::string& symbol) const;
    
    /**
     * @brief ?먯냼??湲곕낯 ?됱긽 議고쉶 (湲곗〈 findElementColor ?泥?
     * @param symbol ?먯냼 湲고샇
     * @return Jmol ?쒖? ?됱긽 (湲곕낯媛? ?뚯깋)
     */
    // ImVec4 getDefaultColor(const std::string& symbol) const;
    Color4f getDefaultColor(const std::string& symbol) const;

    /**
     * @brief ?먯냼 ?대쫫 議고쉶
     */
    std::string getElementName(const std::string& symbol) const;
    
    /**
     * @brief ?먯옄??議고쉶
     */
    float getAtomicMass(const std::string& symbol) const;
    
    /**
     * @brief ?먯옄 踰덊샇 議고쉶
     */
    int getAtomicNumber(const std::string& symbol) const;
    
    // ========================================================================
    // 狩?二쇨린?⑦몴 ?뚮뜑留곸쓣 ?꾪븳 ??硫붿꽌?쒕뱾 (異붽?)
    // ========================================================================
    
    /**
     * @brief 二쇨린?⑦몴 ?꾩튂 議고쉶
     * @param symbol ?먯냼 湲고샇
     * @return 二쇨린?⑦몴 ?꾩튂 ?뺣낫 (period, group)
     */
    PeriodicTablePosition getElementPosition(const std::string& symbol) const;
    
    /**
     * @brief ?먯냼 遺꾨쪟 議고쉶
     * @param symbol ?먯냼 湲고샇
     * @return ?먯냼 遺꾨쪟 臾몄옄??(?? "Non-metals", "Alkali Metals")
     */
    std::string getElementClassification(const std::string& symbol) const;
    
    /**
     * @brief 遺꾨쪟蹂??먯냼 紐⑸줉 議고쉶
     * @param classification ?먯냼 遺꾨쪟 臾몄옄??
     * @return ?대떦 遺꾨쪟???랁븯???먯냼 湲고샇 紐⑸줉
     */
    std::vector<std::string> getElementsByClassification(const std::string& classification) const;
    
    /**
     * @brief ?먯옄踰덊샇濡?遺꾨쪟 議고쉶 (湲곗〈 element_classifications 諛곗뿴 ?泥?
     * @param Z ?먯옄 踰덊샇
     * @return ?먯냼 遺꾨쪟 臾몄옄??
     */
    std::string getClassificationByAtomicNumber(int Z) const;
    
    /**
     * @brief 議?踰덊샇 議고쉶
     * @param symbol ?먯냼 湲고샇
     * @return 議?踰덊샇 (1-18)
     */
    int getGroupNumber(const std::string& symbol) const;
    
    /**
     * @brief 二쇨린 踰덊샇 議고쉶
     * @param symbol ?먯냼 湲고샇
     * @return 二쇨린 踰덊샇 (1-7, ???섏”=8, ?낇떚?꾩”=9)
     */
    int getPeriodNumber(const std::string& symbol) const;
    
    // ========================================================================
    // ?꾩껜 ?먯냼 紐⑸줉
    // ========================================================================
    
    /**
     * @brief 吏?먰븯??紐⑤뱺 ?먯냼 湲고샇 紐⑸줉
     */
    std::vector<std::string> getAllSymbols() const;
    
    /**
     * @brief ?깅줉???먯냼 媛쒖닔
     */
    size_t getElementCount() const;
    
    /**
     * @brief 二쇨린蹂??먯냼 紐⑸줉
     */
    std::vector<std::string> getElementsByPeriod(int period) const;
    
    /**
     * @brief 議깅퀎 ?먯냼 紐⑸줉  
     */
    std::vector<std::string> getElementsByGroup(const std::string& group) const;
    
private:
    ElementDatabase();
    ~ElementDatabase() = default;
    
    // 蹂듭궗 ?앹꽦???좊떦 ?곗궛????젣 (?깃???
    ElementDatabase(const ElementDatabase&) = delete;
    ElementDatabase& operator=(const ElementDatabase&) = delete;
    
    /**
     * @brief ?먯냼 ?곗씠?곕쿋?댁뒪 珥덇린??
     * 
     * 湲곗〈 atoms_template.cpp???뺤쟻 諛곗뿴?ㅼ쓣 濡쒕뱶:
     * - chemical_symbols, atomic_names, atomic_masses
     * - covalent_radii, jmol_colors
     */
    void initializeDatabase();
    
    /**
     * @brief ?뱀젙 ?먯냼 異붽? (?대? ?ъ슜)
     */
    void addElement(const std::string& symbol, const std::string& name, 
        float atomicRadius, float covalentRadius, 
        // const ImVec4& color, 
        const Color4f& color, 
        int atomicNumber, float mass,
        const std::string& group, int period,
        int groupNumber, const std::string& classification);
    
    // ========================================================================
    // ?대? ?곗씠??
    // ========================================================================
    std::unordered_map<std::string, ElementInfo> m_elements;
    bool m_initialized = false;
};

} // namespace data
} // namespace core
