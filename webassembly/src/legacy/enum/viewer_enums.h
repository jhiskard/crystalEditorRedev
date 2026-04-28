#pragma once


// User's point of view
// - First: Front
// - Second: Up
enum class CameraDirection {
    XPLUS_YPLUS,
    XPLUS_YMINUS,
    XPLUS_ZPLUS,
    XPLUS_ZMINUS,
    XMINUS_YPLUS,
    XMINUS_YMINUS,
    XMINUS_ZPLUS,
    XMINUS_ZMINUS,
    YPLUS_XPLUS,
    YPLUS_XMINUS,
    YPLUS_ZPLUS,
    YPLUS_ZMINUS,
    YMINUS_XPLUS,
    YMINUS_XMINUS,
    YMINUS_ZPLUS,
    YMINUS_ZMINUS,
    ZPLUS_XPLUS,
    ZPLUS_XMINUS,
    ZPLUS_YPLUS,
    ZPLUS_YMINUS,
    ZMINUS_XPLUS,
    ZMINUS_XMINUS,
    ZMINUS_YPLUS,
    ZMINUS_YMINUS,
    NOT_ALIGNED
};

enum class MeshDisplayMode {
    WIREFRAME = 0,
    SHADED = 1,
    WIRESHADED = 2
};

enum class ProjectionMode {
    PERSPECTIVE = 0,
    PARALLEL = 1
};