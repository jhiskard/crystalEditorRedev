#include "element_database.h"
#include <stdexcept>
#include <algorithm>

namespace atoms {
namespace domain {

// ============================================================================
// ⭐ 원소 분류명 반환 함수 (추가)
// ============================================================================

const char* getClassificationName(ElementClassification classification) {
    switch (classification) {
        case ElementClassification::ALL_ELEMENTS:           return "All Elements";
        case ElementClassification::NON_METALS:             return "Non-metals";
        case ElementClassification::ALKALI_METALS:          return "Alkali Metals";
        case ElementClassification::ALKALINE_EARTH_METALS:  return "Alkaline Earth Metals";
        case ElementClassification::TRANSITION_METALS:      return "Transition Metals";
        case ElementClassification::POST_TRANSITION_METALS: return "Post-transition Metals";
        case ElementClassification::METALLOID:              return "Metalloid";
        case ElementClassification::HALOGENS:               return "Halogens";
        case ElementClassification::NOBLE_GASES:            return "Noble Gases";
        case ElementClassification::LANTHANIDE:             return "Lanthanide";
        case ElementClassification::ACTINIDE:               return "Actinide";
        default:                                            return "Unknown";
    }
}

// ============================================================================
// ⭐ ElementInfo 구현 (확장된 생성자)
// ============================================================================

ElementInfo::ElementInfo(const std::string& sym, const std::string& nm, 
    float aRadius, float cRadius, 
    // const ImVec4& color,
    const Color4f& color, 
    int atomicNum, float mass, 
    const std::string& grp, int per,
    int groupNum, const std::string& classif)
    : symbol(sym), name(nm), atomicRadius(aRadius), covalentRadius(cRadius),
      defaultColor(color), atomicNumber(atomicNum), atomicMass(mass),
      group(grp), period(per), groupNumber(groupNum), classification(classif) {
}


// ========================================================================
// ElementDatabase 구현
// ========================================================================

ElementDatabase& ElementDatabase::getInstance() {
    static ElementDatabase instance;
    if (!instance.m_initialized) {
        instance.initializeDatabase();
        instance.m_initialized = true;
    }
    return instance;
}

ElementDatabase::ElementDatabase() : m_initialized(false) {
}

const ElementInfo* ElementDatabase::getElementInfo(const std::string& symbol) const {
    auto it = m_elements.find(symbol);
    return (it != m_elements.end()) ? &it->second : nullptr;
}

bool ElementDatabase::hasElement(const std::string& symbol) const {
    return m_elements.find(symbol) != m_elements.end();
}

float ElementDatabase::getDefaultRadius(const std::string& symbol) const {
    const ElementInfo* info = getElementInfo(symbol);
    return info ? info->covalentRadius : 1.0f;  // 기본값 1.0f (기존 로직 유지)
}

// ImVec4 ElementDatabase::getDefaultColor(const std::string& symbol) const {
Color4f ElementDatabase::getDefaultColor(const std::string& symbol) const {
    const ElementInfo* info = getElementInfo(symbol);
    // return info ? info->defaultColor : ImVec4(0.7f, 0.7f, 0.7f, 1.0f);  // 기본값 회색
    return info ? info->defaultColor : Color4f(0.7f, 0.7f, 0.7f, 1.0f); 
}

std::string ElementDatabase::getElementName(const std::string& symbol) const {
    const ElementInfo* info = getElementInfo(symbol);
    return info ? info->name : "Unknown";
}

float ElementDatabase::getAtomicMass(const std::string& symbol) const {
    const ElementInfo* info = getElementInfo(symbol);
    return info ? info->atomicMass : 0.0f;
}

int ElementDatabase::getAtomicNumber(const std::string& symbol) const {
    const ElementInfo* info = getElementInfo(symbol);
    return info ? info->atomicNumber : 0;
}

std::vector<std::string> ElementDatabase::getAllSymbols() const {
    std::vector<std::string> symbols;
    symbols.reserve(m_elements.size());
    
    for (const auto& pair : m_elements) {
        symbols.push_back(pair.first);
    }
    
    // 원자 번호 순으로 정렬
    std::sort(symbols.begin(), symbols.end(), [this](const std::string& a, const std::string& b) {
        const ElementInfo* infoA = getElementInfo(a);
        const ElementInfo* infoB = getElementInfo(b);
        return infoA && infoB && infoA->atomicNumber < infoB->atomicNumber;
    });
    
    return symbols;
}

size_t ElementDatabase::getElementCount() const {
    return m_elements.size();
}

std::vector<std::string> ElementDatabase::getElementsByPeriod(int period) const {
    std::vector<std::string> result;
    for (const auto& pair : m_elements) {
        if (pair.second.period == period) {
            result.push_back(pair.first);
        }
    }
    return result;
}

std::vector<std::string> ElementDatabase::getElementsByGroup(const std::string& group) const {
    std::vector<std::string> result;
    for (const auto& pair : m_elements) {
        if (pair.second.group == group) {
            result.push_back(pair.first);
        }
    }
    return result;
}

// ============================================================================
// ⭐ 주기율표 렌더링을 위한 새 메서드들 (추가)
// ============================================================================

PeriodicTablePosition ElementDatabase::getElementPosition(const std::string& symbol) const {
    const ElementInfo* info = getElementInfo(symbol);
    if (info) {
        return PeriodicTablePosition(info->period, info->groupNumber);
    }
    return PeriodicTablePosition(0, 0);
}

std::string ElementDatabase::getElementClassification(const std::string& symbol) const {
    const ElementInfo* info = getElementInfo(symbol);
    return info ? info->classification : "Unknown";
}

std::vector<std::string> ElementDatabase::getElementsByClassification(const std::string& classification) const {
    std::vector<std::string> result;
    
    for (const auto& [symbol, info] : m_elements) {
        if (info.classification == classification) {
            result.push_back(symbol);
        }
    }
    
    // 원자 번호 순으로 정렬
    std::sort(result.begin(), result.end(), [this](const std::string& a, const std::string& b) {
        const ElementInfo* infoA = getElementInfo(a);
        const ElementInfo* infoB = getElementInfo(b);
        return infoA && infoB && infoA->atomicNumber < infoB->atomicNumber;
    });
    
    return result;
}

std::string ElementDatabase::getClassificationByAtomicNumber(int Z) const {
    for (const auto& [symbol, info] : m_elements) {
        if (info.atomicNumber == Z) {
            return info.classification;
        }
    }
    return "Unknown";
}

int ElementDatabase::getGroupNumber(const std::string& symbol) const {
    const ElementInfo* info = getElementInfo(symbol);
    return info ? info->groupNumber : 0;
}

int ElementDatabase::getPeriodNumber(const std::string& symbol) const {
    const ElementInfo* info = getElementInfo(symbol);
    return info ? info->period : 0;
}

// ============================================================================
// ⭐ addElement 메서드 시그니처 확장
// ============================================================================

void ElementDatabase::addElement(const std::string& symbol, const std::string& name,
    float atomicRadius, float covalentRadius, 
    // const ImVec4& color,
    const Color4f& color,  
    int atomicNumber, float mass,
    const std::string& group, int period,
    int groupNumber, const std::string& classification) {
    m_elements.emplace(symbol, ElementInfo(symbol, name, atomicRadius, covalentRadius,
                                          color, atomicNumber, mass, 
                                          group, period, groupNumber, classification));
}
// ========================================================================
// 기존 atoms_template.cpp 데이터 마이그레이션
// ========================================================================

void ElementDatabase::initializeDatabase() {
    // 기존 atoms_template.cpp에서 추출한 정적 배열들
    
    // *** 추가: 기존 chemical_symbols 배열 데이터 ***
    static const char* chemical_symbols[] = {
        "", "H", "He", "Li", "Be", "B", "C", "N", "O", "F", "Ne",
        "Na", "Mg", "Al", "Si", "P", "S", "Cl", "Ar", "K", "Ca",
        "Sc", "Ti", "V", "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn",
        "Ga", "Ge", "As", "Se", "Br", "Kr", "Rb", "Sr", "Y", "Zr",
        "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn",
        "Sb", "Te", "I", "Xe", "Cs", "Ba", "La", "Ce", "Pr", "Nd",
        "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb",
        "Lu", "Hf", "Ta", "W", "Re", "Os", "Ir", "Pt", "Au", "Hg",
        "Tl", "Pb", "Bi", "Po", "At", "Rn", "Fr", "Ra", "Ac", "Th",
        "Pa", "U", "Np", "Pu", "Am", "Cm", "Bk", "Cf", "Es", "Fm",
        "Md", "No", "Lr", "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Ds",
        "Rg", "Cn", "Nh", "Fl", "Mc", "Lv", "Ts", "Og"
    };
    
    // *** 추가: 기존 atomic_names 배열 데이터 ***
    static const char* atomic_names[] = {
        "", "Hydrogen", "Helium", "Lithium", "Beryllium", "Boron", "Carbon", 
        "Nitrogen", "Oxygen", "Fluorine", "Neon", "Sodium", "Magnesium", 
        "Aluminium", "Silicon", "Phosphorus", "Sulfur", "Chlorine", "Argon", 
        "Potassium", "Calcium", "Scandium", "Titanium", "Vanadium", "Chromium", 
        "Manganese", "Iron", "Cobalt", "Nickel", "Copper", "Zinc", "Gallium", 
        "Germanium", "Arsenic", "Selenium", "Bromine", "Krypton", "Rubidium", 
        "Strontium", "Yttrium", "Zirconium", "Niobium", "Molybdenum", 
        "Technetium", "Ruthenium", "Rhodium", "Palladium", "Silver", "Cadmium", 
        "Indium", "Tin", "Antimony", "Tellurium", "Iodine", "Xenon", "Caesium", 
        "Barium", "Lanthanum", "Cerium", "Praseodymium", "Neodymium", "Promethium", 
        "Samarium", "Europium", "Gadolinium", "Terbium", "Dysprosium", "Holmium", 
        "Erbium", "Thulium", "Ytterbium", "Lutetium", "Hafnium", "Tantalum", 
        "Tungsten", "Rhenium", "Osmium", "Iridium", "Platinum", "Gold", "Mercury", 
        "Thallium", "Lead", "Bismuth", "Polonium", "Astatine", "Radon", "Francium", 
        "Radium", "Actinium", "Thorium", "Protactinium", "Uranium", "Neptunium", 
        "Plutonium", "Americium", "Curium", "Berkelium", "Californium", 
        "Einsteinium", "Fermium", "Mendelevium", "Nobelium", "Lawrencium", 
        "Rutherfordium", "Dubnium", "Seaborgium", "Bohrium", "Hassium", 
        "Meitnerium", "Darmastadtium", "Roentgenium", "Copernicium", "Nihonium", 
        "Flerovium", "Moscovium", "Livermorium", "Tennessine", "Oganesson"
    };
    
    // *** 추가: 기존 atomic_masses 배열 데이터 ***
    static const float atomic_masses[] = {
        1.0f, 1.008f, 4.002602f, 6.94f, 9.0121831f, 10.81f, 12.011f, 14.007f, 
        15.999f, 18.998403163f, 20.1797f, 22.98976928f, 24.305f, 26.9815385f, 
        28.085f, 30.973761998f, 32.06f, 35.45f, 39.948f, 39.0983f, 40.078f, 
        44.955908f, 47.867f, 50.9415f, 51.9961f, 54.938044f, 55.845f, 58.933194f, 
        58.6934f, 63.546f, 65.38f, 69.723f, 72.630f, 74.921595f, 78.971f, 79.904f, 
        83.798f, 85.4678f, 87.62f, 88.90584f, 91.224f, 92.90637f, 95.95f, 97.90721f, 
        101.07f, 102.90550f, 106.42f, 107.8682f, 112.414f, 114.818f, 118.710f, 
        121.760f, 127.60f, 126.90447f, 131.293f, 132.90545196f, 137.327f, 
        138.90547f, 140.116f, 140.90766f, 144.242f, 144.91276f, 150.36f, 151.964f, 
        157.25f, 158.92535f, 162.500f, 164.93033f, 167.259f, 168.93422f, 173.054f, 
        174.9668f, 178.49f, 180.94788f, 183.84f, 186.207f, 190.23f, 192.217f, 
        195.084f, 196.966569f, 200.592f, 204.38f, 207.2f, 208.98040f, 208.98243f, 
        209.98715f, 222.01758f, 223.01974f, 226.02541f, 227.02775f, 232.0377f, 
        231.03588f, 238.02891f, 237.04817f, 244.06421f, 243.06138f, 247.07035f, 
        247.07031f, 251.07959f, 252.0830f, 257.09511f, 258.09843f, 259.1010f, 
        262.110f, 267.122f, 268.126f, 271.134f, 270.133f, 269.1338f, 278.156f, 
        281.165f, 281.166f, 285.177f, 286.182f, 289.190f, 289.194f, 293.204f, 
        293.208f, 294.214f
    };
    
    // *** 추가: 기존 covalent_radii 배열 데이터 ***
    static const float covalent_radii[] = {
        0.0f, 0.31f, 0.28f, 1.28f, 0.96f, 0.84f, 0.76f, 0.71f, 0.66f, 0.57f, 
        0.58f, 1.66f, 1.41f, 1.21f, 1.11f, 1.07f, 1.05f, 1.02f, 1.06f, 2.03f, 
        1.76f, 1.70f, 1.60f, 1.53f, 1.39f, 1.39f, 1.32f, 1.26f, 1.24f, 1.32f, 
        1.22f, 1.22f, 1.20f, 1.19f, 1.20f, 1.20f, 1.16f, 2.20f, 1.95f, 1.90f, 
        1.75f, 1.64f, 1.54f, 1.47f, 1.46f, 1.42f, 1.39f, 1.45f, 1.44f, 1.42f, 
        1.39f, 1.39f, 1.38f, 1.39f, 1.40f, 2.44f, 2.15f, 2.07f, 2.04f, 2.03f, 
        2.01f, 1.99f, 1.98f, 1.98f, 1.96f, 1.94f, 1.92f, 1.92f, 1.89f, 1.90f, 
        1.87f, 1.87f, 1.75f, 1.70f, 1.62f, 1.51f, 1.44f, 1.41f, 1.36f, 1.36f, 
        1.32f, 1.45f, 1.46f, 1.48f, 1.40f, 1.50f, 1.50f, 2.60f, 2.21f, 2.15f, 
        2.06f, 2.00f, 1.96f, 1.90f, 1.87f, 1.80f, 1.69f, 1.68f, 1.68f, 1.65f, 
        1.67f, 1.73f, 1.76f, 1.61f, 1.57f, 1.49f, 1.43f, 1.41f, 1.34f, 1.29f, 
        1.28f, 1.21f, 1.22f, 1.36f, 1.43f, 1.62f, 1.75f, 1.65f, 1.57f
    };
    
    // *** 추가: 기존 jmol_colors 구조체 정의 ***
    struct Color {
        float r, g, b;
        Color(float red, float green, float blue) : r(red), g(green), b(blue) {}
    };
    
    // *** 추가: 기존 jmol_colors 배열 데이터 (Jmol 표준 색상) ***
    static const Color jmol_colors[] = {
        Color(1.0f, 0.078f, 0.576f),    // 0: 기본 (사용 안함)
        Color(1.0f, 1.0f, 1.0f),        // 1: H  - 흰색
        Color(0.851f, 1.0f, 1.0f),      // 2: He - 연한 청록색
        Color(0.8f, 0.502f, 1.0f),      // 3: Li - 연한 보라색
        Color(0.761f, 1.0f, 0.0f),      // 4: Be - 연한 녹색
        Color(1.0f, 0.710f, 0.710f),    // 5: B  - 연한 분홍색
        Color(0.565f, 0.565f, 0.565f),  // 6: C  - 회색
        Color(0.188f, 0.314f, 0.973f),  // 7: N  - 파란색
        Color(1.0f, 0.051f, 0.051f),    // 8: O  - 빨간색
        Color(0.565f, 0.878f, 0.314f),  // 9: F  - 연한 녹색
        Color(0.702f, 0.890f, 0.961f),  // 10: Ne - 연한 파란색
        Color(0.671f, 0.361f, 0.949f),  // 11: Na - 보라색
        Color(0.541f, 1.0f, 0.0f),      // 12: Mg - 녹색
        Color(0.749f, 0.651f, 0.651f),  // 13: Al - 연한 회색
        Color(0.941f, 0.784f, 0.627f),  // 14: Si - 연한 갈색
        Color(1.0f, 0.502f, 0.0f),      // 15: P  - 주황색
        Color(1.0f, 1.0f, 0.188f),      // 16: S  - 노란색
        Color(0.122f, 0.941f, 0.122f),  // 17: Cl - 녹색
        Color(0.502f, 0.820f, 0.890f),  // 18: Ar - 연한 파란색
        Color(0.561f, 0.251f, 0.831f),  // 19: K  - 보라색
        Color(0.239f, 1.0f, 0.0f),      // 20: Ca - 녹색
        Color(0.902f, 0.902f, 0.902f),  // 21: Sc - 회색
        Color(0.749f, 0.761f, 0.780f),  // 22: Ti - 회색
        Color(0.651f, 0.651f, 0.671f),  // 23: V  - 회색
        Color(0.541f, 0.600f, 0.780f),  // 24: Cr - 회청색
        Color(0.611f, 0.478f, 0.780f),  // 25: Mn - 보라색
        Color(0.878f, 0.400f, 0.200f),  // 26: Fe - 갈색
        Color(0.941f, 0.565f, 0.627f),  // 27: Co - 분홍색
        Color(0.314f, 0.816f, 0.314f),  // 28: Ni - 녹색
        Color(0.784f, 0.502f, 0.200f),  // 29: Cu - 갈색
        Color(0.490f, 0.502f, 0.690f),  // 30: Zn - 회색
        Color(0.761f, 0.561f, 0.561f),  // 31: Ga - 회색
        Color(0.400f, 0.561f, 0.561f),  // 32: Ge - 회색
        Color(0.741f, 0.502f, 0.890f),  // 33: As - 보라색
        Color(1.0f, 0.631f, 0.0f),      // 34: Se - 주황색
        Color(0.651f, 0.161f, 0.161f),  // 35: Br - 어두운 빨간색
        Color(0.361f, 0.722f, 0.820f),  // 36: Kr - 파란색
        Color(0.439f, 0.180f, 0.690f),  // 37: Rb - 보라색
        Color(0.0f, 1.0f, 0.0f),        // 38: Sr - 녹색
        Color(0.580f, 1.0f, 1.0f),      // 39: Y  - 연한 청록색
        Color(0.580f, 0.878f, 0.878f),  // 40: Zr - 청록색
        Color(0.451f, 0.761f, 0.788f),  // 41: Nb - 파란색
        Color(0.329f, 0.710f, 0.710f),  // 42: Mo - 청록색
        Color(0.231f, 0.620f, 0.620f),  // 43: Tc - 청록색
        Color(0.141f, 0.561f, 0.561f),  // 44: Ru - 청록색
        Color(0.039f, 0.490f, 0.549f),  // 45: Rh - 파란색
        Color(0.0f, 0.412f, 0.522f),    // 46: Pd - 파란색
        Color(0.753f, 0.753f, 0.753f),  // 47: Ag - 회색
        Color(1.0f, 0.851f, 0.561f),    // 48: Cd - 연한 노란색
        Color(0.651f, 0.459f, 0.451f),  // 49: In - 갈색
        Color(0.400f, 0.502f, 0.502f),  // 50: Sn - 회색
        Color(0.620f, 0.388f, 0.710f),  // 51: Sb - 보라색
        Color(0.831f, 0.478f, 0.0f),    // 52: Te - 주황색
        Color(0.580f, 0.0f, 0.580f),    // 53: I  - 보라색
        Color(0.259f, 0.620f, 0.690f),  // 54: Xe - 파란색
        Color(0.341f, 0.090f, 0.561f),  // 55: Cs - 보라색
        Color(0.0f, 0.788f, 0.0f),      // 56: Ba - 녹색
        Color(0.439f, 0.831f, 1.0f),    // 57: La - 연한 파란색
        Color(1.0f, 1.0f, 0.780f),      // 58: Ce - 연한 노란색
        Color(0.851f, 1.0f, 0.780f),    // 59: Pr - 연한 녹색
        Color(0.780f, 1.0f, 0.780f),    // 60: Nd - 연한 녹색
        Color(0.639f, 1.0f, 0.780f),    // 61: Pm - 연한 녹색
        Color(0.561f, 1.0f, 0.780f),    // 62: Sm - 연한 녹색
        Color(0.380f, 1.0f, 0.780f),    // 63: Eu - 연한 녹색
        Color(0.271f, 1.0f, 0.780f),    // 64: Gd - 연한 녹색
        Color(0.188f, 1.0f, 0.780f),    // 65: Tb - 연한 녹색
        Color(0.122f, 1.0f, 0.780f),    // 66: Dy - 연한 녹색
        Color(0.0f, 1.0f, 0.612f),      // 67: Ho - 연한 녹색
        Color(0.0f, 0.902f, 0.459f),    // 68: Er - 녹색
        Color(0.0f, 0.831f, 0.322f),    // 69: Tm - 녹색
        Color(0.0f, 0.749f, 0.220f),    // 70: Yb - 녹색
        Color(0.0f, 0.671f, 0.141f),    // 71: Lu - 녹색
        Color(0.302f, 0.761f, 1.0f),    // 72: Hf - 파란색
        Color(0.302f, 0.651f, 1.0f),    // 73: Ta - 파란색
        Color(0.129f, 0.580f, 0.839f),  // 74: W  - 파란색
        Color(0.149f, 0.490f, 0.671f),  // 75: Re - 파란색
        Color(0.149f, 0.400f, 0.588f),  // 76: Os - 파란색
        Color(0.090f, 0.329f, 0.529f),  // 77: Ir - 파란색
        Color(0.816f, 0.816f, 0.878f),  // 78: Pt - 회색
        Color(1.0f, 0.820f, 0.137f),    // 79: Au - 금색
        Color(0.722f, 0.722f, 0.816f),  // 80: Hg - 회색
        Color(0.651f, 0.329f, 0.302f),  // 81: Tl - 갈색
        Color(0.341f, 0.349f, 0.380f),  // 82: Pb - 어두운 회색
        Color(0.620f, 0.310f, 0.710f),  // 83: Bi - 보라색
        Color(0.671f, 0.361f, 0.0f),    // 84: Po - 갈색
        Color(0.459f, 0.310f, 0.271f),  // 85: At - 갈색
        Color(0.259f, 0.510f, 0.588f),  // 86: Rn - 파란색
        Color(0.259f, 0.0f, 0.400f),    // 87: Fr - 어두운 보라색
        Color(0.0f, 0.490f, 0.0f),      // 88: Ra - 녹색
        Color(0.439f, 0.671f, 0.980f),  // 89: Ac - 연한 파란색
        Color(0.0f, 0.729f, 1.0f),      // 90: Th - 청록색
        Color(0.0f, 0.631f, 1.0f),      // 91: Pa - 파란색
        Color(0.0f, 0.561f, 1.0f),      // 92: U  - 파란색
        Color(0.0f, 0.502f, 1.0f),      // 93: Np - 파란색
        Color(0.0f, 0.420f, 1.0f),      // 94: Pu - 파란색
        Color(0.329f, 0.361f, 0.949f),  // 95: Am - 파란색
        Color(0.471f, 0.361f, 0.890f),  // 96: Cm - 보라색
        Color(0.541f, 0.310f, 0.890f),  // 97: Bk - 보라색
        Color(0.631f, 0.212f, 0.831f),  // 98: Cf - 보라색
        Color(0.702f, 0.122f, 0.831f),  // 99: Es - 보라색
        Color(0.702f, 0.122f, 0.729f),  // 100: Fm - 보라색
        Color(0.702f, 0.051f, 0.651f),  // 101: Md - 보라색
        Color(0.741f, 0.051f, 0.529f),  // 102: No - 보라색
        Color(0.780f, 0.0f, 0.400f),    // 103: Lr - 보라색
        Color(0.800f, 0.0f, 0.349f),    // 104: Rf - 보라색
        Color(0.820f, 0.0f, 0.310f),    // 105: Db - 보라색
        Color(0.851f, 0.0f, 0.271f),    // 106: Sg - 보라색
        Color(0.878f, 0.0f, 0.220f),    // 107: Bh - 보라색
        Color(0.906f, 0.0f, 0.180f),    // 108: Hs - 보라색
        Color(0.922f, 0.0f, 0.149f),    // 109: Mt - 보라색
        Color(0.922f, 0.0f, 0.149f),    // 110: Ds - 보라색
        Color(0.922f, 0.0f, 0.149f),    // 111: Rg - 보라색
        Color(0.922f, 0.0f, 0.149f),    // 112: Cn - 보라색
        Color(0.922f, 0.0f, 0.149f),    // 113: Nh - 보라색
        Color(0.922f, 0.0f, 0.149f),    // 114: Fl - 보라색
        Color(0.922f, 0.0f, 0.149f),    // 115: Mc - 보라색
        Color(0.922f, 0.0f, 0.149f),    // 116: Lv - 보라색
        Color(0.922f, 0.0f, 0.149f),    // 117: Ts - 보라색
        Color(0.922f, 0.0f, 0.149f)     // 118: Og - 보라색
    };
    
    // 배열 크기 확인
    const size_t numElements = sizeof(chemical_symbols) / sizeof(chemical_symbols[0]);
    
    // // *** 수정: 모든 원소 데이터를 ElementDatabase에 추가 ***
    // for (size_t i = 1; i < numElements && i < 119; ++i) {  // 0번 인덱스는 빈 문자열이므로 제외
    //     std::string symbol = chemical_symbols[i];
    //     std::string name = atomic_names[i];
    //     float mass = atomic_masses[i];
    //     float covalentR = covalent_radii[i];
        
    //     // Jmol 색상 (인덱스 범위 체크)
    //     ImVec4 color(0.7f, 0.7f, 0.7f, 1.0f);  // 기본 회색
    //     if (i < sizeof(jmol_colors) / sizeof(jmol_colors[0])) {
    //         const Color& jmolColor = jmol_colors[i];
    //         color = ImVec4(jmolColor.r, jmolColor.g, jmolColor.b, 1.0f);
    //     }
        
    //     // 원자 반지름은 공유결합 반지름 사용 (기존 로직 유지)
    //     float atomicR = covalentR;
        
    //     // 주기와 족 정보 (간단한 계산으로 추정)
    //     int period = 1;
    //     std::string group = "Unknown";
        
    //     // 간단한 주기 계산
    //     if (i <= 2) period = 1;
    //     else if (i <= 10) period = 2;
    //     else if (i <= 18) period = 3;
    //     else if (i <= 36) period = 4;
    //     else if (i <= 54) period = 5;
    //     else if (i <= 86) period = 6;
    //     else period = 7;
        
    //     addElement(symbol, name, atomicR, covalentR, color, 
    //               static_cast<int>(i), mass, group, period);
    // }
    
    // // *** 추가: 특수 원소들 (*La, *Ac) - 기존 lanthanides 벡터에서 추출 ***
    // // Lanthanum 변형
    // addElement("*La", "Lanthanum", covalent_radii[57], covalent_radii[57],
    //            ImVec4(jmol_colors[57].r, jmol_colors[57].g, jmol_colors[57].b, 1.0f),
    //            57, atomic_masses[57], "Lanthanide", 6);
    
    // // Actinium 변형 
    // addElement("*Ac", "Actinium", covalent_radii[89], covalent_radii[89],
    //            ImVec4(jmol_colors[89].r, jmol_colors[89].g, jmol_colors[89].b, 1.0f),
    //            89, atomic_masses[89], "Actinide", 7);

    // ⭐ 주기율표 위치 매핑 데이터 (Period, Group)
    struct ElementPosition {
        int period;
        int group;
        const char* classification;
    };

    // 118개 원소의 주기율표 위치 정보
    static const ElementPosition positions[] = {
        {0, 0, ""},                          // 0: 빈 자리
        {1, 1, "Non-metals"},                // 1: H
        {1, 18, "Noble Gases"},              // 2: He
        {2, 1, "Alkali Metals"},             // 3: Li
        {2, 2, "Alkaline Earth Metals"},     // 4: Be
        {2, 13, "Metalloid"},                // 5: B
        {2, 14, "Non-metals"},               // 6: C
        {2, 15, "Non-metals"},               // 7: N
        {2, 16, "Non-metals"},               // 8: O
        {2, 17, "Halogens"},                 // 9: F
        {2, 18, "Noble Gases"},              // 10: Ne
        {3, 1, "Alkali Metals"},             // 11: Na
        {3, 2, "Alkaline Earth Metals"},     // 12: Mg
        {3, 13, "Post-transition Metals"},   // 13: Al
        {3, 14, "Metalloid"},                // 14: Si
        {3, 15, "Non-metals"},               // 15: P
        {3, 16, "Non-metals"},               // 16: S
        {3, 17, "Halogens"},                 // 17: Cl
        {3, 18, "Noble Gases"},              // 18: Ar
        {4, 1, "Alkali Metals"},             // 19: K
        {4, 2, "Alkaline Earth Metals"},     // 20: Ca
        {4, 3, "Transition Metals"},         // 21: Sc
        {4, 4, "Transition Metals"},         // 22: Ti
        {4, 5, "Transition Metals"},         // 23: V
        {4, 6, "Transition Metals"},         // 24: Cr
        {4, 7, "Transition Metals"},         // 25: Mn
        {4, 8, "Transition Metals"},         // 26: Fe
        {4, 9, "Transition Metals"},         // 27: Co
        {4, 10, "Transition Metals"},        // 28: Ni
        {4, 11, "Transition Metals"},        // 29: Cu
        {4, 12, "Transition Metals"},        // 30: Zn
        {4, 13, "Post-transition Metals"},   // 31: Ga
        {4, 14, "Metalloid"},                // 32: Ge
        {4, 15, "Metalloid"},                // 33: As
        {4, 16, "Non-metals"},               // 34: Se
        {4, 17, "Halogens"},                 // 35: Br
        {4, 18, "Noble Gases"},              // 36: Kr
        {5, 1, "Alkali Metals"},             // 37: Rb
        {5, 2, "Alkaline Earth Metals"},     // 38: Sr
        {5, 3, "Transition Metals"},         // 39: Y
        {5, 4, "Transition Metals"},         // 40: Zr
        {5, 5, "Transition Metals"},         // 41: Nb
        {5, 6, "Transition Metals"},         // 42: Mo
        {5, 7, "Transition Metals"},         // 43: Tc
        {5, 8, "Transition Metals"},         // 44: Ru
        {5, 9, "Transition Metals"},         // 45: Rh
        {5, 10, "Transition Metals"},        // 46: Pd
        {5, 11, "Transition Metals"},        // 47: Ag
        {5, 12, "Transition Metals"},        // 48: Cd
        {5, 13, "Post-transition Metals"},   // 49: In
        {5, 14, "Post-transition Metals"},   // 50: Sn
        {5, 15, "Metalloid"},                // 51: Sb
        {5, 16, "Metalloid"},                // 52: Te
        {5, 17, "Halogens"},                 // 53: I
        {5, 18, "Noble Gases"},              // 54: Xe
        {6, 1, "Alkali Metals"},             // 55: Cs
        {6, 2, "Alkaline Earth Metals"},     // 56: Ba
        {6, 3, "Lanthanide"},                // 57: La (주기율표에서 *La 표시)
        {8, 4, "Lanthanide"},                // 58: Ce (란타넘족 시작)
        {8, 5, "Lanthanide"},                // 59: Pr
        {8, 6, "Lanthanide"},                // 60: Nd
        {8, 7, "Lanthanide"},                // 61: Pm
        {8, 8, "Lanthanide"},                // 62: Sm
        {8, 9, "Lanthanide"},                // 63: Eu
        {8, 10, "Lanthanide"},               // 64: Gd
        {8, 11, "Lanthanide"},               // 65: Tb
        {8, 12, "Lanthanide"},               // 66: Dy
        {8, 13, "Lanthanide"},               // 67: Ho
        {8, 14, "Lanthanide"},               // 68: Er
        {8, 15, "Lanthanide"},               // 69: Tm
        {8, 16, "Lanthanide"},               // 70: Yb
        {8, 17, "Lanthanide"},               // 71: Lu
        {6, 4, "Transition Metals"},         // 72: Hf
        {6, 5, "Transition Metals"},         // 73: Ta
        {6, 6, "Transition Metals"},         // 74: W
        {6, 7, "Transition Metals"},         // 75: Re
        {6, 8, "Transition Metals"},         // 76: Os
        {6, 9, "Transition Metals"},         // 77: Ir
        {6, 10, "Transition Metals"},        // 78: Pt
        {6, 11, "Transition Metals"},        // 79: Au
        {6, 12, "Transition Metals"},        // 80: Hg
        {6, 13, "Post-transition Metals"},   // 81: Tl
        {6, 14, "Post-transition Metals"},   // 82: Pb
        {6, 15, "Post-transition Metals"},   // 83: Bi
        {6, 16, "Metalloid"},                // 84: Po
        {6, 17, "Halogens"},                 // 85: At
        {6, 18, "Noble Gases"},              // 86: Rn
        {7, 1, "Alkali Metals"},             // 87: Fr
        {7, 2, "Alkaline Earth Metals"},     // 88: Ra
        {7, 3, "Actinide"},                  // 89: Ac (주기율표에서 *Ac 표시)
        {9, 4, "Actinide"},                  // 90: Th (악티늄족 시작)
        {9, 5, "Actinide"},                  // 91: Pa
        {9, 6, "Actinide"},                  // 92: U
        {9, 7, "Actinide"},                  // 93: Np
        {9, 8, "Actinide"},                  // 94: Pu
        {9, 9, "Actinide"},                  // 95: Am
        {9, 10, "Actinide"},                 // 96: Cm
        {9, 11, "Actinide"},                 // 97: Bk
        {9, 12, "Actinide"},                 // 98: Cf
        {9, 13, "Actinide"},                 // 99: Es
        {9, 14, "Actinide"},                 // 100: Fm
        {9, 15, "Actinide"},                 // 101: Md
        {9, 16, "Actinide"},                 // 102: No
        {9, 17, "Actinide"},                 // 103: Lr
        {7, 4, "Transition Metals"},         // 104: Rf
        {7, 5, "Transition Metals"},         // 105: Db
        {7, 6, "Transition Metals"},         // 106: Sg
        {7, 7, "Transition Metals"},         // 107: Bh
        {7, 8, "Transition Metals"},         // 108: Hs
        {7, 9, "Transition Metals"},         // 109: Mt
        {7, 10, "Transition Metals"},        // 110: Ds
        {7, 11, "Transition Metals"},        // 111: Rg
        {7, 12, "Transition Metals"},        // 112: Cn
        {7, 13, "Post-transition Metals"},   // 113: Nh
        {7, 14, "Post-transition Metals"},   // 114: Fl
        {7, 15, "Post-transition Metals"},   // 115: Mc
        {7, 16, "Post-transition Metals"},   // 116: Lv
        {7, 17, "Halogens"},                 // 117: Ts
        {7, 18, "Noble Gases"}               // 118: Og
    };
    
    // ⭐ 모든 원소 데이터를 확장된 정보와 함께 추가
    for (size_t i = 1; i < numElements && i < 119; ++i) {
        std::string symbol = chemical_symbols[i];
        std::string name = atomic_names[i];
        float mass = atomic_masses[i];
        float covalentR = covalent_radii[i];
        
        // Jmol 색상
        // ImVec4 color(0.7f, 0.7f, 0.7f, 1.0f);
        Color4f color(0.7f, 0.7f, 0.7f, 1.0f);
        if (i < sizeof(jmol_colors) / sizeof(jmol_colors[0])) {
            const Color& jmolColor = jmol_colors[i];
            // color = ImVec4(jmolColor.r, jmolColor.g, jmolColor.b, 1.0f);
            color = Color4f(jmolColor.r, jmolColor.g, jmolColor.b, 1.0f);
        }
        
        float atomicR = covalentR;
        
        // ⭐ 주기율표 위치 정보 가져오기
        int period = positions[i].period;
        int group = positions[i].group;
        const char* classification = positions[i].classification;
        
        // 족 정보 문자열 생성
        std::string groupStr = (group > 0) ? std::to_string(group) : "Unknown";
        
        addElement(symbol, name, atomicR, covalentR, color, 
                  static_cast<int>(i), mass, 
                  groupStr, period, group, classification);
    }
    
    // ⭐ 특수 원소들 (*La, *Ac) - 주기율표 위치 표시용
    addElement("*La", "Lanthanum", covalent_radii[57], covalent_radii[57],
            //    ImVec4(jmol_colors[57].r, jmol_colors[57].g, jmol_colors[57].b, 1.0f),
               Color4f(jmol_colors[57].r, jmol_colors[57].g, jmol_colors[57].b, 1.0f),
               57, atomic_masses[57], "3", 6, 3, "Lanthanide");
    
    addElement("*Ac", "Actinium", covalent_radii[89], covalent_radii[89],
            //    ImVec4(jmol_colors[89].r, jmol_colors[89].g, jmol_colors[89].b, 1.0f),
               Color4f(jmol_colors[89].r, jmol_colors[89].g, jmol_colors[89].b, 1.0f),
               89, atomic_masses[89], "3", 7, 3, "Actinide");
}

} // namespace domain
} // namespace atoms