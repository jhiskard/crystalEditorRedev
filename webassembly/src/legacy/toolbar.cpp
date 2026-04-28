#include "toolbar.h"
#include "config/log_config.h"
#include "app.h"
#include "atoms/atoms_template.h"
#include "atoms/domain/cell_manager.h"
#include "mesh_manager.h"
#include "vtk_viewer.h"
#include "icon/FontAwesome6.h"

// ImGui
#include <imgui.h>
#include <imgui_internal.h>

#include "atoms/ui/charge_density_ui.h"


Toolbar::Toolbar()
    : m_Padding(5 * App::DevicePixelRatio()) {
    init();
}

Toolbar::~Toolbar() {
}

void Toolbar::init() {
    // Mesh display mode icons
    ImageUPtr imgWireframe = Image::New("resources/icon_image/MeshDisplayMode_Wireframe_32x32.png");
    ImageUPtr imgShaded = Image::New("resources/icon_image/MeshDisplayMode_Shaded_32x32.png");
    ImageUPtr imgWireShaded = Image::New("resources/icon_image/MeshDisplayMode_WireShaded_32x32.png");
    m_MeshDisplayIconMap.insert(std::make_pair(MeshDisplayMode::WIREFRAME, Texture::New(imgWireframe.get())));
    m_MeshDisplayIconMap.insert(std::make_pair(MeshDisplayMode::SHADED, Texture::New(imgShaded.get())));
    m_MeshDisplayIconMap.insert(std::make_pair(MeshDisplayMode::WIRESHADED, Texture::New(imgWireShaded.get())));

    // Projection mode icons
    ImageUPtr imgParallel = Image::New("resources/icon_image/ProjectionMode_Parallel_32x32.png");
    ImageUPtr imgPerspective = Image::New("resources/icon_image/ProjectionMode_Perspective_32x32.png");
    m_ProjectionIconMap.insert(std::make_pair(ProjectionMode::PARALLEL, Texture::New(imgParallel.get())));
    m_ProjectionIconMap.insert(std::make_pair(ProjectionMode::PERSPECTIVE, Texture::New(imgPerspective.get())));
}

void Toolbar::Render(const ImVec2& viewerContentSize) {
    // 현재 Viewer 창의 콘텐츠 영역 기준으로 위치 계산
    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    
    // Toolbar 위치 계산 (상단 중앙 또는 하단 중앙)
    float toolbarWidth = 250.0f;  // 대략적인 너비 (자동 조정됨)
    float xPos, yPos;
    
    if (m_Anchor == Anchor::TOP) {
        xPos = contentMin.x + (contentMax.x - contentMin.x) / 2.0f - toolbarWidth / 2.0f;
        yPos = contentMin.y + m_Padding;
    }
    else {  // BOTTOM
        xPos = contentMin.x + (contentMax.x - contentMin.x) / 2.0f - toolbarWidth / 2.0f;
        yPos = contentMax.y - 60.0f - m_Padding;
    }
    
    // 커서 위치 설정
    ImGui::SetCursorPos(ImVec2(xPos, yPos));
    
    // Child Window로 Toolbar 렌더링
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    
    ImGui::BeginChild("##ToolbarChild", ImVec2(0, 0), 
        ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY,
        ImGuiWindowFlags_NoScrollbar);
    
    renderBoundaryAtomsQuickButton();
    ImGui::SameLine();
    renderMeshDisplayModeButtons();
    ImGui::SameLine();
    renderProjectionModeButtons();
    ImGui::SameLine();
    renderResetViewButton();
    
    if (atoms::domain::hasUnitCell()) {
        ImGui::SameLine();
        renderCellAlignButtons();
    }
    
    // ✅ Charge Density 컨트롤 추가
    if (AtomsTemplate::Instance().HasChargeDensity() &&
        AtomsTemplate::Instance().IsChargeDensitySimpleViewActive()) {
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        renderChargeDensityControls();
    }

    ImGui::EndChild();

    const float stepToolbarWidth = 360.0f;
    const float stepToolbarHeight = 42.0f;
    float stepXPos = contentMin.x + (contentMax.x - contentMin.x) / 2.0f - stepToolbarWidth / 2.0f;
    float stepYPos = contentMax.y - stepToolbarHeight - m_Padding;
    if (stepYPos < contentMin.y + m_Padding) {
        stepYPos = contentMin.y + m_Padding;
    }

    ImGui::SetCursorPos(ImVec2(stepXPos, stepYPos));
    ImGui::BeginChild("##ToolbarStepChild", ImVec2(0, 0),
        ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY,
        ImGuiWindowFlags_NoScrollbar);
    renderArrowRotationStepInput();
    ImGui::EndChild();
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    
    // ✅ 애니메이션 업데이트 (매 프레임)
    if (AtomsTemplate::Instance().HasChargeDensity() &&
        AtomsTemplate::Instance().IsChargeDensitySimpleViewActive()) {
        auto* cdUI = AtomsTemplate::Instance().chargeDensityUI();
        if (cdUI) {
            cdUI->updateAnimation();
        }
    }
}

void Toolbar::renderBoundaryAtomsQuickButton() {
    AtomsTemplate& atomsTemplate = AtomsTemplate::Instance();
    const bool boundaryAtomsEnabled = atomsTemplate.isSurroundingsVisible();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));
    if (boundaryAtomsEnabled) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.25f, 0.65f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.55f, 0.30f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.35f, 0.20f, 0.90f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.3f, 0.2f));
    }

    const char* iconLabel = boundaryAtomsEnabled
        ? ICON_FA6_BORDER_ALL
        : ICON_FA6_BORDER_NONE;
    if (ImGui::Button(iconLabel, ImVec2(46.0f, 46.0f))) {
        atomsTemplate.SetBoundaryAtomsEnabled(!boundaryAtomsEnabled);
    }

    if (boundaryAtomsEnabled) {
        ImGui::PopStyleColor(3);
    } else {
        ImGui::PopStyleColor(1);
    }
    ImGui::PopStyleVar();

    App::AddTooltip(
        "Boundary atoms",
        boundaryAtomsEnabled ? "Boundary atoms: ON (click to hide)" : "Boundary atoms: OFF (click to show)");
}

void Toolbar::renderArrowRotationStepInput() {
    int stepDeg = static_cast<int>(VtkViewer::Instance().GetArrowRotateStepDeg());

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.3f, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.2f, 0.3f, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.2f, 0.3f, 0.2f));
    ImGui::Button("Step (degree)");
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.0f * App::UiScale());
    if (ImGui::InputInt("##ArrowRotationStepDegree", &stepDeg, 1, 1)) {
        if (stepDeg < 1) {
            stepDeg = 1;
        }
        else if (stepDeg > 180) {
            stepDeg = 180;
        }
        VtkViewer::Instance().SetArrowRotateStepDeg(static_cast<float>(stepDeg));
    }
    App::AddTooltip("Arrow key camera rotation step (1 - 180 degrees)");
}


void Toolbar::renderChargeDensityControls() {
    auto* cdUI = AtomsTemplate::Instance().chargeDensityUI();
    if (!cdUI || !cdUI->hasData()) return;

    bool isPlaying = cdUI->isPlaying();
    float level = cdUI->getLevelPercent() * 100.0f; // 0~100%

    // =========================
    // Play / Pause 버튼
    // =========================
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.3f, 0.2f));

    const char* playIcon = isPlaying ? ICON_FA6_PAUSE : ICON_FA6_PLAY;
    const char* playTooltip = isPlaying
        ? "Pause animation"
        : "Play from current charge density level";

    if (ImGui::Button(playIcon, ImVec2(46.0f, 46.0f))) {
        if (isPlaying) {
            cdUI->stopAnimation();          // Pause
        } else {
            cdUI->startAnimation(10.0f);    // ▶ 현재 level에서 재생
        }
    }
    App::AddTooltip("Play/Pause", playTooltip);

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    // =========================
    // Restart 버튼
    // =========================
    ImGui::SameLine();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 0.25f));

    if (ImGui::Button(ICON_FA6_ROTATE_LEFT, ImVec2(46.0f, 46.0f))) {
        cdUI->stopAnimation();
        cdUI->setLevelPercent(0.0f, false); // 0%로 리셋
        cdUI->startAnimation(10.0f);        // 처음부터 재생
    }
    App::AddTooltip("Restart", "Restart charge density animation from beginning");

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    // =========================
    // Level 슬라이더
    // =========================
    ImGui::SameLine();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
    ImGui::PushItemWidth(120.0f);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.5f, 0.8f, 1.0f, 1.0f));

    // 수직 중앙 정렬
    float sliderHeight = ImGui::GetFrameHeight();
    float buttonHeight = 46.0f;
    float offsetY = (buttonHeight - sliderHeight) * 0.5f;
    ImVec2 cursorPos = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(cursorPos.y + offsetY);

    if (ImGui::SliderFloat("##DensityLevel", &level, 0.0f, 100.0f, "%.0f%%")) {
        cdUI->stopAnimation(); // 슬라이더 조작 시 정지
        cdUI->setLevelPercent(level / 100.0f, true);
    }

    ImGui::PopStyleColor(5);
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Charge density level: %.1f%%\nDrag to adjust manually",
            level
        );
    }
}

//void Toolbar::Render(const ImVec2& windowPos, const ImVec2& windowSize) {
//    if (m_Anchor == Anchor::TOP) {
//        float xPos = windowPos.x + windowSize.x / 2;
//        float yPos = windowPos.y + App::TitleBarHeight() + m_Padding;
//        ImGui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
//    }
//    else if (m_Anchor == Anchor::BOTTOM) {
//        float xPos = windowPos.x + windowSize.x / 2;
//        float yPos = windowPos.y + windowSize.y + App::TitleBarHeight() - m_Padding;
//
//        ImGui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
//    }
//    ImGui::SetNextWindowBgAlpha(0.0f);  // Set transparent background
//    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
//    ImGui::Begin("Toolbar", nullptr, 
//        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
//    renderMeshDisplayModeButtons();
//    ImGui::SameLine();
//    renderProjectionModeButtons();
//    ImGui::SameLine();
//    renderResetViewButton();
//    if (atoms::domain::hasUnitCell()) {
//        ImGui::SameLine();
//        renderCellAlignButtons();
//    }
//    ImGui::End();
//    ImGui::PopStyleVar();  // ImGuiStyleVar_WindowBorderSize
//    // Bring the toolbar window to the front
//    ImGuiWindow* window = ImGui::FindWindowByName("Toolbar");
//    if (window) {
//        ImGui::BringWindowToDisplayFront(window);
//    }
//}

void Toolbar::renderMeshDisplayModeButtons() {
    const TextureUPtr& texCurMeshDispMode = m_MeshDisplayIconMap.at(m_MeshDisplayMode);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1, 0.2, 0.3, 0.2));
    bool isMeshDispModeBtnClicked = ImGui::ImageButton("MeshDispMode",
        (ImTextureID)(intptr_t)texCurMeshDispMode->Get(),
        ImVec2(texCurMeshDispMode->GetWidth(), texCurMeshDispMode->GetHeight()), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();  // ImGuiStyleVar_FramePadding

    // Add tooltip on mesh display mode button
    if (m_MeshDisplayMode == MeshDisplayMode::WIREFRAME) {
        App::AddTooltip("Mesh display mode", "Wireframe");
    }
    else if (m_MeshDisplayMode == MeshDisplayMode::SHADED) {
        App::AddTooltip("Mesh display mode", "Shaded");
    }
    else {
        App::AddTooltip("Mesh display mode", "Shaded with Wireframe");
    }

    // Calculate the pop-up position with button position and size.
    ImVec2 meshDispModeBtnPos = ImGui::GetItemRectMin();
    ImVec2 meshDispModeBtnSize = ImGui::GetItemRectSize();
    meshDispModeBtnPos.y += meshDispModeBtnSize.y;

    if (isMeshDispModeBtnClicked) {
        ImGui::OpenPopup("MeshDispModePopup");
    }

    // Combo drop-down
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowPos(meshDispModeBtnPos);  // Set the pop-up position
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
    bool isPopupOpen = ImGui::BeginPopup("MeshDispModePopup");
    ImGui::PopStyleVar(2);  // ImGuiStyleVar_WindowPadding, ImGuiStyleVar_PopupBorderSize

    if (isPopupOpen) {
        const TextureUPtr& texWireframe = m_MeshDisplayIconMap.at(MeshDisplayMode::WIREFRAME);
        const TextureUPtr& texShaded = m_MeshDisplayIconMap.at(MeshDisplayMode::SHADED);
        const TextureUPtr& texWireShaded = m_MeshDisplayIconMap.at(MeshDisplayMode::WIRESHADED);
        
        bool isWireframeClicked = false;
        bool isShadedClicked = false;
        bool isWireShadedClicked = false;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 6.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0, 0.0, 0.0, 0.0));

        // Wireframe icon
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        isWireframeClicked = ImGui::ImageButton("MeshDispWireframe",
            (ImTextureID)(intptr_t)texWireframe->Get(),
            ImVec2(texWireframe->GetWidth(), texWireframe->GetHeight()), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::PopStyleVar();  // ImGuiStyleVar_ItemSpacing
        App::AddTooltip("Wireframe", "All meshes are displayed in wireframe mode.");

        // Shaded icon
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        isShadedClicked = ImGui::ImageButton("MeshDispShaded",
            (ImTextureID)(intptr_t)texShaded->Get(),
            ImVec2(texShaded->GetWidth(), texShaded->GetHeight()), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::PopStyleVar();  // ImGuiStyleVar_ItemSpacing
        App::AddTooltip("Shaded", "All meshes are displayed in shaded mode.");

        // Wireshaded icon
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        isWireShadedClicked = ImGui::ImageButton("MeshDispWireShaded",
            (ImTextureID)(intptr_t)texWireShaded->Get(),
            ImVec2(texWireShaded->GetWidth(), texWireShaded->GetHeight()), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::PopStyleVar();  // ImGuiStyleVar_ItemSpacing
        App::AddTooltip("Shaded with Wireframe", "All meshes are displayed in shaded with wireframe mode.");

        ImGui::PopStyleColor();  // ImGuiCol_Button
        ImGui::PopStyleVar();  // ImGuiStyleVar_FramePadding

        MeshManager& meshManager = MeshManager::Instance();

        if (isWireframeClicked) {
            if (m_MeshDisplayMode != MeshDisplayMode::WIREFRAME) {
                m_MeshDisplayMode = MeshDisplayMode::WIREFRAME;
                meshManager.SetAllDisplayMode(MeshDisplayMode::WIREFRAME);
            }
            ImGui::CloseCurrentPopup();
        }
        else if (isShadedClicked) {
            if (m_MeshDisplayMode != MeshDisplayMode::SHADED) {  
                m_MeshDisplayMode = MeshDisplayMode::SHADED;
                meshManager.SetAllDisplayMode(MeshDisplayMode::SHADED);
            }
            ImGui::CloseCurrentPopup();
        }
        else if (isWireShadedClicked) {
            if (m_MeshDisplayMode != MeshDisplayMode::WIRESHADED) {
                m_MeshDisplayMode = MeshDisplayMode::WIRESHADED;
                meshManager.SetAllDisplayMode(MeshDisplayMode::WIRESHADED);
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void Toolbar::renderProjectionModeButtons() {
    const TextureUPtr& texCurProjMode = m_ProjectionIconMap.at(m_ProjectionMode);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1, 0.2, 0.3, 0.2));
    bool isProjModeBtnClicked = ImGui::ImageButton("ProjectionMode",
        (ImTextureID)(intptr_t)texCurProjMode->Get(),
        ImVec2(texCurProjMode->GetWidth(), texCurProjMode->GetHeight()), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();  // ImGuiStyleVar_FramePadding

    // Add tooltip on projection mode button
    if (m_ProjectionMode == ProjectionMode::PERSPECTIVE) {
        App::AddTooltip("Projection mode", "Perspective View");
    }
    else if (m_ProjectionMode == ProjectionMode::PARALLEL) {
      App::AddTooltip("Projection mode", "Parallel View");
    }

    // Calculate the pop-up position with button position and size.
    ImVec2 projModeBtnPos = ImGui::GetItemRectMin();
    ImVec2 projModeBtnSize = ImGui::GetItemRectSize();
    projModeBtnPos.y += projModeBtnSize.y;

    if (isProjModeBtnClicked) {
        ImGui::OpenPopup("ProjectionModePopup");
    }

    // Combo drop-down
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowPos(projModeBtnPos);  // Set the pop-up position
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
    bool isPopupOpen = ImGui::BeginPopup("ProjectionModePopup");
    ImGui::PopStyleVar(2);  // ImGuiStyleVar_WindowPadding, ImGuiStyleVar_PopupBorderSize

    if (isPopupOpen) {
        const TextureUPtr& texPerspective = m_ProjectionIconMap.at(ProjectionMode::PERSPECTIVE);
        const TextureUPtr& texParallel = m_ProjectionIconMap.at(ProjectionMode::PARALLEL);
        
        bool isPerspectiveClicked = false;
        bool isParallelClicked = false;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 6.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0, 0.0, 0.0, 0.0));

        // Perspective icon
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        isPerspectiveClicked = ImGui::ImageButton("ProjectionPerspective",
            (ImTextureID)(intptr_t)texPerspective->Get(),
            ImVec2(texPerspective->GetWidth(), texPerspective->GetHeight()), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::PopStyleVar();  // ImGuiStyleVar_ItemSpacing
        App::AddTooltip("Perspective View");

        // Parallel icon
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        isParallelClicked = ImGui::ImageButton("ProjectionParallel",
            (ImTextureID)(intptr_t)texParallel->Get(),
            ImVec2(texParallel->GetWidth(), texParallel->GetHeight()), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::PopStyleVar();  // ImGuiStyleVar_ItemSpacing
        App::AddTooltip("Parallel View");

        ImGui::PopStyleColor();  // ImGuiCol_Button
        ImGui::PopStyleVar();  // ImGuiStyleVar_FramePadding

        if (isPerspectiveClicked) {
            if (m_ProjectionMode != ProjectionMode::PERSPECTIVE) {
                m_ProjectionMode = ProjectionMode::PERSPECTIVE;
                VtkViewer::Instance().SetProjectionMode(ProjectionMode::PERSPECTIVE);
            }
            ImGui::CloseCurrentPopup();
        }
        else if (isParallelClicked) {
            if (m_ProjectionMode != ProjectionMode::PARALLEL) {
                m_ProjectionMode = ProjectionMode::PARALLEL;  
                VtkViewer::Instance().SetProjectionMode(ProjectionMode::PARALLEL);
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void Toolbar::renderResetViewButton() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1, 0.2, 0.3, 0.2));
    bool clicked = ImGui::Button(ICON_FA6_ARROWS_ROTATE, ImVec2(46.0f, 46.0f));
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();  // ImGuiStyleVar_FramePadding

    App::AddTooltip("Reset View", "Reset camera to fit current view");

    if (clicked) {
        VtkViewer::Instance().ResetView();
    }
}

void Toolbar::renderCellAlignButtons() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1, 0.2, 0.3, 0.2));

    const bool isBzMode = AtomsTemplate::Instance().IsBZPlotMode();
    const char* const labelsA[3] = { "a1", "a2", "a3" };
    const char* const labelsB[3] = { "b1", "b2", "b3" };
    const char* const* labels = isBzMode ? labelsB : labelsA;

    const ImVec2 btnSize(32.0f, 46.0f);
    if (ImGui::Button(labels[0], btnSize)) {
        if (isBzMode) {
            VtkViewer::Instance().AlignCameraToIcellAxis(0);
        }
        else {
            VtkViewer::Instance().AlignCameraToCellAxis(0);
        }
    }
    App::AddTooltip(isBzMode ? "Align camera to icell[0] direction" : "Align camera to cell[0] direction");

    ImGui::SameLine();
    if (ImGui::Button(labels[1], btnSize)) {
        if (isBzMode) {
            VtkViewer::Instance().AlignCameraToIcellAxis(1);
        }
        else {
            VtkViewer::Instance().AlignCameraToCellAxis(1);
        }
    }
    App::AddTooltip(isBzMode ? "Align camera to icell[1] direction" : "Align camera to cell[1] direction");

    ImGui::SameLine();
    if (ImGui::Button(labels[2], btnSize)) {
        if (isBzMode) {
            VtkViewer::Instance().AlignCameraToIcellAxis(2);
        }
        else {
            VtkViewer::Instance().AlignCameraToCellAxis(2);
        }
    }
    App::AddTooltip(isBzMode ? "Align camera to icell[2] direction" : "Align camera to cell[2] direction");

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();  // ImGuiStyleVar_FramePadding
}
