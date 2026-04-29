/**
 * @file features/utilities/brillouin_zone/bz_plot.cpp
 * @brief Brillouin Zone calculation and bandpath helpers.
 */
#include "bz_plot.h"
#include "special_points.h"

#include <spdlog/spdlog.h>
#include <voro++.hh>

#include <algorithm>
#include <cctype>
#include <cmath>

namespace features::utilities::bz {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

std::vector<std::array<double, 4>> BZCalculator::generateLatticePoints(const double icell[3][3]) {
    std::vector<std::array<double, 4>> points;
    points.reserve(27);

    int id = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            for (int k = -1; k <= 1; ++k) {
                const double x = i * icell[0][0] + j * icell[1][0] + k * icell[2][0];
                const double y = i * icell[0][1] + j * icell[1][1] + k * icell[2][1];
                const double z = i * icell[0][2] + j * icell[1][2] + k * icell[2][2];

                points.push_back({x, y, z, static_cast<double>(id)});
                ++id;
            }
        }
    }

    SPDLOG_DEBUG("Generated {} lattice points", points.size());
    return points;
}

std::array<double, 6> BZCalculator::calculateBounds(
    const std::vector<std::array<double, 4>>& points,
    double margin) {
    if (points.empty()) {
        return {-2.0, 2.0, -2.0, 2.0, -2.0, 2.0};
    }

    double xmin = points[0][0], xmax = points[0][0];
    double ymin = points[0][1], ymax = points[0][1];
    double zmin = points[0][2], zmax = points[0][2];

    for (const auto& p : points) {
        xmin = std::min(xmin, p[0]);
        xmax = std::max(xmax, p[0]);
        ymin = std::min(ymin, p[1]);
        ymax = std::max(ymax, p[1]);
        zmin = std::min(zmin, p[2]);
        zmax = std::max(zmax, p[2]);
    }

    xmin -= margin;
    xmax += margin;
    ymin -= margin;
    ymax += margin;
    zmin -= margin;
    zmax += margin;

    return {xmin, xmax, ymin, ymax, zmin, zmax};
}

std::array<double, 3> BZCalculator::calculateNormal(
    const std::array<double, 3>& point1,
    const std::array<double, 3>& point2) {
    const double nx = point1[0] + point2[0];
    const double ny = point1[1] + point2[1];
    const double nz = point1[2] + point2[2];

    const double norm = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (norm < 1e-10) {
        return {0.0, 0.0, 1.0};
    }

    return {nx / norm, ny / norm, nz / norm};
}

BZVerticesResult BZCalculator::calculateBZVertices(const double icell[3][3]) {
    BZVerticesResult result;

    try {
        const auto points = generateLatticePoints(icell);
        if (points.size() != 27) {
            result.errorMessage = "Failed to generate lattice points.";
            return result;
        }

        constexpr int centralId = 13;
        const auto bounds = calculateBounds(points);

        voro::container con(
            bounds[0], bounds[1],
            bounds[2], bounds[3],
            bounds[4], bounds[5],
            6, 6, 6,
            false, false, false,
            8);

        for (const auto& p : points) {
            const int id = static_cast<int>(p[3]);
            con.put(id, p[0], p[1], p[2]);
        }

        voro::voronoicell_neighbor cell;
        voro::c_loop_all loop(con);
        bool foundCell = false;

        if (loop.start()) {
            do {
                if (loop.pid() == centralId) {
                    foundCell = con.compute_cell(cell, loop);
                    break;
                }
            } while (loop.inc());
        }

        if (!foundCell) {
            result.errorMessage = "Failed to compute Voronoi cell.";
            return result;
        }

        std::vector<double> allVertices;
        std::vector<int> faceVertexIndices;
        std::vector<int> neighborIds;

        cell.vertices(allVertices);
        cell.face_vertices(faceVertexIndices);
        cell.neighbors(neighborIds);

        const int numVertices = static_cast<int>(allVertices.size() / 3);

        int offset = 0;
        for (size_t faceIdx = 0; faceIdx < neighborIds.size(); ++faceIdx) {
            const int numFaceVertices = faceVertexIndices[offset];
            const int neighborId = neighborIds[faceIdx];

            BZFacet facet;
            facet.neighborId = neighborId;

            for (int i = 1; i <= numFaceVertices; ++i) {
                const int vIdx = faceVertexIndices[offset + i];
                if (vIdx < 0 || vIdx >= numVertices) {
                    continue;
                }

                facet.vertices.push_back({
                    allVertices[3 * vIdx + 0],
                    allVertices[3 * vIdx + 1],
                    allVertices[3 * vIdx + 2]});
            }

            if (neighborId >= 0 && neighborId < static_cast<int>(points.size())) {
                const std::array<double, 3> neighborPoint = {
                    points[neighborId][0],
                    points[neighborId][1],
                    points[neighborId][2]};
                facet.normal = calculateNormal({0.0, 0.0, 0.0}, neighborPoint);
            }

            result.facets.push_back(std::move(facet));
            offset += numFaceVertices + 1;
        }

        result.success = true;
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string("Exception during BZ calculation: ") + e.what();
    }

    return result;
}

std::map<std::string, std::array<double, 3>> BandpathCalculator::getCubicSpecialPoints(const double icell[3][3]) {
    std::map<std::string, std::array<double, 3>> points;

    const auto frac = SpecialPointsDatabase::getCubicPoints();
    for (const auto& kv : frac) {
        points[kv.first] = SpecialPointsDatabase::fractionalToCartesian(kv.second, icell);
    }

    return points;
}

std::vector<std::string> BandpathCalculator::parsePath(const std::string& path) {
    std::vector<std::string> labels;
    labels.reserve(path.size());

    std::string token;
    token.reserve(8);

    for (const char ch : path) {
        if (ch == '-' || ch == ',' || std::isspace(static_cast<unsigned char>(ch))) {
            if (!token.empty()) {
                labels.push_back(token);
                token.clear();
            }
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(ch))) {
            if (!token.empty()) {
                labels.push_back(token);
            }
            token.clear();
            token.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        }
    }

    if (!token.empty()) {
        labels.push_back(token);
    }

    return labels;
}

std::vector<std::array<double, 3>> BandpathCalculator::interpolateSegment(
    const std::array<double, 3>& start,
    const std::array<double, 3>& end,
    int npoints) {
    if (npoints < 2) {
        npoints = 2;
    }

    std::vector<std::array<double, 3>> out;
    out.reserve(static_cast<size_t>(npoints));

    for (int i = 0; i < npoints; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(npoints - 1);
        out.push_back({
            start[0] + (end[0] - start[0]) * t,
            start[1] + (end[1] - start[1]) * t,
            start[2] + (end[2] - start[2]) * t});
    }

    return out;
}

std::vector<std::array<double, 3>> BandpathCalculator::generateKpoints(
    const std::string& path,
    const double icell[3][3],
    int npointsPerSegment) {
    std::vector<std::array<double, 3>> result;

    if (npointsPerSegment < 2) {
        npointsPerSegment = 2;
    }

    const std::vector<std::string> labels = parsePath(path);
    if (labels.size() < 2) {
        return result;
    }

    const auto specialPoints = getCubicSpecialPoints(icell);
    bool firstSegment = true;

    for (size_t i = 0; i + 1 < labels.size(); ++i) {
        const auto it0 = specialPoints.find(labels[i]);
        const auto it1 = specialPoints.find(labels[i + 1]);
        if (it0 == specialPoints.end() || it1 == specialPoints.end()) {
            continue;
        }

        auto segment = interpolateSegment(it0->second, it1->second, npointsPerSegment);
        if (!firstSegment && !segment.empty()) {
            segment.erase(segment.begin());
        }

        result.insert(result.end(), segment.begin(), segment.end());
        firstSegment = false;
    }

    return result;
}

namespace BZTestUtils {

void createCubicReciprocalLattice(double icell[3][3], double a) {
    const double factor = 2.0 * kPi / a;

    icell[0][0] = factor;
    icell[0][1] = 0.0;
    icell[0][2] = 0.0;
    icell[1][0] = 0.0;
    icell[1][1] = factor;
    icell[1][2] = 0.0;
    icell[2][0] = 0.0;
    icell[2][1] = 0.0;
    icell[2][2] = factor;
}

void printBZResult(const BZVerticesResult& result) {
    if (!result.success) {
        SPDLOG_ERROR("BZ calculation failed: {}", result.errorMessage);
        return;
    }

    SPDLOG_INFO("BZ facets: {}", result.facets.size());
}

bool compareToPythonResult(
    const BZVerticesResult& cppResult,
    const std::vector<std::array<double, 3>>& /*pythonVertices*/,
    const std::vector<std::vector<int>>& pythonRidgeVertices) {
    if (!cppResult.success) {
        return false;
    }

    return cppResult.facets.size() == pythonRidgeVertices.size();
}

} // namespace BZTestUtils

} // namespace features::utilities::bz
