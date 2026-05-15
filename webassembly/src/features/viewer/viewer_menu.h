#pragma once

namespace core::scene {
struct SceneState;
}

namespace core::vtk {
class MouseInteractor;
}

namespace features::viewer {

struct Request {
    enum class Type {
        ShowViewer,
    };

    Type type = Type::ShowViewer;
};

void InitOnce(core::scene::SceneState& scene, core::vtk::MouseInteractor& mouseInteractor);
void DrawMenus();
void HandleRequest(const Request& request);
void RenderWindows();
void Shutdown();

} // namespace features::viewer
