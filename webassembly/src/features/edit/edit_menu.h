#pragma once

namespace core::scene {
struct SceneState;
}

namespace core::vtk {
class MouseInteractor;
}

namespace features::edit {

struct Request {
    enum class Type {
        ShowCellInformation,
        ShowCreatedAtoms,
        ShowBondsManagement,
    };

    Type type = Type::ShowCellInformation;
};

void InitOnce(core::scene::SceneState& scene, core::vtk::MouseInteractor& mouseInteractor);

void DrawMenu();
void HandleRequest(const Request& request);
void RenderWindows();
void Tick(float dt);
void Shutdown();

bool IsBoundaryAtomsEnabled();
void SetBoundaryAtomsEnabled(bool enabled);
bool AlignCameraToCurrentCellAxis(int axisIndex);

} // namespace features::edit
