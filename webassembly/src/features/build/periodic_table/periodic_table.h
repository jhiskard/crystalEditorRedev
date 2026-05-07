#pragma once

#include <array>
#include <string>

namespace features::build::periodic_table {

std::array<float, 3> ComputeDefaultPosition(const std::string& symbol);

std::array<float, 3> FractionalToCartesian(
    const std::array<float, 3>& fractional,
    const std::array<std::array<float, 3>, 3>& matrix);

std::array<float, 3> CartesianToFractional(
    const std::array<float, 3>& cartesian,
    const std::array<std::array<float, 3>, 3>& matrix);

bool Invert3x3(
    const std::array<std::array<float, 3>, 3>& matrix,
    std::array<std::array<float, 3>, 3>& inverse);

} // namespace features::build::periodic_table
