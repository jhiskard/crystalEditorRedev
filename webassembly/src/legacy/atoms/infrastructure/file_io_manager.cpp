// atoms/infrastructure/file_io_manager.cpp (신규, merge-style progress parser)
#include "file_io_manager.h"
#include "../atoms_template.h"
#include "../../config/log_config.h"
#include "../domain/element_database.h"
#include "../domain/atom_manager.h"
#include "../domain/cell_manager.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <unordered_set>
#include <cmath>

namespace atoms {
namespace infrastructure {

namespace {
constexpr int kMinProgressIntervalMs = 1000;
constexpr uint64_t kLineStep = 200;
constexpr uint64_t kTokenStep = 4096;
constexpr uint64_t kValueStep = 16384;

bool isKeywordToken(const std::string& token) {
    if (token == "CRYSTAL" ||
        token == "PRIMVEC" ||
        token == "PRIMCOORD" ||
        token == "ATOMS" ||
        token == "BEGIN_BLOCK_DATAGRID_3D" ||
        token == "END_BLOCK_DATAGRID_3D" ||
        token.rfind("BEGIN_DATAGRID_3D", 0) == 0 ||
        token.rfind("END_DATAGRID_3D", 0) == 0) {
        return true;
    }
    return token.rfind("DATAGRID_3D", 0) == 0;
}

bool isNumberToken(const std::string& token) {
    if (token.empty()) {
        return false;
    }
    size_t idx = 0;
    if (token[0] == '-' || token[0] == '+') {
        idx = 1;
    }
    bool hasDigit = false;
    for (; idx < token.size(); ++idx) {
        if (std::isdigit(static_cast<unsigned char>(token[idx]))) {
            hasDigit = true;
            continue;
        }
        return false;
    }
    return hasDigit;
}

std::string toUpperCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}

std::string extractGridLabelFromToken(const std::string& token, const std::string& prefix) {
    if (token.size() <= prefix.size()) {
        return std::string();
    }
    std::string label = token.substr(prefix.size());
    if (!label.empty() && (label.front() == '_' || label.front() == ':' || label.front() == '-')) {
        label.erase(label.begin());
    }
    return label;
}

std::string shortGridLabel(const std::string& label) {
    std::string trimmed = label;
    while (!trimmed.empty() && (trimmed.front() == '_' || trimmed.front() == ':' || trimmed.front() == '-')) {
        trimmed.erase(trimmed.begin());
    }
    if (trimmed.empty()) {
        return trimmed;
    }
    auto pos = trimmed.rfind(':');
    if (pos != std::string::npos && pos + 1 < trimmed.size()) {
        return trimmed.substr(pos + 1);
    }
    return trimmed;
}

void parseLabelQuality(const std::string& label, std::string& baseLabel, int& qualityIndex) {
    baseLabel = label;
    qualityIndex = 2;
    std::string upper = toUpperCopy(label);

    auto stripSuffix = [&](const std::string& suffix, int quality) {
        if (upper.size() < suffix.size()) {
            return false;
        }
        if (upper.compare(upper.size() - suffix.size(), suffix.size(), suffix) != 0) {
            return false;
        }
        baseLabel = label.substr(0, label.size() - suffix.size());
        while (!baseLabel.empty() && (baseLabel.back() == '_' || baseLabel.back() == '-')) {
            baseLabel.pop_back();
        }
        qualityIndex = quality;
        return true;
    };

    if (stripSuffix("_LOW", 0) || stripSuffix("-LOW", 0) || stripSuffix("LOW", 0)) {
        return;
    }
    if (stripSuffix("_MEDIUM", 1) || stripSuffix("_MED", 1) || stripSuffix("-MEDIUM", 1) ||
        stripSuffix("-MED", 1) || stripSuffix("MEDIUM", 1) || stripSuffix("MED", 1)) {
        return;
    }
    if (stripSuffix("_HIGH", 2) || stripSuffix("-HIGH", 2) || stripSuffix("HIGH", 2)) {
        return;
    }
}

int calcDownsampleDim(int dim, int factor) {
    if (dim <= 1) {
        return 1;
    }
    return (dim - 1) / factor + 1;
}

bool buildDownsampledGrid(const FileIOManager::Grid3DResult& source,
    int factor,
    FileIOManager::Grid3DResult& output) {
    if (factor <= 1 || source.values.empty()) {
        return false;
    }

    const size_t sourceTotal = static_cast<size_t>(source.dims[0]) *
        static_cast<size_t>(source.dims[1]) *
        static_cast<size_t>(source.dims[2]);
    if (sourceTotal == 0 || source.values.size() != sourceTotal) {
        return false;
    }

    output = source;
    output.dims[0] = calcDownsampleDim(source.dims[0], factor);
    output.dims[1] = calcDownsampleDim(source.dims[1], factor);
    output.dims[2] = calcDownsampleDim(source.dims[2], factor);

    const bool hasReduction = output.dims[0] != source.dims[0] ||
        output.dims[1] != source.dims[1] ||
        output.dims[2] != source.dims[2];
    if (!hasReduction) {
        return false;
    }

    const size_t downsampleTotal = static_cast<size_t>(output.dims[0]) *
        static_cast<size_t>(output.dims[1]) *
        static_cast<size_t>(output.dims[2]);
    output.values.clear();
    output.values.reserve(downsampleTotal);

    for (int k = 0; k < output.dims[2]; ++k) {
        const int srcK = std::min(k * factor, source.dims[2] - 1);
        for (int j = 0; j < output.dims[1]; ++j) {
            const int srcJ = std::min(j * factor, source.dims[1] - 1);
            for (int i = 0; i < output.dims[0]; ++i) {
                const int srcI = std::min(i * factor, source.dims[0] - 1);
                const size_t srcIndex = static_cast<size_t>(srcI) +
                    static_cast<size_t>(srcJ) * static_cast<size_t>(source.dims[0]) +
                    static_cast<size_t>(srcK) * static_cast<size_t>(source.dims[0]) * static_cast<size_t>(source.dims[1]);
                output.values.push_back(source.values[srcIndex]);
            }
        }
    }

    return output.values.size() == downsampleTotal;
}
} // namespace

// ============================================================================
// 정적 데이터
// ============================================================================

const char* FileIOManager::s_chemicalSymbols[] = {
    "X", "H", "He", "Li", "Be", "B", "C", "N", "O", "F", "Ne", 
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

const size_t FileIOManager::s_numElements = 
    sizeof(FileIOManager::s_chemicalSymbols) / sizeof(FileIOManager::s_chemicalSymbols[0]);

// ============================================================================
// 생성자/소멸자
// ============================================================================

FileIOManager::FileIOManager(::AtomsTemplate* parent) 
    : m_parent(parent) {
    SPDLOG_DEBUG("FileIOManager initialized");
}

FileIOManager::~FileIOManager() {
    SPDLOG_DEBUG("FileIOManager destroyed");
}

// ============================================================================
// Public 인터페이스
// ============================================================================

FileIOManager::ParseResult FileIOManager::loadXSFFile(const std::string& filePath) {
    SPDLOG_INFO("Loading XSF file: {}", filePath);
    
    // 파일 확장자 확인
    std::string ext = getFileExtension(filePath);
    if (ext != "xsf") {
        SPDLOG_WARN("File extension is not .xsf: {}", ext);
    }
    
    return parseXSFFile(filePath);
}

FileIOManager::Grid3DParseResult FileIOManager::load3DGridXSFFile(const std::string& filePath) {
    SPDLOG_INFO("Loading XSF grid file: {}", filePath);

    std::string ext = getFileExtension(filePath);
    if (ext != "xsf") {
        SPDLOG_WARN("File extension is not .xsf: {}", ext);
    }

    return parse3DGridXSFFile(filePath);
}

void FileIOManager::SetProgressCallback(std::function<void(float)> callback) {
    m_progressCallback = std::move(callback);
    m_lastProgress = -1.0f;
    m_lastProgressTime = std::chrono::steady_clock::time_point{};
    m_progressCounter = 0;
}

bool FileIOManager::saveXSFFile(const std::string& filePath, 
                                const float cellVectors[3][3],
                                const std::vector<AtomData>& atoms) {
    SPDLOG_WARN("XSF file saving not implemented yet");
    return false; // 미구현 - 추후 확장용
}

bool FileIOManager::initializeStructureFromXSF(const std::string& filePath, ::AtomsTemplate* parent, std::string& errorMessage, std::vector<uint32_t>* outNewAtomIds) {
    auto result = loadXSFFile(filePath);
    if (!result.success) {
        errorMessage = result.errorMessage;
        return false;
    }
    return initializeStructure(result.cellVectors, result.atoms, parent, errorMessage, outNewAtomIds);
}

bool FileIOManager::initializeStructure(const float cellVectors[3][3], const std::vector<AtomData>& atomsData, ::AtomsTemplate* parent, std::string& errorMessage, std::vector<uint32_t>* outNewAtomIds) {
    if (!parent) {
        errorMessage = "AtomsTemplate parent is null";
        return false;
    }
    try {
        // 셀 행렬 업데이트
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                cellInfo.matrix[i][j] = cellVectors[i][j];
            }
        }
        cellInfo.modified = true;
        atoms::domain::calculateInverseMatrix(cellInfo.matrix, cellInfo.invmatrix);

        parent->createUnitCell(cellInfo.matrix);

        // 원자 생성
        const auto& elementDB = atoms::domain::ElementDatabase::getInstance();
        for (const auto& atom : atomsData) {
            const std::string& symbol = atom.symbol;
            const float* position = atom.position;
            atoms::domain::Color4f color = elementDB.getDefaultColor(symbol);
            float radius = elementDB.getDefaultRadius(symbol);

            parent->createAtomSphere(symbol.c_str(), color, radius, position);
            if (outNewAtomIds && !atoms::domain::createdAtoms.empty()) {
                outNewAtomIds->push_back(atoms::domain::createdAtoms.back().id);
            }
        }

        atoms::domain::setCellVisible(true);
        atoms::domain::setSurroundingsVisible(false);

        SPDLOG_INFO("Initialized structure: {} atoms", atomsData.size());
        return true;
    } catch (const std::exception& e) {
        errorMessage = e.what();
        return false;
    }
}

std::string FileIOManager::getSymbolFromAtomicNumber(int atomicNumber) {
    if (atomicNumber >= 0 && atomicNumber < static_cast<int>(s_numElements)) {
        return s_chemicalSymbols[atomicNumber];
    }
    return "X"; // 기본값 (알 수 없는 원소)
}

std::string FileIOManager::getFileExtension(const std::string& filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }
    
    std::string ext = filePath.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), 
                  [](unsigned char c) { return std::tolower(c); });
    return ext;
}

// ============================================================================
// XSF 파싱 구현
// ============================================================================

FileIOManager::ParseResult FileIOManager::parseXSFFile(const std::string& filePath) {
    ParseResult result;
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        result.success = false;
        result.errorMessage = "Failed to open file: " + filePath;
        SPDLOG_ERROR("{}", result.errorMessage);
        return result;
    }

    std::streampos fileSize = 0;
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    m_lastProgress = -1.0f;
    m_lastProgressTime = std::chrono::steady_clock::time_point{};
    m_progressCounter = 0;

    std::string line;
    bool readingPrimvec = false;
    bool readingPrimcoord = false;
    int atomCount = 0;
    int currentAtomLine = 0;
    int primvecLine = 0;
    
    try {
        reportProgress(file, fileSize, true);
        while (std::getline(file, line)) {
            ++m_progressCounter;
            if (m_progressCounter % kLineStep == 0) {
                reportProgress(file, fileSize);
            }
            std::istringstream iss(line);
            std::string firstToken;
            iss >> firstToken;
            
            if (firstToken == "CRYSTAL") {
                SPDLOG_DEBUG("Found CRYSTAL keyword");
                continue;
            }
            
            if (firstToken == "PRIMVEC") {
                SPDLOG_DEBUG("Starting PRIMVEC section");
                readingPrimvec = true;
                primvecLine = 0;
                continue;
            }
            
            if (readingPrimvec) {
                if (primvecLine < 3) {
                    if (!parsePrimvecLine(line, primvecLine, result.cellVectors)) {
                        SPDLOG_WARN("Failed to parse PRIMVEC line {}", primvecLine);
                    }
                    primvecLine++;
                    
                    if (primvecLine >= 3) {
                        readingPrimvec = false;
                        SPDLOG_DEBUG("Completed PRIMVEC section");
                    }
                }
            }
            
            if (firstToken == "PRIMCOORD") {
                SPDLOG_DEBUG("Starting PRIMCOORD section");
                readingPrimcoord = true;
                currentAtomLine = 0;
                continue;
            }
            
            if (readingPrimcoord) {
                if (currentAtomLine == 0) {
                    // 첫 줄은 원자 개수
                    iss.str(line);
                    iss >> atomCount;
                    SPDLOG_DEBUG("Expecting {} atoms", atomCount);
                    currentAtomLine++;
                    continue;
                }
                
                if (currentAtomLine <= atomCount) {
                    if (parseAtomLine(line, result.atoms)) {
                        SPDLOG_DEBUG("Parsed atom {}/{}", currentAtomLine, atomCount);
                    } else {
                        SPDLOG_WARN("Failed to parse atom line {}", currentAtomLine);
                    }
                    
                    currentAtomLine++;
                    if (currentAtomLine > atomCount) {
                        readingPrimcoord = false;
                        SPDLOG_DEBUG("Completed PRIMCOORD section");
                    }
                }
            }
        }

        reportProgress(file, fileSize, true);
        file.close();
        if (m_progressCallback) {
            m_progressCallback(1.0f);
        }
        
        // 결과 검증
        if (primvecLine == 3 && !result.atoms.empty()) {
            result.success = true;
            SPDLOG_INFO("Successfully parsed XSF file: {} atoms, cell vectors complete", 
                       result.atoms.size());
        } else {
            result.success = false;
            result.errorMessage = "Invalid XSF file structure";
            SPDLOG_ERROR("{}: Cell vectors: {}, Atoms: {}", 
                        result.errorMessage, primvecLine, result.atoms.size());
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Exception during parsing: " + std::string(e.what());
        SPDLOG_ERROR("{}", result.errorMessage);
    }
    
    return result;
}

FileIOManager::Grid3DParseResult FileIOManager::parse3DGridXSFFile(const std::string& filePath) {
    Grid3DParseResult result;

    const std::string blockBegin = "BEGIN_BLOCK_DATAGRID_3D";
    const std::string blockEnd = "END_BLOCK_DATAGRID_3D";
    const std::string gridPrefix = "BEGIN_DATAGRID_3D";
    const float kCellEps = 1e-6f;

    auto ltrim = [](const std::string& input) {
        size_t start = input.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return std::string();
        }
        return input.substr(start);
    };

    auto startsWith = [](const std::string& value, const std::string& prefix) {
        return value.rfind(prefix, 0) == 0;
    };

    auto isKeywordLine = [&](const std::string& trimmed) {
        return startsWith(trimmed, "BEGIN_") ||
               startsWith(trimmed, "END_") ||
               startsWith(trimmed, "PRIMVEC") ||
               startsWith(trimmed, "PRIMCOORD") ||
               startsWith(trimmed, "ATOMS");
    };

    {
        std::ifstream atomsFile(filePath);
        if (atomsFile.is_open()) {
            std::string line;
            bool inAtoms = false;
            while (std::getline(atomsFile, line)) {
                std::string trimmed = ltrim(line);
                if (trimmed.empty()) {
                    continue;
                }
                if (!inAtoms) {
                    if (startsWith(trimmed, "ATOMS")) {
                        inAtoms = true;
                        continue;
                    }
                    if (startsWith(trimmed, blockBegin)) {
                        break;
                    }
                    continue;
                }
                if (isKeywordLine(trimmed)) {
                    break;
                }
                std::istringstream iss(trimmed);
                int atomicNumber = 0;
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
                if (!(iss >> atomicNumber >> x >> y >> z)) {
                    continue;
                }
                result.atoms.emplace_back(getSymbolFromAtomicNumber(atomicNumber), x, y, z);
            }
        }
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        result.success = false;
        result.errorMessage = "Failed to open file: " + filePath;
        SPDLOG_ERROR("{}", result.errorMessage);
        return result;
    }

    std::streampos fileSize = 0;
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    m_lastProgress = -1.0f;
    m_lastProgressTime = std::chrono::steady_clock::time_point{};
    m_progressCounter = 0;

    bool inBlock = false;
    std::string token;

    reportProgress(file, fileSize, true);

    auto calcDownsampleDim = [](int dim, int factor) {
        if (dim <= 1) {
            return 1;
        }
        return (dim - 1) / factor + 1;
    };

    while (file >> token) {
        ++m_progressCounter;
        if (m_progressCounter % kTokenStep == 0) {
            reportProgress(file, fileSize);
        }
        if (token == blockBegin) {
            inBlock = true;
            continue;
        }
        if (token == blockEnd) {
            inBlock = false;
            continue;
        }
        if (!inBlock) {
            continue;
        }

        if (token.rfind(gridPrefix, 0) != 0) {
            continue;
        }

        Grid3DSet gridSet;
        Grid3DResult& grid = gridSet.high;
        if (token.size() > gridPrefix.size()) {
            size_t start = gridPrefix.size();
            if (token[start] == '_' || token[start] == ':') {
                start += 1;
            }
            if (start < token.size()) {
                grid.label = token.substr(start);
            }
        }
        if (grid.label.empty()) {
            if (!(file >> grid.label)) {
                result.success = false;
                result.errorMessage = "Missing DATAGRID_3D label";
                SPDLOG_ERROR("{}", result.errorMessage);
                return result;
            }
        }

        if (!(file >> grid.dims[0] >> grid.dims[1] >> grid.dims[2])) {
            result.success = false;
            result.errorMessage = "Failed to read DATAGRID_3D dimensions";
            SPDLOG_ERROR("{}", result.errorMessage);
            return result;
        }
        if (grid.dims[0] <= 0 || grid.dims[1] <= 0 || grid.dims[2] <= 0) {
            result.success = false;
            result.errorMessage = "Invalid DATAGRID_3D dimensions";
            SPDLOG_ERROR("{}", result.errorMessage);
            return result;
        }

        gridSet.medium.label = grid.label;
        gridSet.low.label = grid.label;
        for (int axis = 0; axis < 3; ++axis) {
            gridSet.medium.dims[axis] = calcDownsampleDim(grid.dims[axis], 2);
            gridSet.low.dims[axis] = calcDownsampleDim(grid.dims[axis], 4);
        }
        gridSet.hasMedium = (gridSet.medium.dims[0] != grid.dims[0] ||
            gridSet.medium.dims[1] != grid.dims[1] ||
            gridSet.medium.dims[2] != grid.dims[2]);
        gridSet.hasLow = (gridSet.low.dims[0] != grid.dims[0] ||
            gridSet.low.dims[1] != grid.dims[1] ||
            gridSet.low.dims[2] != grid.dims[2]);

        if (!(file >> grid.origin[0] >> grid.origin[1] >> grid.origin[2])) {
            result.success = false;
            result.errorMessage = "Failed to read DATAGRID_3D origin";
            SPDLOG_ERROR("{}", result.errorMessage);
            return result;
        }
        gridSet.medium.origin[0] = grid.origin[0];
        gridSet.medium.origin[1] = grid.origin[1];
        gridSet.medium.origin[2] = grid.origin[2];
        gridSet.low.origin[0] = grid.origin[0];
        gridSet.low.origin[1] = grid.origin[1];
        gridSet.low.origin[2] = grid.origin[2];

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (!(file >> grid.vectors[i][j])) {
                    result.success = false;
                    result.errorMessage = "Failed to read DATAGRID_3D vectors";
                    SPDLOG_ERROR("{}", result.errorMessage);
                    return result;
                }
                gridSet.medium.vectors[i][j] = grid.vectors[i][j];
                gridSet.low.vectors[i][j] = grid.vectors[i][j];
            }
        }
        if (!result.hasCellVectors) {
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    result.cellVectors[i][j] = grid.vectors[i][j];
                }
            }
            result.hasCellVectors = true;
        } else if (result.cellVectorsConsistent) {
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    if (std::fabs(result.cellVectors[i][j] - grid.vectors[i][j]) > kCellEps) {
                        result.cellVectorsConsistent = false;
                        break;
                    }
                }
                if (!result.cellVectorsConsistent) {
                    break;
                }
            }
        }

        size_t totalValues = static_cast<size_t>(grid.dims[0]) *
                             static_cast<size_t>(grid.dims[1]) *
                             static_cast<size_t>(grid.dims[2]);
        grid.values.reserve(totalValues);
        size_t mediumTotal = static_cast<size_t>(gridSet.medium.dims[0]) *
                             static_cast<size_t>(gridSet.medium.dims[1]) *
                             static_cast<size_t>(gridSet.medium.dims[2]);
        size_t lowTotal = static_cast<size_t>(gridSet.low.dims[0]) *
                          static_cast<size_t>(gridSet.low.dims[1]) *
                          static_cast<size_t>(gridSet.low.dims[2]);
        if (gridSet.hasMedium) {
            gridSet.medium.values.reserve(mediumTotal);
        }
        if (gridSet.hasLow) {
            gridSet.low.values.reserve(lowTotal);
        }

        int iIndex = 0;
        int jIndex = 0;
        int kIndex = 0;
        for (size_t idx = 0; idx < totalValues; ++idx) {
            float value = 0.0f;
            if (!(file >> value)) {
                result.success = false;
                result.errorMessage = "Failed to read DATAGRID_3D values";
                SPDLOG_ERROR("{}", result.errorMessage);
                return result;
            }
            grid.values.push_back(value);
            if (gridSet.hasMedium && (iIndex % 2 == 0) && (jIndex % 2 == 0) && (kIndex % 2 == 0)) {
                gridSet.medium.values.push_back(value);
            }
            if (gridSet.hasLow && (iIndex % 4 == 0) && (jIndex % 4 == 0) && (kIndex % 4 == 0)) {
                gridSet.low.values.push_back(value);
            }
            ++iIndex;
            if (iIndex >= grid.dims[0]) {
                iIndex = 0;
                ++jIndex;
                if (jIndex >= grid.dims[1]) {
                    jIndex = 0;
                    ++kIndex;
                }
            }
            if (idx % kValueStep == 0) {
                reportProgress(file, fileSize);
            }
        }

        if (gridSet.hasMedium && gridSet.medium.values.size() != mediumTotal) {
            SPDLOG_WARN("Medium grid value count mismatch: expected {}, got {}",
                mediumTotal, gridSet.medium.values.size());
        }
        if (gridSet.hasLow && gridSet.low.values.size() != lowTotal) {
            SPDLOG_WARN("Low grid value count mismatch: expected {}, got {}",
                lowTotal, gridSet.low.values.size());
        }

        std::string endToken;
        if (!(file >> endToken)) {
            result.success = false;
            result.errorMessage = "Missing END_DATAGRID_3D token";
            SPDLOG_ERROR("{}", result.errorMessage);
            return result;
        }
        while (endToken != "END_DATAGRID_3D") {
            if (endToken == blockEnd) {
                result.success = false;
                result.errorMessage = "Unexpected END_BLOCK_DATAGRID_3D before END_DATAGRID_3D";
                SPDLOG_ERROR("{}", result.errorMessage);
                return result;
            }
            if (!(file >> endToken)) {
                result.success = false;
                result.errorMessage = "Missing END_DATAGRID_3D token";
                SPDLOG_ERROR("{}", result.errorMessage);
                return result;
            }
        }

        result.grids.push_back(std::move(gridSet));
    }

    reportProgress(file, fileSize, true);
    file.close();
    if (m_progressCallback) {
        m_progressCallback(1.0f);
    }

    if (result.grids.empty()) {
        result.success = false;
        result.errorMessage = "No DATAGRID_3D blocks found";
        SPDLOG_ERROR("{}", result.errorMessage);
        return result;
    }

    result.success = true;
    SPDLOG_INFO("Parsed DATAGRID_3D blocks: {}", result.grids.size());
    return result;
}

void FileIOManager::reportProgress(std::ifstream& file, std::streampos totalSize, bool force) {
    if (!m_progressCallback) {
        return;
    }
    if (totalSize <= 0) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    if (!force) {
        if (m_lastProgressTime.time_since_epoch().count() != 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastProgressTime);
            if (elapsed.count() < kMinProgressIntervalMs) {
                return;
            }
        }
    }

    std::streampos pos = file.tellg();
    if (pos == std::streampos(-1)) {
        if (!force) {
            return;
        }
        pos = totalSize;
    } else if (pos <= 0 && !force) {
        return;
    }

    float progress = static_cast<float>(pos) / static_cast<float>(totalSize);
    if (progress > 1.0f) {
        progress = 1.0f;
    }
    if (progress < 0.0f) {
        progress = 0.0f;
    }

    if (!force) {
        if (m_lastProgress >= 0.0f && progress - m_lastProgress < 0.01f && progress < 1.0f) {
            return;
        }
    }

    m_lastProgress = progress;
    m_lastProgressTime = now;
    m_progressCallback(progress);
}

bool FileIOManager::parsePrimvecLine(const std::string& line, int primvecLine, 
                                     float cellVectors[3][3]) {
    if (primvecLine < 0 || primvecLine >= 3) {
        return false;
    }
    
    std::istringstream iss(line);
    float v1, v2, v3;
    
    if (iss >> v1 >> v2 >> v3) {
        cellVectors[primvecLine][0] = v1;
        cellVectors[primvecLine][1] = v2;
        cellVectors[primvecLine][2] = v3;
        
        SPDLOG_DEBUG("PRIMVEC[{}]: ({:.6f}, {:.6f}, {:.6f})", 
                    primvecLine, v1, v2, v3);
        return true;
    }
    
    return false;
}

bool FileIOManager::parseAtomLine(const std::string& line, 
                                  std::vector<AtomData>& atoms) {
    std::istringstream iss(line);
    int atomicNum;
    float x, y, z;
    
    if (iss >> atomicNum >> x >> y >> z) {
        std::string symbol = getSymbolFromAtomicNumber(atomicNum);
        atoms.push_back(AtomData(symbol, x, y, z));
        
        SPDLOG_DEBUG("Parsed atom: {} ({}) at ({:.6f}, {:.6f}, {:.6f})", 
                    symbol, atomicNum, x, y, z);
        return true;
    }
    
    return false;
}

} // namespace infrastructure
} // namespace atoms
