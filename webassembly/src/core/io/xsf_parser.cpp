#include "xsf_parser.h"

#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace core::io {
namespace {

constexpr int kMinProgressIntervalMs = 1000;
constexpr uint64_t kLineStep = 200;
constexpr uint64_t kValueStep = 16384;

std::string trim(const std::string& value) {
    const size_t begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

bool startsWith(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

bool isKeywordLine(const std::string& trimmed) {
    return startsWith(trimmed, "BEGIN_") ||
           startsWith(trimmed, "END_") ||
           startsWith(trimmed, "PRIMVEC") ||
           startsWith(trimmed, "PRIMCOORD") ||
           startsWith(trimmed, "ATOMS") ||
           startsWith(trimmed, "CRYSTAL");
}

bool parseAtomLine(const std::string& line, XsfAtom& atom) {
    std::istringstream atomIss(line);
    return static_cast<bool>(atomIss >> atom.symbol
                                     >> atom.position[0]
                                     >> atom.position[1]
                                     >> atom.position[2]);
}

bool vectorsClose(
    const std::array<std::array<double, 3>, 3>& lhs,
    const std::array<std::array<double, 3>, 3>& rhs) {
    constexpr double kEpsilon = 1e-6;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (std::fabs(lhs[row][col] - rhs[row][col]) > kEpsilon) {
                return false;
            }
        }
    }
    return true;
}

std::string defaultGridLabel(size_t index) {
    std::string label = "noname_";
    if (index < 10) {
        label += "0";
    }
    label += std::to_string(index);
    return label;
}

void reportProgress(std::ifstream& file,
                    std::streampos totalSize,
                    const XsfProgressCallback& callback,
                    bool force,
                    float& lastProgress,
                    std::chrono::steady_clock::time_point& lastProgressTime) {
    if (!callback || totalSize <= 0) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (!force && lastProgressTime.time_since_epoch().count() != 0) {
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastProgressTime);
        if (elapsed.count() < kMinProgressIntervalMs) {
            return;
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

    if (!force && lastProgress >= 0.0f && progress - lastProgress < 0.01f && progress < 1.0f) {
        return;
    }

    lastProgress = progress;
    lastProgressTime = now;
    callback(progress);
}

} // namespace

bool ContainsDatagrid3D(const std::string& filePath) {
    std::ifstream fin(filePath);
    if (!fin.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(fin, line)) {
        if (line.find("DATAGRID_3D") != std::string::npos) {
            return true;
        }
    }
    return false;
}

XsfParseResult ParseXSFFile(const std::string& filePath, XsfProgressCallback progressCallback) {
    XsfParseResult result;

    std::ifstream fin(filePath);
    if (!fin.is_open()) {
        result.errorMessage = "Failed to open XSF file.";
        return result;
    }

    fin.seekg(0, std::ios::end);
    const std::streampos fileSize = fin.tellg();
    fin.seekg(0, std::ios::beg);
    float lastProgress = -1.0f;
    std::chrono::steady_clock::time_point lastProgressTime;
    uint64_t progressCounter = 0;

    std::string line;
    bool inPrimvec = false;
    bool inPrimcoord = false;
    int latticeRow = 0;
    int remainingAtomRows = 0;

    reportProgress(fin, fileSize, progressCallback, true, lastProgress, lastProgressTime);
    while (std::getline(fin, line)) {
        ++progressCounter;
        if (progressCounter % kLineStep == 0) {
            reportProgress(fin, fileSize, progressCallback, false, lastProgress, lastProgressTime);
        }

        if (line.empty()) {
            continue;
        }

        std::istringstream headerIss(line);
        std::string token;
        headerIss >> token;
        if (token.empty()) {
            continue;
        }

        if (token == "PRIMVEC") {
            inPrimvec = true;
            inPrimcoord = false;
            latticeRow = 0;
            continue;
        }

        if (token == "PRIMCOORD") {
            inPrimvec = false;
            inPrimcoord = true;
            remainingAtomRows = -1;
            continue;
        }

        if (inPrimvec && latticeRow < 3) {
            std::istringstream vecIss(line);
            if (!(vecIss >> result.latticeVectors[latticeRow][0]
                         >> result.latticeVectors[latticeRow][1]
                         >> result.latticeVectors[latticeRow][2])) {
                result.errorMessage = "Invalid PRIMVEC row in XSF file.";
                return result;
            }
            ++latticeRow;
            if (latticeRow == 3) {
                inPrimvec = false;
            }
            continue;
        }

        if (inPrimcoord) {
            if (remainingAtomRows < 0) {
                std::istringstream countIss(line);
                int atomCount = 0;
                int ignored = 0;
                if (!(countIss >> atomCount >> ignored) || atomCount < 0) {
                    result.errorMessage = "Invalid PRIMCOORD header in XSF file.";
                    return result;
                }
                remainingAtomRows = atomCount;
                continue;
            }

            if (remainingAtomRows > 0) {
                XsfAtom atom;
                if (!parseAtomLine(line, atom)) {
                    result.errorMessage = "Invalid atom row in XSF file.";
                    return result;
                }
                result.atoms.push_back(std::move(atom));
                --remainingAtomRows;
                if (remainingAtomRows == 0) {
                    inPrimcoord = false;
                }
            }
        }
    }

    if (result.atoms.empty()) {
        result.errorMessage = "No atoms were parsed from XSF file.";
        return result;
    }

    result.structureName = filePath;
    result.success = true;
    reportProgress(fin, fileSize, progressCallback, true, lastProgress, lastProgressTime);
    return result;
}

XsfGridParseResult ParseXSFGridFile(const std::string& filePath, XsfProgressCallback progressCallback) {
    XsfGridParseResult result;
    result.structureName = filePath;

    std::ifstream fin(filePath);
    if (!fin.is_open()) {
        result.errorMessage = "Failed to open XSF grid file.";
        return result;
    }

    fin.seekg(0, std::ios::end);
    const std::streampos fileSize = fin.tellg();
    fin.seekg(0, std::ios::beg);
    float lastProgress = -1.0f;
    std::chrono::steady_clock::time_point lastProgressTime;
    uint64_t progressCounter = 0;

    bool inPrimvec = false;
    bool inPrimcoord = false;
    bool inAtoms = false;
    int latticeRow = 0;
    int remainingAtomRows = 0;
    bool haveReferenceGridVectors = false;
    std::array<std::array<double, 3>, 3> referenceGridVectors = result.latticeVectors;

    std::string line;
    reportProgress(fin, fileSize, progressCallback, true, lastProgress, lastProgressTime);
    while (std::getline(fin, line)) {
        ++progressCounter;
        if (progressCounter % kLineStep == 0) {
            reportProgress(fin, fileSize, progressCallback, false, lastProgress, lastProgressTime);
        }

        const std::string trimmed = trim(line);
        if (trimmed.empty()) {
            continue;
        }

        if (startsWith(trimmed, "BEGIN_DATAGRID_3D")) {
            XsfGridData grid;
            std::istringstream labelIss(trimmed);
            std::string beginToken;
            labelIss >> beginToken >> grid.label;
            constexpr const char* kGridPrefix = "BEGIN_DATAGRID_3D";
            if (grid.label.empty() && startsWith(beginToken, kGridPrefix)) {
                size_t labelStart = std::string(kGridPrefix).size();
                if (labelStart < beginToken.size() &&
                    (beginToken[labelStart] == '_' || beginToken[labelStart] == ':')) {
                    ++labelStart;
                }
                if (labelStart < beginToken.size()) {
                    grid.label = beginToken.substr(labelStart);
                }
            }
            if (grid.label.empty()) {
                grid.label = defaultGridLabel(result.grids.size() + 1);
            }

            if (!std::getline(fin, line)) {
                result.errorMessage = "Missing DATAGRID_3D dimensions.";
                return result;
            }
            {
                std::istringstream dimsIss(line);
                if (!(dimsIss >> grid.dims[0] >> grid.dims[1] >> grid.dims[2]) ||
                    grid.dims[0] <= 0 || grid.dims[1] <= 0 || grid.dims[2] <= 0) {
                    result.errorMessage = "Invalid DATAGRID_3D dimensions.";
                    return result;
                }
            }

            if (!std::getline(fin, line)) {
                result.errorMessage = "Missing DATAGRID_3D origin.";
                return result;
            }
            {
                std::istringstream originIss(line);
                if (!(originIss >> grid.origin[0] >> grid.origin[1] >> grid.origin[2])) {
                    result.errorMessage = "Invalid DATAGRID_3D origin.";
                    return result;
                }
            }

            for (int row = 0; row < 3; ++row) {
                if (!std::getline(fin, line)) {
                    result.errorMessage = "Missing DATAGRID_3D vector.";
                    return result;
                }
                std::istringstream vecIss(line);
                if (!(vecIss >> grid.vectors[row][0] >> grid.vectors[row][1] >> grid.vectors[row][2])) {
                    result.errorMessage = "Invalid DATAGRID_3D vector.";
                    return result;
                }
            }

            const size_t valueCount = static_cast<size_t>(grid.dims[0]) *
                                      static_cast<size_t>(grid.dims[1]) *
                                      static_cast<size_t>(grid.dims[2]);
            grid.values.reserve(valueCount);
            while (grid.values.size() < valueCount && std::getline(fin, line)) {
                ++progressCounter;
                const std::string valueLine = trim(line);
                if (startsWith(valueLine, "END_DATAGRID_3D")) {
                    break;
                }
                std::istringstream valuesIss(valueLine);
                double value = 0.0;
                while (valuesIss >> value && grid.values.size() < valueCount) {
                    grid.values.push_back(static_cast<float>(value));
                    if (grid.values.size() % kValueStep == 0) {
                        reportProgress(fin, fileSize, progressCallback, false, lastProgress, lastProgressTime);
                    }
                }
                if (progressCounter % kLineStep == 0) {
                    reportProgress(fin, fileSize, progressCallback, false, lastProgress, lastProgressTime);
                }
            }

            if (grid.values.size() != valueCount) {
                result.errorMessage = "DATAGRID_3D value count does not match dimensions.";
                return result;
            }

            if (!haveReferenceGridVectors) {
                referenceGridVectors = grid.vectors;
                haveReferenceGridVectors = true;
            } else if (!vectorsClose(referenceGridVectors, grid.vectors)) {
                result.cellVectorsConsistent = false;
            }

            result.grids.push_back(std::move(grid));
            inAtoms = false;
            inPrimcoord = false;
            inPrimvec = false;
            continue;
        }

        std::istringstream headerIss(trimmed);
        std::string token;
        headerIss >> token;
        if (token.empty()) {
            continue;
        }

        if (token == "PRIMVEC") {
            inPrimvec = true;
            inPrimcoord = false;
            inAtoms = false;
            latticeRow = 0;
            continue;
        }

        if (inPrimvec && latticeRow < 3) {
            std::istringstream vecIss(trimmed);
            if (!(vecIss >> result.latticeVectors[latticeRow][0]
                         >> result.latticeVectors[latticeRow][1]
                         >> result.latticeVectors[latticeRow][2])) {
                result.errorMessage = "Invalid PRIMVEC row in XSF grid file.";
                return result;
            }
            ++latticeRow;
            if (latticeRow == 3) {
                inPrimvec = false;
                result.hasCellVectors = true;
            }
            continue;
        }

        if (token == "PRIMCOORD") {
            inPrimcoord = true;
            inAtoms = false;
            remainingAtomRows = -1;
            continue;
        }

        if (inPrimcoord) {
            if (remainingAtomRows < 0) {
                std::istringstream countIss(trimmed);
                int atomCount = 0;
                int ignored = 0;
                if (!(countIss >> atomCount >> ignored) || atomCount < 0) {
                    result.errorMessage = "Invalid PRIMCOORD header in XSF grid file.";
                    return result;
                }
                remainingAtomRows = atomCount;
                continue;
            }

            if (remainingAtomRows > 0) {
                XsfAtom atom;
                if (!parseAtomLine(trimmed, atom)) {
                    result.errorMessage = "Invalid atom row in XSF grid file.";
                    return result;
                }
                result.atoms.push_back(std::move(atom));
                --remainingAtomRows;
                if (remainingAtomRows == 0) {
                    inPrimcoord = false;
                }
                continue;
            }
        }

        if (token == "ATOMS") {
            inAtoms = true;
            inPrimcoord = false;
            inPrimvec = false;
            continue;
        }

        if (inAtoms) {
            if (isKeywordLine(trimmed)) {
                inAtoms = false;
            } else {
                XsfAtom atom;
                if (parseAtomLine(trimmed, atom)) {
                    result.atoms.push_back(std::move(atom));
                }
                continue;
            }
        }
    }

    if (result.grids.empty()) {
        result.errorMessage = "No DATAGRID_3D blocks were parsed from XSF file.";
        return result;
    }

    if (!result.hasCellVectors && haveReferenceGridVectors) {
        result.latticeVectors = referenceGridVectors;
        result.hasCellVectors = true;
    }

    result.success = true;
    reportProgress(fin, fileSize, progressCallback, true, lastProgress, lastProgressTime);
    return result;
}

} // namespace core::io
