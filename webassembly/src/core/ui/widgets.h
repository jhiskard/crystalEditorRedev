#pragma once

#include <imgui.h>

namespace core::ui {

class Widgets {
public:
    static bool IconButton(const char* buttonId,
                           const char* label,
                           ImVec2 buttonSize = ImVec2(72.0f, 26.0f));
};

} // namespace core::ui
