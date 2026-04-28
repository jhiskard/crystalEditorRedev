#pragma once

#include "macro/singleton_macro.h"

// Standard library
#include <cstdint>

class MeshGroup;


class MeshGroupDetail {
    DECLARE_SINGLETON(MeshGroupDetail)

public:
    void Render(int32_t meshId);

    static int32_t GetSelectedMeshGroupId() { return s_SelectedMeshGroupId; }

    void SetUiPointSize(float size) { m_UiPointSize = size; }
    void SetUiGroupColor(const double* color);

    void SetHasPointSizeChanged(bool hasChanged) { s_HasPointSizeChanged = hasChanged; }
    void SetHasGroupColorChanged(bool hasChanged) { s_HasGroupColorChanged = hasChanged; }

private:
    static int32_t s_SelectedMeshGroupId;
    static int32_t s_DeleteMeshGroupId;

    float m_UiPointSize { 0.0f };
    float m_UiGroupColor[3] { 0.0f, 0.0f, 0.0f };

    static bool s_HasPointSizeChanged;
    static bool s_HasGroupColorChanged;

    void renderTableRow(const char* item, int32_t value);
    void renderTableRow(const char* item, const char* value = nullptr);
    void renderTableRow(const char* item, double value);

    void renderTableRowPointSize(MeshGroup* group);
    void renderTableRowGroupColor(MeshGroup* group);
};