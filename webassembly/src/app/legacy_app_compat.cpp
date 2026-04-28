/**
 * @file app/legacy_app_compat.cpp
 * @brief Temporary compatibility shim for legacy-identical font manager usage.
 */
#include "../legacy/app.h"
#include "app.h"

double App::DevicePixelRatio() {
    return static_cast<double>(app::App::DevicePixelRatio());
}
