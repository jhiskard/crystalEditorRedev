// atoms/infrastructure/chgcar_parser.h
#pragma once

#include <functional>
#include <istream>
#include <string>
#include <vector>
#include <array>

namespace atoms {
namespace infrastructure {

class ChgcarParser {
public:
    using ProgressCallback = std::function<void(float)>;

    struct ParseResult {
        bool success = false;
        std::string errorMessage;
        
        // 시스템 정보
        std::string systemName;
        float scale = 1.0f;
        
        // 격자 정보
        float lattice[3][3] = {};
        
        // 원자 정보
        std::vector<std::string> elements;
        std::vector<int> atomCounts;
        std::vector<std::array<float, 3>> positions;
        bool isDirect = true;
        
        // 전하 밀도
        std::array<int, 3> gridShape = {0, 0, 0};
        std::vector<float> density;
        
        // 통계
        float minValue = 0.0f;
        float maxValue = 0.0f;
        
        // 스핀 분극 (선택적)
        bool hasSpinPolarization = false;
        std::vector<float> spinDensity;
    };
    
    static ParseResult parse(const std::string& filePath, ProgressCallback progressCallback = nullptr);
    static ParseResult parseFromString(const std::string& content);
    
private:
    static bool parseHeader(std::istream& stream, ParseResult& result);
    static bool parseAtomPositions(std::istream& stream, ParseResult& result);
    static bool parseDensityData(std::istream& stream,
                                 ParseResult& result,
                                 const ProgressCallback& progressCallback = nullptr,
                                 std::streampos totalSize = 0);
};

} // namespace infrastructure
} // namespace atoms
