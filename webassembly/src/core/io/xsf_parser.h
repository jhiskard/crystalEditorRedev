#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace core::io {

using XsfProgressCallback = std::function<void(float)>;

struct XsfAtom {
    std::string symbol;
    std::array<double, 3> position = {0.0, 0.0, 0.0};
};

struct XsfParseResult {
    bool success = false;
    std::string errorMessage;
    std::string structureName;
    std::array<std::array<double, 3>, 3> latticeVectors = {{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}};
    std::vector<XsfAtom> atoms;
};

struct XsfGridData {
    std::string label;
    std::array<int, 3> dims = {0, 0, 0};
    std::array<double, 3> origin = {0.0, 0.0, 0.0};
    std::array<std::array<double, 3>, 3> vectors = {{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}};
    std::vector<float> values;
};

struct XsfGridParseResult {
    bool success = false;
    std::string errorMessage;
    std::string structureName;
    std::array<std::array<double, 3>, 3> latticeVectors = {{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}};
    bool hasCellVectors = false;
    bool cellVectorsConsistent = true;
    std::vector<XsfAtom> atoms;
    std::vector<XsfGridData> grids;
};

bool ContainsDatagrid3D(const std::string& filePath);
XsfParseResult ParseXSFFile(const std::string& filePath, XsfProgressCallback progressCallback = nullptr);
XsfGridParseResult ParseXSFGridFile(const std::string& filePath, XsfProgressCallback progressCallback = nullptr);

} // namespace core::io
