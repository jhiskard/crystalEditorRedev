#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

namespace features::utilities::bz {

struct CellInfo {
    std::array<std::array<double, 3>, 3> matrix{};
    std::array<std::array<double, 3>, 3> invmatrix{};
    bool valid = false;
};

struct BZFacet {
    std::vector<std::array<double, 3>> vertices;
    std::array<double, 3> normal = {0.0, 0.0, 0.0};
    int neighborId = -1;
};

struct BZVerticesResult {
    std::vector<BZFacet> facets;
    bool success = false;
    std::string errorMessage;
};

class BZCalculator {
public:
    static BZVerticesResult calculateBZVertices(const double icell[3][3]);

    static std::vector<std::array<double, 4>> generateLatticePoints(const double icell[3][3]);

    static std::array<double, 3> calculateNormal(
        const std::array<double, 3>& point1,
        const std::array<double, 3>& point2);

    static std::array<double, 6> calculateBounds(
        const std::vector<std::array<double, 4>>& points,
        double margin = 0.5);
};

class BandpathCalculator {
public:
    static std::map<std::string, std::array<double, 3>> getCubicSpecialPoints(const double icell[3][3]);

    static std::vector<std::string> parsePath(const std::string& path);

    static std::vector<std::array<double, 3>> interpolateSegment(
        const std::array<double, 3>& start,
        const std::array<double, 3>& end,
        int npoints);

    static std::vector<std::array<double, 3>> generateKpoints(
        const std::string& path,
        const double icell[3][3],
        int npointsPerSegment = 50);
};

namespace BZTestUtils {

void createCubicReciprocalLattice(double icell[3][3], double a = 1.0);
void printBZResult(const BZVerticesResult& result);

bool compareToPythonResult(
    const BZVerticesResult& cppResult,
    const std::vector<std::array<double, 3>>& pythonVertices,
    const std::vector<std::vector<int>>& pythonRidgeVertices);

} // namespace BZTestUtils

} // namespace features::utilities::bz
