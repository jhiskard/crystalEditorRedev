#include "rho_parser.h"

#include <fstream>
#include <sstream>

namespace core::io {

RhoParseResult ParseRhoFile(const std::string& filePath) {
    RhoParseResult result;

    std::ifstream fin(filePath);
    if (!fin.is_open()) {
        result.errorMessage = "Failed to open RHO file.";
        return result;
    }

    std::string line;
    if (!std::getline(fin, line)) {
        result.errorMessage = "RHO file is empty.";
        return result;
    }

    std::istringstream dimIss(line);
    if (!(dimIss >> result.dims[0] >> result.dims[1] >> result.dims[2])) {
        result.errorMessage = "RHO header must start with 3 grid dimensions.";
        return result;
    }

    float value = 0.0f;
    while (fin >> value) {
        result.values.push_back(value);
    }

    if (result.values.empty()) {
        result.errorMessage = "RHO file has no scalar values.";
        return result;
    }

    result.success = true;
    return result;
}

} // namespace core::io
