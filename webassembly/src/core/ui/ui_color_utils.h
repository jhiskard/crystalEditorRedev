#pragma once

#include "../data/color.h"

#include <algorithm>

#include <imgui.h>

namespace core::ui {

inline float Clamp01(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

inline ImVec4 ToImVec4(const core::data::Color4f& color) {
    return ImVec4(Clamp01(color.r), Clamp01(color.g), Clamp01(color.b), Clamp01(color.a));
}

inline ImVec4 GetContrastTextColor(const ImVec4& backgroundColor) {
    const float luminance = 0.299f * backgroundColor.x +
                            0.587f * backgroundColor.y +
                            0.114f * backgroundColor.z;
    if (luminance > 0.5f) {
        return ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

} // namespace core::ui
