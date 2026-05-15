#pragma once

#include "toolbar.h"

namespace features::viewer {

class ViewerPanel {
public:
    void Render(bool* open);

private:
    Toolbar toolbar_;
};

} // namespace features::viewer
