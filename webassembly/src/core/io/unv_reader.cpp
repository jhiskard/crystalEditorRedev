#include "unv_reader.h"

#include <fstream>
#include <string>

namespace core::io {

UnvParseResult ParseUnvFile(const std::string& filePath) {
    UnvParseResult result;

    std::ifstream fin(filePath);
    if (!fin.is_open()) {
        result.errorMessage = "Failed to open UNV file.";
        return result;
    }

    std::string line;
    while (std::getline(fin, line)) {
        if (line.find("2411") != std::string::npos) {
            ++result.nodeCount;
        }
        if (line.find("2412") != std::string::npos) {
            ++result.faceCount;
        }
        if (line.find("2467") != std::string::npos) {
            ++result.edgeCount;
        }
    }

    result.success = true;
    return result;
}

} // namespace core::io
