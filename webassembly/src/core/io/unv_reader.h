#pragma once

#include <cstdint>
#include <string>

namespace core::io {

struct UnvParseResult {
    bool success = false;
    std::string errorMessage;
    int32_t nodeCount = 0;
    int32_t edgeCount = 0;
    int32_t faceCount = 0;
    int32_t volumeCount = 0;
};

UnvParseResult ParseUnvFile(const std::string& filePath);

} // namespace core::io
