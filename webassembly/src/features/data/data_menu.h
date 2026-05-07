#pragma once

namespace core::scene {
struct SceneState;
}

namespace features::data {

struct Request {
    enum class Type {
        ShowIsosurface,
        ShowSurface,
        ShowVolumetric,
        ShowPlane,
    };

    Type type = Type::ShowIsosurface;
};

void InitOnce(core::scene::SceneState& scene);

void DrawMenu();
void HandleRequest(const Request& request);
void RenderWindows();
void Tick(float dt);
void Shutdown();

} // namespace features::data

