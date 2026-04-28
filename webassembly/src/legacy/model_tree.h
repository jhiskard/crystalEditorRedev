#pragma once

#include "macro/singleton_macro.h"

// ImGui
#include <imgui.h>

// Standard library
#include <cstdint>

class TreeNode;


class ModelTree {
    DECLARE_SINGLETON(ModelTree)

public:
    void Render(bool* openWindow = nullptr);

    static int32_t GetSelectedMeshId() { return s_SelectedMeshId; }

private:
    static int32_t s_DeleteMeshId;
    static int32_t s_SelectedMeshId;

    void renderMeshTree(TreeNode* node);
    void renderMeshTable(ImGuiTableFlags tableFlags);
    void renderDeleteConfirmPopup();  // ✅ 추가
    void renderClearMeasurementsConfirmPopup();

    void renderXsfStructureTable(ImGuiTableFlags tableFlags);

    // ✅ 삭제 확인 팝업용 변수
    static int32_t s_PendingDeleteMeshId;
    static bool s_ShowDeleteConfirmPopup;
    static int32_t s_PendingClearMeasurementsStructureId;
    static bool s_ShowClearMeasurementsConfirmPopup;
}; 
