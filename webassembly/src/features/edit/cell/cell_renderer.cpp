#include "cell_renderer.h"

#include "core/vtk/vtk_viewer.h"

#include <vtkActor.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

namespace features::edit::cell {

CellRenderer::CellRenderer(core::scene::SceneState& scene)
    : scene_(scene) {
}

CellRenderer::~CellRenderer() {
    ClearAllUnitCells();
}

void CellRenderer::Subscribe() {
    if (subscribed_) {
        return;
    }
    subscribed_ = true;

    scene_.events.onCellChanged.Subscribe([this](const core::scene::CellChangedEvent& event) {
        OnCellChanged(event.structureId);
    });
    scene_.events.onStructureRemoved.Subscribe([this](const core::scene::StructureRemovedEvent& event) {
        ClearUnitCell(event.structureId);
    });
}

void CellRenderer::CreateUnitCell(const std::array<std::array<float, 3>, 3>& matrix) {
    CreateUnitCell(-1, matrix);
}

void CellRenderer::CreateUnitCell(int32_t structureId, const std::array<std::array<float, 3>, 3>& matrix) {
    int32_t sid = structureId;
    if (sid < 0) {
        sid = scene_.currentStructureId;
    }
    if (sid < 0) {
        return;
    }

    ClearUnitCell(sid);

    std::array<std::array<float, 3>, 8> vertices = {{
        {{0.0f, 0.0f, 0.0f}},
        {{matrix[0][0], matrix[0][1], matrix[0][2]}},
        {{matrix[1][0], matrix[1][1], matrix[1][2]}},
        {{matrix[0][0] + matrix[1][0], matrix[0][1] + matrix[1][1], matrix[0][2] + matrix[1][2]}},
        {{matrix[2][0], matrix[2][1], matrix[2][2]}},
        {{matrix[0][0] + matrix[2][0], matrix[0][1] + matrix[2][1], matrix[0][2] + matrix[2][2]}},
        {{matrix[1][0] + matrix[2][0], matrix[1][1] + matrix[2][1], matrix[1][2] + matrix[2][2]}},
        {{matrix[0][0] + matrix[1][0] + matrix[2][0],
          matrix[0][1] + matrix[1][1] + matrix[2][1],
          matrix[0][2] + matrix[1][2] + matrix[2][2]}},
    }};

    constexpr int edges[12][2] = {
        {0, 1}, {0, 2}, {0, 4},
        {1, 3}, {1, 5},
        {2, 3}, {2, 6},
        {3, 7},
        {4, 5}, {4, 6},
        {5, 7}, {6, 7},
    };

    bool storedVisible = true;
    if (const auto recordIt = scene_.structureRecords.find(sid); recordIt != scene_.structureRecords.end()) {
        storedVisible = recordIt->second.cell.visible;
    }
    const auto itVisible = unitCellVisibleByStructure_.find(sid);
    if (itVisible != unitCellVisibleByStructure_.end()) {
        storedVisible = itVisible->second;
    }
    const bool effectiveVisible = storedVisible && !unitCellGlobalHidden_;

    auto& actors = cellEdgeActorsByStructure_[sid];
    actors.clear();

    for (int i = 0; i < 12; ++i) {
        const int startIdx = edges[i][0];
        const int endIdx = edges[i][1];

        vtkSmartPointer<vtkLineSource> lineSource = vtkSmartPointer<vtkLineSource>::New();
        lineSource->SetPoint1(vertices[startIdx].data());
        lineSource->SetPoint2(vertices[endIdx].data());
        lineSource->Update();

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(lineSource->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->SetPickable(false);
        actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
        actor->GetProperty()->SetLineWidth(2.0);
        actor->SetVisibility(effectiveVisible ? 1 : 0);

        core::vtk::VtkViewer::Instance().AddActor(actor);
        actors.push_back(actor);
    }

    unitCellVisibleByStructure_[sid] = storedVisible;
    core::vtk::VtkViewer::Instance().RequestRender();
}

void CellRenderer::ClearUnitCell() {
    ClearAllUnitCells();
    core::vtk::VtkViewer::Instance().RequestRender();
}

void CellRenderer::ClearUnitCell(int32_t structureId) {
    int32_t sid = structureId;
    if (sid < 0) {
        sid = scene_.currentStructureId;
    }
    if (sid < 0) {
        return;
    }

    const auto it = cellEdgeActorsByStructure_.find(sid);
    if (it == cellEdgeActorsByStructure_.end()) {
        return;
    }

    for (const auto& actor : it->second) {
        if (actor != nullptr) {
            core::vtk::VtkViewer::Instance().RemoveActor(actor);
        }
    }
    cellEdgeActorsByStructure_.erase(it);
    unitCellVisibleByStructure_.erase(sid);
    core::vtk::VtkViewer::Instance().RequestRender();
}

void CellRenderer::SetUnitCellVisible(bool visible) {
    for (auto& [structureId, record] : scene_.structureRecords) {
        (void)structureId;
        record.cell.visible = visible;
    }
    unitCellGlobalHidden_ = !visible;

    for (auto& [structureId, actors] : cellEdgeActorsByStructure_) {
        bool structureVisible = true;
        const auto itVisible = unitCellVisibleByStructure_.find(structureId);
        if (itVisible != unitCellVisibleByStructure_.end()) {
            structureVisible = itVisible->second;
        }

        const bool effectiveVisible = visible && structureVisible;
        for (const auto& actor : actors) {
            if (actor != nullptr) {
                actor->SetVisibility(effectiveVisible ? 1 : 0);
            }
        }
    }
    core::vtk::VtkViewer::Instance().RequestRender();
}

void CellRenderer::SetUnitCellVisible(int32_t structureId, bool visible) {
    int32_t sid = structureId;
    if (sid < 0) {
        sid = scene_.currentStructureId;
    }
    if (sid < 0) {
        return;
    }

    if (auto recordIt = scene_.structureRecords.find(sid); recordIt != scene_.structureRecords.end()) {
        recordIt->second.cell.visible = visible;
    }

    unitCellVisibleByStructure_[sid] = visible;

    const auto it = cellEdgeActorsByStructure_.find(sid);
    if (it == cellEdgeActorsByStructure_.end()) {
        return;
    }

    const bool effectiveVisible = visible && !unitCellGlobalHidden_;
    for (const auto& actor : it->second) {
        if (actor != nullptr) {
            actor->SetVisibility(effectiveVisible ? 1 : 0);
        }
    }
    core::vtk::VtkViewer::Instance().RequestRender();
}

bool CellRenderer::HasUnitCell(int32_t structureId) const {
    int32_t sid = structureId;
    if (sid < 0) {
        sid = scene_.currentStructureId;
    }
    const auto it = cellEdgeActorsByStructure_.find(sid);
    return it != cellEdgeActorsByStructure_.end() && !it->second.empty();
}

bool CellRenderer::IsUnitCellVisible(int32_t structureId) const {
    int32_t sid = structureId;
    if (sid < 0) {
        sid = scene_.currentStructureId;
    }
    const auto it = unitCellVisibleByStructure_.find(sid);
    if (it == unitCellVisibleByStructure_.end()) {
        if (const auto recordIt = scene_.structureRecords.find(sid); recordIt != scene_.structureRecords.end()) {
            return recordIt->second.cell.visible;
        }
        return false;
    }
    return it->second;
}

void CellRenderer::OnCellChanged(int32_t structureId) {
    const auto it = scene_.structureRecords.find(structureId);
    if (it == scene_.structureRecords.end() || !it->second.cell.hasCell) {
        ClearUnitCell(structureId);
        return;
    }
    CreateUnitCell(structureId, it->second.cell.matrix);
}

void CellRenderer::ClearAllUnitCells() {
    for (auto& [structureId, actors] : cellEdgeActorsByStructure_) {
        (void)structureId;
        for (auto& actor : actors) {
            if (actor != nullptr) {
                core::vtk::VtkViewer::Instance().RemoveActor(actor);
            }
        }
    }
    cellEdgeActorsByStructure_.clear();
    unitCellVisibleByStructure_.clear();
}

} // namespace features::edit::cell
