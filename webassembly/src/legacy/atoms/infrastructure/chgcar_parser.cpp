// atoms/infrastructure/chgcar_parser.cpp
#include "chgcar_parser.h"
#include "../../config/log_config.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace atoms {
namespace infrastructure {

namespace {
    constexpr int kMinProgressIntervalMs = 1000;
    constexpr uint64_t kProgressLineStep = 256;

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }
    
    std::vector<std::string> split(const std::string& str) {
        std::vector<std::string> tokens;
        std::istringstream iss(str);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }
    
    bool isInteger(const std::string& str) {
        if (str.empty()) return false;
        size_t start = (str[0] == '-' || str[0] == '+') ? 1 : 0;
        for (size_t i = start; i < str.length(); ++i) {
            if (!std::isdigit(str[i])) return false;
        }
        return start < str.length();
    }
    
    // VASP 과학적 표기법 파싱 (예: -.76873636857E+01, 0.30745948045E+01)
    float parseVaspFloat(const std::string& str) {
        std::string s = str;
        
        // VASP 특수 케이스: 공백 없이 붙어있는 음수 (예: "-.76873636857E+01")
        // 이미 split으로 분리되어 있으므로 바로 파싱
        
        // E를 e로 변환 (일부 시스템 호환성)
        for (char& c : s) {
            if (c == 'D' || c == 'd') c = 'e';  // Fortran D 표기법
            if (c == 'E') c = 'e';
        }
        
        return std::stof(s);
    }

    void reportProgress(std::istream& stream,
                        std::streampos totalSize,
                        const ChgcarParser::ProgressCallback& progressCallback,
                        float& lastProgress,
                        std::chrono::steady_clock::time_point& lastProgressTime,
                        bool force = false) {
        if (!progressCallback) {
            return;
        }
        if (totalSize <= 0) {
            return;
        }

        auto now = std::chrono::steady_clock::now();
        if (!force && lastProgressTime.time_since_epoch().count() != 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastProgressTime);
            if (elapsed.count() < kMinProgressIntervalMs) {
                return;
            }
        }

        std::streampos pos = stream.tellg();
        if (pos == std::streampos(-1)) {
            if (!force) {
                return;
            }
            pos = totalSize;
        } else if (pos <= 0 && !force) {
            return;
        }

        float progress = static_cast<float>(pos) / static_cast<float>(totalSize);
        if (progress < 0.0f) {
            progress = 0.0f;
        } else if (progress > 1.0f) {
            progress = 1.0f;
        }

        if (!force && lastProgress >= 0.0f && progress - lastProgress < 0.01f && progress < 1.0f) {
            return;
        }

        lastProgress = progress;
        lastProgressTime = now;
        progressCallback(progress);
    }
}

ChgcarParser::ParseResult ChgcarParser::parse(const std::string& filePath, ProgressCallback progressCallback) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        ParseResult result;
        result.errorMessage = "Failed to open file: " + filePath;
        return result;
    }

    std::streampos fileSize = 0;
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    ParseResult result;
    float lastProgress = -1.0f;
    std::chrono::steady_clock::time_point lastProgressTime{};
    reportProgress(file, fileSize, progressCallback, lastProgress, lastProgressTime, true);

    if (!parseHeader(file, result)) {
        return result;
    }

    if (!parseAtomPositions(file, result)) {
        return result;
    }

    if (!parseDensityData(file, result, progressCallback, fileSize)) {
        return result;
    }

    reportProgress(file, fileSize, progressCallback, lastProgress, lastProgressTime, true);
    if (progressCallback) {
        progressCallback(1.0f);
    }

    result.success = true;
    SPDLOG_INFO("CHGCAR parsed successfully: {}x{}x{} grid, {} atoms",
                result.gridShape[0], result.gridShape[1], result.gridShape[2],
                result.positions.size());
    return result;
}

ChgcarParser::ParseResult ChgcarParser::parseFromString(const std::string& content) {
    ParseResult result;
    std::istringstream stream(content);
    
    if (!parseHeader(stream, result)) {
        return result;
    }
    
    if (!parseAtomPositions(stream, result)) {
        return result;
    }
    
    if (!parseDensityData(stream, result)) {
        return result;
    }
    
    result.success = true;
    SPDLOG_INFO("CHGCAR parsed successfully: {}x{}x{} grid, {} atoms",
                result.gridShape[0], result.gridShape[1], result.gridShape[2],
                result.positions.size());
    
    return result;
}

bool ChgcarParser::parseHeader(std::istream& stream, ParseResult& result) {
    std::string line;
    
    // 1줄: 시스템 이름
    if (!std::getline(stream, line)) {
        result.errorMessage = "Failed to read system name";
        return false;
    }
    result.systemName = trim(line);
    SPDLOG_DEBUG("System name: {}", result.systemName);
    
    // 2줄: 스케일 팩터
    if (!std::getline(stream, line)) {
        result.errorMessage = "Failed to read scale factor";
        return false;
    }
    try {
        result.scale = std::stof(trim(line));
    } catch (...) {
        result.errorMessage = "Invalid scale factor: " + line;
        return false;
    }
    SPDLOG_DEBUG("Scale factor: {}", result.scale);
    
    // 3-5줄: 격자 벡터
    for (int i = 0; i < 3; ++i) {
        if (!std::getline(stream, line)) {
            result.errorMessage = "Failed to read lattice vector " + std::to_string(i);
            return false;
        }
        auto tokens = split(line);
        if (tokens.size() < 3) {
            result.errorMessage = "Invalid lattice vector: " + line;
            return false;
        }
        try {
            result.lattice[i][0] = std::stof(tokens[0]) * result.scale;
            result.lattice[i][1] = std::stof(tokens[1]) * result.scale;
            result.lattice[i][2] = std::stof(tokens[2]) * result.scale;
        } catch (...) {
            result.errorMessage = "Failed to parse lattice vector: " + line;
            return false;
        }
    }
    SPDLOG_DEBUG("Lattice: [{:.4f}, {:.4f}, {:.4f}], [{:.4f}, {:.4f}, {:.4f}], [{:.4f}, {:.4f}, {:.4f}]",
                 result.lattice[0][0], result.lattice[0][1], result.lattice[0][2],
                 result.lattice[1][0], result.lattice[1][1], result.lattice[1][2],
                 result.lattice[2][0], result.lattice[2][1], result.lattice[2][2]);
    
    // 6줄: 원소 기호 또는 원자 개수
    if (!std::getline(stream, line)) {
        result.errorMessage = "Failed to read element line";
        return false;
    }
    
    auto tokens = split(line);
    if (tokens.empty()) {
        result.errorMessage = "Empty element/count line";
        return false;
    }
    
    // 첫 토큰이 정수면 VASP 4 포맷
    if (isInteger(tokens[0])) {
        for (const auto& t : tokens) {
            result.atomCounts.push_back(std::stoi(t));
            result.elements.push_back("X");
        }
    } else {
        // VASP 5+: 원소 기호 (예: "Si")
        result.elements = tokens;
        
        // 7줄: 원자 개수
        if (!std::getline(stream, line)) {
            result.errorMessage = "Failed to read atom counts";
            return false;
        }
        tokens = split(line);
        for (const auto& t : tokens) {
            try {
                result.atomCounts.push_back(std::stoi(t));
            } catch (...) {
                result.errorMessage = "Invalid atom count: " + t;
                return false;
            }
        }
    }
    
    SPDLOG_DEBUG("Elements: {}, Counts: {}", 
                 result.elements.size(), result.atomCounts.size());
    
    return true;
}

bool ChgcarParser::parseAtomPositions(std::istream& stream, ParseResult& result) {
    std::string line;
    
    // 좌표 타입 줄
    if (!std::getline(stream, line)) {
        result.errorMessage = "Failed to read coordinate type";
        return false;
    }
    
    std::string coordLine = trim(line);
    
    // Selective dynamics 체크
    if (!coordLine.empty() && (coordLine[0] == 'S' || coordLine[0] == 's')) {
        if (!std::getline(stream, line)) {
            result.errorMessage = "Failed to read coordinate type after Selective dynamics";
            return false;
        }
        coordLine = trim(line);
    }
    
    // Direct vs Cartesian
    result.isDirect = (!coordLine.empty() && (coordLine[0] == 'D' || coordLine[0] == 'd'));
    SPDLOG_DEBUG("Coordinate type: {}", result.isDirect ? "Direct" : "Cartesian");
    
    // 총 원자 수
    int totalAtoms = 0;
    for (int count : result.atomCounts) {
        totalAtoms += count;
    }
    
    // 원자 좌표 읽기
    result.positions.reserve(totalAtoms);
    for (int i = 0; i < totalAtoms; ++i) {
        if (!std::getline(stream, line)) {
            result.errorMessage = "Failed to read atom position " + std::to_string(i);
            return false;
        }
        
        auto tokens = split(line);
        if (tokens.size() < 3) {
            result.errorMessage = "Invalid atom position: " + line;
            return false;
        }
        
        try {
            std::array<float, 3> pos = {
                std::stof(tokens[0]),
                std::stof(tokens[1]),
                std::stof(tokens[2])
            };
            result.positions.push_back(pos);
        } catch (...) {
            result.errorMessage = "Failed to parse position: " + line;
            return false;
        }
    }
    
    SPDLOG_DEBUG("Parsed {} atom positions", result.positions.size());
    
    return true;
}

bool ChgcarParser::parseDensityData(std::istream& stream,
                                    ParseResult& result,
                                    const ProgressCallback& progressCallback,
                                    std::streampos totalSize) {
    std::string line;
    uint64_t lineCounter = 0;
    float lastProgress = -1.0f;
    std::chrono::steady_clock::time_point lastProgressTime{};

    reportProgress(stream, totalSize, progressCallback, lastProgress, lastProgressTime, true);
    
    // 빈 줄 건너뛰기
    while (std::getline(stream, line)) {
        ++lineCounter;
        if (lineCounter % kProgressLineStep == 0) {
            reportProgress(stream, totalSize, progressCallback, lastProgress, lastProgressTime);
        }
        std::string trimmed = trim(line);
        if (!trimmed.empty()) {
            break;
        }
    }
    
    // 그리드 크기 파싱 (예: "60   60   60")
    auto tokens = split(line);
    if (tokens.size() < 3) {
        result.errorMessage = "Invalid grid size line: " + line;
        return false;
    }
    
    try {
        result.gridShape[0] = std::stoi(tokens[0]);  // NGX
        result.gridShape[1] = std::stoi(tokens[1]);  // NGY
        result.gridShape[2] = std::stoi(tokens[2]);  // NGZ
    } catch (...) {
        result.errorMessage = "Failed to parse grid size: " + line;
        return false;
    }
    
    size_t totalPoints = static_cast<size_t>(result.gridShape[0]) * 
                         result.gridShape[1] * result.gridShape[2];
    SPDLOG_DEBUG("Grid size: {}x{}x{} = {} points", 
                 result.gridShape[0], result.gridShape[1], result.gridShape[2], totalPoints);
    
    // 밀도 데이터 읽기
    result.density.reserve(totalPoints);
    
    while (std::getline(stream, line) && result.density.size() < totalPoints) {
        ++lineCounter;
        if (lineCounter % kProgressLineStep == 0) {
            reportProgress(stream, totalSize, progressCallback, lastProgress, lastProgressTime);
        }

        // augmentation 데이터 체크
        if (line.find("augmentation") != std::string::npos) {
            SPDLOG_DEBUG("Found augmentation data, stopping density parsing");
            break;
        }
        
        tokens = split(line);
        for (const auto& t : tokens) {
            if (result.density.size() >= totalPoints) break;
            
            try {
                float value = parseVaspFloat(t);
                result.density.push_back(value);
            } catch (const std::exception& e) {
                SPDLOG_WARN("Failed to parse density value '{}': {}", t, e.what());
            }
        }
    }
    
    // 데이터 크기 검증
    if (result.density.size() != totalPoints) {
        SPDLOG_WARN("Expected {} density values, got {}. Padding with zeros.", 
                    totalPoints, result.density.size());
        result.density.resize(totalPoints, 0.0f);
    }
    
    // 통계 계산
    auto [minIt, maxIt] = std::minmax_element(result.density.begin(), result.density.end());
    result.minValue = *minIt;
    result.maxValue = *maxIt;
    
    SPDLOG_INFO("Density data: min={:.4e}, max={:.4e}", result.minValue, result.maxValue);
    reportProgress(stream, totalSize, progressCallback, lastProgress, lastProgressTime, true);
    
    return true;
}

} // namespace infrastructure
} // namespace atoms
