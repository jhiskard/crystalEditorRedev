#pragma once

#include "macro/singleton_macro.h"

// Standard library
#include <vector>


class TestWindow {
    DECLARE_SINGLETON(TestWindow)

public:
    void Render(bool* openWindow = nullptr);

private:
    std::vector<void*> m_TestPtrs;
};