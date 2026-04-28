// webassembly/src/atoms/ui/ui_color_utils.h
#pragma once

#include <imgui.h>
#include <algorithm>

// Color4f 정의 위치에 맞춰 include 경로 조정.
// 현재 프로젝트 구조상 UI는 webassembly/src/atoms/ui,
// domain은 webassembly/src/atoms/domain 이므로 아래 경로가 자연스럽습니다.
#include "../domain/color.h"

namespace atoms::ui {

/// Clamp helper (float 0..1)
inline float Clamp01(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

/// Convert domain color to ImGui color (ImVec4)
inline ImVec4 ToImVec4(const atoms::domain::Color4f& c) {
    return ImVec4(Clamp01(c.r), Clamp01(c.g), Clamp01(c.b), Clamp01(c.a));
}

/// Compute a readable text color (black/white) based on background luminance.
inline ImVec4 GetContrastTextColor(const ImVec4& bgColor) {
    float luminance = 0.299f * bgColor.x + 0.587f * bgColor.y + 0.114f * bgColor.z;
    return (luminance > 0.5f)
        ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f)
        : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

} // namespace atoms::ui
