/**
 * @file main.cpp
 * @brief WebAssembly entry point for the Phase 1 app/core bootstrap shell.
 */
#include "app/app.h"

#include <emscripten/emscripten.h>

int main(int /*argc*/, char* /*argv*/[]) {
    app::App& app = app::App::Instance();
    if (app.Init() != 0) {
        return 1;
    }

    emscripten_set_main_loop([]() {
        app::App::Instance().RenderFrame();
    }, 0, true);

    return 0;
}
