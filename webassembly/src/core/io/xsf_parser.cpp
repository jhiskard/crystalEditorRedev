#include "xsf_parser.h"

#include <fstream>
#include <sstream>
#include <string>

namespace core::io {

XsfParseResult ParseXSFFile(const std::string& filePath) {
    XsfParseResult result;

    std::ifstream fin(filePath);
    if (!fin.is_open()) {
        result.errorMessage = "Failed to open XSF file.";
        return result;
    }

    std::string line;
    bool inPrimvec = false;
    bool inPrimcoord = false;
    int latticeRow = 0;
    int remainingAtomRows = 0;

    while (std::getline(fin, line)) {
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
                std::istringstream atomIss(line);
                if (!(atomIss >> atom.symbol
                              >> atom.position[0]
                              >> atom.position[1]
                              >> atom.position[2])) {
                    result.errorMessage = "Invalid atom row in XSF file.";
                    return result;
                }
                result.atoms.push_back(atom);
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
    return result;
}

} // namespace core::io
