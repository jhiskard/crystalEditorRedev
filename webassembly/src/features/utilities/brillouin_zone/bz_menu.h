#pragma once

namespace core::scene {
struct SceneState;
}

namespace features::utilities::bz {

class BZPlotController;

struct Request {
    enum class Type {
        Show,
    };

    Type type = Type::Show;
};

void InitOnce(core::scene::SceneState& scene);

void DrawMenu();
void HandleRequest(const Request& request);
void RenderWindows();
void Tick(float dt);
void Shutdown();

BZPlotController& Controller();

} // namespace features::utilities::bz
