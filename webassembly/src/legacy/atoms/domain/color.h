#pragma once
#include <cstdint>

namespace atoms::domain {

/**
 * @brief UI-independent RGBA color (float 0..1).
 * This replaces ImGui's ImVec4 in domain/infrastructure layers.
 */
struct Color4f {
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    float a = 1.f;

    constexpr Color4f() = default;
    constexpr Color4f(float r_, float g_, float b_, float a_ = 1.f) : r(r_), g(g_), b(b_), a(a_) {}
};

} // namespace atoms::domain
