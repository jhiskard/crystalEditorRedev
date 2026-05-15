#pragma once

namespace core::vtk {

enum class ProjectionMode {
    Perspective,
    Parallel,
};

enum class CameraDirection {
    NotAligned,
    XPlus,
    XMinus,
    YPlus,
    YMinus,
    ZPlus,
    ZMinus,
};

} // namespace core::vtk
