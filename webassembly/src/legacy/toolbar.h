#pragma once

#include "macro/singleton_macro.h"
#include "enum/toolbar_enums.h"
#include "enum/viewer_enums.h"
#include "texture.h"

// Standard library
#include <cstdint>
#include <unordered_map>

struct ImVec2;


class Toolbar {
    DECLARE_SINGLETON(Toolbar)

public:
    //void Render(const ImVec2& windowPos, const ImVec2& windowSize);
    void Render(const ImVec2& viewerContentSize);
    // ✅ 새로 추가: Charge Density 애니메이션 컨트롤
    // Getters
    MeshDisplayMode GetMeshDisplayMode() const { return m_MeshDisplayMode; }
    ProjectionMode GetProjectionMode() const { return m_ProjectionMode; }

private:
    Anchor m_Anchor { Anchor::TOP };
    int32_t m_Padding;

    MeshDisplayMode m_MeshDisplayMode { MeshDisplayMode::WIRESHADED };
    std::unordered_map<MeshDisplayMode, TextureUPtr> m_MeshDisplayIconMap;

    ProjectionMode m_ProjectionMode { ProjectionMode::PERSPECTIVE };
    std::unordered_map<ProjectionMode, TextureUPtr> m_ProjectionIconMap;

    void init();
    void renderBoundaryAtomsQuickButton();
    void renderMeshDisplayModeButtons();
    void renderProjectionModeButtons();
    void renderResetViewButton();
    void renderCellAlignButtons();
    void renderChargeDensityControls();
    void renderArrowRotationStepInput();

};
