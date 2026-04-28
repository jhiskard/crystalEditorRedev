#pragma once

#include "app.h"

// ImGui
#include <imgui.h>


class CustomUI {
public:
    static bool IconButton(const char* btnId, const char* label,
        ImVec2 btnSize = ImVec2(3.0f * App::TextBaseWidth(), 1.2f * App::TextBaseHeight()));
};