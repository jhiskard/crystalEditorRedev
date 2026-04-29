// atoms/infrastructure/chgcar_parser.h
#pragma once

#include <functional>
#include <istream>
#include <string>
#include <vector>
#include <array>

namespace core {
namespace io {

class ChgcarParser {
public:
    using ProgressCallback = std::function<void(float)>;

    struct ParseResult {
        bool success = false;
        std::string errorMessage;
        
        // ?쒖뒪???뺣낫
        std::string systemName;
        float scale = 1.0f;
        
        // 寃⑹옄 ?뺣낫
        float lattice[3][3] = {};
        
        // ?먯옄 ?뺣낫
        std::vector<std::string> elements;
        std::vector<int> atomCounts;
        std::vector<std::array<float, 3>> positions;
        bool isDirect = true;
        
        // ?꾪븯 諛??
        std::array<int, 3> gridShape = {0, 0, 0};
        std::vector<float> density;
        
        // ?듦퀎
        float minValue = 0.0f;
        float maxValue = 0.0f;
        
        // ?ㅽ? 遺꾧레 (?좏깮??
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

} // namespace io
} // namespace core

