#pragma once

#include <array>
#include <string>
#include <vector>

namespace core::io {

struct RhoParseResult {
    bool success = false;
    std::string errorMessage;
    std::array<int, 3> dims = {0, 0, 0};
    std::vector<float> values;
};

RhoParseResult ParseRhoFile(const std::string& filePath);

} // namespace core::io
