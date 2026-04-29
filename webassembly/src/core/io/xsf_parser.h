#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace core::io {

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

XsfParseResult ParseXSFFile(const std::string& filePath);

} // namespace core::io
