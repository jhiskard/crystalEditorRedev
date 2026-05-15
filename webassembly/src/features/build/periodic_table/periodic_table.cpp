#include "periodic_table.h"

#include <cmath>

namespace features::build::periodic_table {

std::array<float, 3> ComputeDefaultPosition(const std::string& /*symbol*/) {
    // Legacy default behavior starts from origin unless the user edits position.
    return {0.0f, 0.0f, 0.0f};
}

bool Invert3x3(
    const std::array<std::array<float, 3>, 3>& matrix,
    std::array<std::array<float, 3>, 3>& inverse) {
    const float a = matrix[0][0];
    const float b = matrix[0][1];
    const float c = matrix[0][2];
    const float d = matrix[1][0];
    const float e = matrix[1][1];
    const float f = matrix[1][2];
    const float g = matrix[2][0];
    const float h = matrix[2][1];
    const float i = matrix[2][2];

    const float A = (e * i) - (f * h);
    const float B = -((d * i) - (f * g));
    const float C = (d * h) - (e * g);
    const float D = -((b * i) - (c * h));
    const float E = (a * i) - (c * g);
    const float F = -((a * h) - (b * g));
    const float G = (b * f) - (c * e);
    const float H = -((a * f) - (c * d));
    const float I = (a * e) - (b * d);

    const float det = (a * A) + (b * B) + (c * C);
    if (std::fabs(det) < 1e-10f) {
        return false;
    }

    const float invDet = 1.0f / det;
    inverse = {{
        {{A * invDet, D * invDet, G * invDet}},
        {{B * invDet, E * invDet, H * invDet}},
        {{C * invDet, F * invDet, I * invDet}},
    }};
    return true;
}

std::array<float, 3> FractionalToCartesian(
    const std::array<float, 3>& fractional,
    const std::array<std::array<float, 3>, 3>& matrix) {
    std::array<float, 3> cartesian = {0.0f, 0.0f, 0.0f};

    cartesian[0] = fractional[0] * matrix[0][0] +
                   fractional[1] * matrix[1][0] +
                   fractional[2] * matrix[2][0];
    cartesian[1] = fractional[0] * matrix[0][1] +
                   fractional[1] * matrix[1][1] +
                   fractional[2] * matrix[2][1];
    cartesian[2] = fractional[0] * matrix[0][2] +
                   fractional[1] * matrix[1][2] +
                   fractional[2] * matrix[2][2];

    return cartesian;
}

std::array<float, 3> CartesianToFractional(
    const std::array<float, 3>& cartesian,
    const std::array<std::array<float, 3>, 3>& matrix) {
    std::array<std::array<float, 3>, 3> inverse = {{
        {{1.0f, 0.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, 1.0f}},
    }};
    if (!Invert3x3(matrix, inverse)) {
        return {0.0f, 0.0f, 0.0f};
    }

    // Cell vectors are stored as matrix rows, so use cartesian[j] * inverse[j][i].
    std::array<float, 3> fractional = {0.0f, 0.0f, 0.0f};
    fractional[0] = (cartesian[0] * inverse[0][0]) +
                    (cartesian[1] * inverse[1][0]) +
                    (cartesian[2] * inverse[2][0]);
    fractional[1] = (cartesian[0] * inverse[0][1]) +
                    (cartesian[1] * inverse[1][1]) +
                    (cartesian[2] * inverse[2][1]);
    fractional[2] = (cartesian[0] * inverse[0][2]) +
                    (cartesian[1] * inverse[1][2]) +
                    (cartesian[2] * inverse[2][2]);

    return fractional;
}

} // namespace features::build::periodic_table
