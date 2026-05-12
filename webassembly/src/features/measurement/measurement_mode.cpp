#include "measurement_mode.h"

namespace features::measurement {

const char* ModeLabel(MeasurementMode mode) {
    switch (mode) {
    case MeasurementMode::Distance:
        return "Distance";
    case MeasurementMode::Angle:
        return "Angle";
    case MeasurementMode::Dihedral:
        return "Dihedral";
    case MeasurementMode::GeometricCenter:
        return "Geometric Center";
    case MeasurementMode::CenterOfMass:
        return "Center of Mass";
    case MeasurementMode::None:
    default:
        return "None";
    }
}

const char* TypeLabel(MeasurementType type) {
    switch (type) {
    case MeasurementType::Distance:
        return "Distance";
    case MeasurementType::Angle:
        return "Angle";
    case MeasurementType::Dihedral:
        return "Dihedral";
    case MeasurementType::GeometricCenter:
        return "Geometric Center";
    case MeasurementType::CenterOfMass:
        return "Center of Mass";
    default:
        return "Measurement";
    }
}

bool IsCenterMode(MeasurementMode mode) {
    return mode == MeasurementMode::GeometricCenter || mode == MeasurementMode::CenterOfMass;
}

size_t TargetPickCount(MeasurementMode mode) {
    switch (mode) {
    case MeasurementMode::Distance:
        return 2;
    case MeasurementMode::Angle:
        return 3;
    case MeasurementMode::Dihedral:
        return 4;
    case MeasurementMode::GeometricCenter:
    case MeasurementMode::CenterOfMass:
    case MeasurementMode::None:
    default:
        return 0;
    }
}

MeasurementType TypeFromMode(MeasurementMode mode) {
    switch (mode) {
    case MeasurementMode::Distance:
        return MeasurementType::Distance;
    case MeasurementMode::Angle:
        return MeasurementType::Angle;
    case MeasurementMode::Dihedral:
        return MeasurementType::Dihedral;
    case MeasurementMode::GeometricCenter:
        return MeasurementType::GeometricCenter;
    case MeasurementMode::CenterOfMass:
        return MeasurementType::CenterOfMass;
    case MeasurementMode::None:
    default:
        return MeasurementType::Distance;
    }
}

} // namespace features::measurement
