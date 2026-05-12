/**
 * @file features/measurement/measurement_mode.h
 * @brief Measurement mode and type helpers.
 */
#pragma once

#include <cstddef>

namespace features::measurement {

enum class MeasurementMode {
    None = 0,
    Distance,
    Angle,
    Dihedral,
    GeometricCenter,
    CenterOfMass,
};

enum class MeasurementType {
    Distance = 0,
    Angle,
    Dihedral,
    GeometricCenter,
    CenterOfMass,
};

const char* ModeLabel(MeasurementMode mode);
const char* TypeLabel(MeasurementType type);
bool IsCenterMode(MeasurementMode mode);
size_t TargetPickCount(MeasurementMode mode);
MeasurementType TypeFromMode(MeasurementMode mode);

} // namespace features::measurement
