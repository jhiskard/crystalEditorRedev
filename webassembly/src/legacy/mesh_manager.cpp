#include "mesh_manager.h"
#include "vtk_viewer.h"
#include "mesh_detail.h"

// Standard library
#include <algorithm>
#include <cassert>

// VTK
#include <vtkDataSet.h>
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkScalarBarActor.h>


MeshManager::MeshManager() {
    // Create new mesh
    MeshUPtr meshRoot = Mesh::New("Mesh");
    assert(meshRoot != nullptr && "Failed to create mesh root!");

    // Create mesh tree
    m_MeshTree = LcrsTree::New(meshRoot->GetId(), meshRoot->GetName());
    m_IdToMeshMap[meshRoot->GetId()] = std::move(meshRoot);

#ifdef DEBUG_BUILD
    createTestMeshItems();
#endif
}

MeshManager::~MeshManager() {
}

// ============================================================================
// ✅ 추가: Helper function to build VolumeDisplaySettings from Mesh
// ============================================================================
namespace {
MeshManager::VolumeDisplaySettings buildVolumeDisplaySettingsFromMesh(const Mesh& mesh) {
    MeshManager::VolumeDisplaySettings settings;
    settings.renderMode = mesh.GetVolumeRenderMode();
    settings.volumeVisibility = mesh.GetVolumeMeshVisibility();
    settings.colorPreset = mesh.GetVolumeColorPreset();
    settings.colorMidpoint = mesh.GetVolumeColorMidpoint();
    settings.colorSharpness = mesh.GetVolumeColorSharpness();
    settings.window = mesh.GetVolumeWindow();
    settings.level = mesh.GetVolumeLevel();
    settings.opacityMin = mesh.GetVolumeRenderOpacityMin();
    settings.opacityMax = mesh.GetVolumeRenderOpacity();
    settings.sampleDistanceIndex = mesh.GetVolumeSampleDistanceIndex();
    settings.quality = mesh.GetVolumeQuality();

    const double* meshColor = mesh.GetVolumeMeshColor();
    if (meshColor) {
        settings.meshColor[0] = meshColor[0];
        settings.meshColor[1] = meshColor[1];
        settings.meshColor[2] = meshColor[2];
    }
    settings.meshOpacity = mesh.GetVolumeMeshOpacity();
    const double* edgeColor = mesh.GetVolumeMeshEdgeColor();
    if (edgeColor) {
        settings.meshEdgeColor[0] = edgeColor[0];
        settings.meshEdgeColor[1] = edgeColor[1];
        settings.meshEdgeColor[2] = edgeColor[2];
    }
    return settings;
}
}  // namespace

#ifdef DEBUG_BUILD
void MeshManager::createTestMeshItems() {
    MeshUPtr mesh1 = Mesh::New("Mesh1");
    TreeNode* node1 = m_MeshTree->InsertItem(mesh1->GetId(), mesh1->GetName());
    m_IdToMeshMap[mesh1->GetId()] = std::move(mesh1);

    MeshUPtr mesh2 = Mesh::New("Mesh2");
    TreeNode* node2 = m_MeshTree->InsertItem(mesh2->GetId(), mesh2->GetName());
    m_IdToMeshMap[mesh2->GetId()] = std::move(mesh2);

    MeshUPtr mesh3 = Mesh::New("Mesh3");
    TreeNode* node3 = m_MeshTree->InsertItem(mesh3->GetId(), mesh3->GetName());
    m_IdToMeshMap[mesh3->GetId()] = std::move(mesh3);

    MeshUPtr mesh4 = Mesh::New("Mesh4");
    TreeNode* node4 = m_MeshTree->InsertItem(mesh4->GetId(), mesh4->GetName());
    m_IdToMeshMap[mesh4->GetId()] = std::move(mesh4);

    MeshUPtr mesh11 = Mesh::New("Mesh11");
    TreeNode* node11 = m_MeshTree->InsertItem(mesh11->GetId(), mesh11->GetName(), node1);
    m_IdToMeshMap[mesh11->GetId()] = std::move(mesh11);

    MeshUPtr mesh12 = Mesh::New("Mesh12");
    TreeNode* node12 = m_MeshTree->InsertItem(mesh12->GetId(), mesh12->GetName(), node1);
    m_IdToMeshMap[mesh12->GetId()] = std::move(mesh12);

    MeshUPtr mesh13 = Mesh::New("Mesh13");
    TreeNode* node13 = m_MeshTree->InsertItem(mesh13->GetId(), mesh13->GetName(), node1);
    m_IdToMeshMap[mesh13->GetId()] = std::move(mesh13);

    MeshUPtr mesh21 = Mesh::New("Mesh21");
    TreeNode* node21 = m_MeshTree->InsertItem(mesh21->GetId(), mesh21->GetName(), node2);
    m_IdToMeshMap[mesh21->GetId()] = std::move(mesh21);

    MeshUPtr mesh22 = Mesh::New("Mesh22");
    TreeNode* node22 = m_MeshTree->InsertItem(mesh22->GetId(), mesh22->GetName(), node2);
    m_IdToMeshMap[mesh22->GetId()] = std::move(mesh22);

    MeshUPtr mesh23 = Mesh::New("Mesh23");
    TreeNode* node23 = m_MeshTree->InsertItem(mesh23->GetId(), mesh23->GetName(), node2);
    m_IdToMeshMap[mesh23->GetId()] = std::move(mesh23);

    MeshUPtr mesh131 = Mesh::New("Mesh131");
    TreeNode* node131 = m_MeshTree->InsertItem(mesh131->GetId(), mesh131->GetName(), node13);
    m_IdToMeshMap[mesh131->GetId()] = std::move(mesh131);

    MeshUPtr mesh132 = Mesh::New("Mesh132");
    TreeNode* node132 = m_MeshTree->InsertItem(mesh132->GetId(), mesh132->GetName(), node13);
    m_IdToMeshMap[mesh132->GetId()] = std::move(mesh132);

    MeshUPtr mesh133 = Mesh::New("Mesh133");
    TreeNode* node133 = m_MeshTree->InsertItem(mesh133->GetId(), mesh133->GetName(), node13);
    m_IdToMeshMap[mesh133->GetId()] = std::move(mesh133);
}
#endif

Mesh* MeshManager::InsertMesh(const char* name, vtkSmartPointer<vtkDataSet> edgeDataSet,
    vtkSmartPointer<vtkDataSet> faceDataSet, vtkSmartPointer<vtkDataSet> volumeDataSet,
    int32_t parentId) {
    if (parentId < 0) {
        assert(false && "Parent id should be greater than or equal to 0.");
        return nullptr;
    }

    // Create new mesh
    MeshUPtr newMesh = Mesh::New(name, edgeDataSet, faceDataSet, volumeDataSet);
    if (newMesh == nullptr) {
        return nullptr;
    }

    TreeNode* parentNode = m_MeshTree->GetTreeNodeByIdMutable(parentId);
    if (parentNode == nullptr) {
        return nullptr;
    }

    // Add the mesh actor to the VtkViewer
    VtkViewer& vtkViewer = VtkViewer::Instance();
    vtkViewer.AddActor(newMesh->GetEdgeMeshActor(), true);
    vtkViewer.AddActor(newMesh->GetFaceMeshActor(), true);
    vtkViewer.AddActor(newMesh->GetVolumeMeshActor(), true);
    
    // ✅ 추가: Volume Render Actor 추가 (있는 경우)
    if (newMesh->GetVolumeRenderActor()) {
        vtkViewer.AddVolume(newMesh->GetVolumeRenderActor(), true);
        newMesh->MarkVolumeRenderAdded();
    }

    // Save mesh data
    TreeNode* addedNode = m_MeshTree->InsertItem(newMesh->GetId(), newMesh->GetName(), parentNode);
    m_IdToMeshMap[newMesh->GetId()] = std::move(newMesh);

    // ✅ 추가: 공유 설정이 있으면 새 메시에 적용
    if (m_HasSharedVolumeDisplaySettings) {
        applyVolumeDisplaySettingsToMesh(m_IdToMeshMap.at(addedNode->GetId()).get(),
            m_SharedVolumeDisplaySettings);
    }

    return m_IdToMeshMap.at(addedNode->GetId()).get();
}

const Mesh* MeshManager::GetMeshById(int32_t id) const {
    if (m_IdToMeshMap.find(id) == m_IdToMeshMap.end()) {
        return nullptr;
    }
    return m_IdToMeshMap.at(id).get();
}

Mesh* MeshManager::GetMeshByIdMutable(int32_t id) {
    if (m_IdToMeshMap.find(id) == m_IdToMeshMap.end()) {
        return nullptr;
    }
    return m_IdToMeshMap.at(id).get();
}

#ifdef DEBUG_BUILD
void MeshManager::PrintMeshTree() {
    Instance().GetMeshTree()->TraverseTree([](const TreeNode* node, void*) {
        for (int i = 0; i < node->GetDepth(); ++i) {
            std::cout << "--";
        }
        std::cout << "[Node]: " << node->GetLabel() << ", [ID]: " << node->GetId();
        std::cout << ", [Parent]: " << (node->GetParent() ? node->GetParent()->GetLabel() : "NULL");
        std::cout << ", [Left Child]: " << (node->GetLeftChild() ? node->GetLeftChild()->GetLabel(): "NULL");
        std::cout << ", [Right Sibling]: " << (node->GetRightSibling() ? node->GetRightSibling()->GetLabel() : "NULL");
        std::cout << ", [Left Sibling]: " << (node->GetLeftSibling() ? node->GetLeftSibling()->GetLabel() : "NULL");
        std::cout << std::endl;
    });
}
#endif

void MeshManager::ShowMesh(int32_t id) {
    TreeNode* node = m_MeshTree->GetTreeNodeByIdMutable(id);
    if (node == nullptr) {
        return;
    }

    MeshManager& meshManager = MeshManager::Instance();

    Instance().GetMeshTree()->TraverseTreeMutable([&](TreeNode* node, void*) {
        Mesh* mesh = meshManager.GetMeshByIdMutable(node->GetId());
        MeshDetail& meshDetail = MeshDetail::Instance();
        if (node->GetIconState() == IconState::HIDDEN ||
            node->GetIconState() == IconState::PARTIAL) {
            if (meshDetail.GetUiEdgeMeshVisibility()) {
                mesh->SetEdgeMeshVisibility(true);
            }
            if (meshDetail.GetUiFaceMeshVisibility()) {
                mesh->SetFaceMeshVisibility(true);
            }
            if (meshDetail.GetUiVolumeMeshVisibility()) {
                mesh->SetVolumeMeshVisibility(true);
            }
            node->SetIconState(IconState::VISIBLE);
        }
    }, node);

    Instance().SetParentIconState(node);
}

void MeshManager::HideMesh(int32_t id) {
    TreeNode* node = m_MeshTree->GetTreeNodeByIdMutable(id);
    if (node == nullptr) {
        return;
    }

    Instance().GetMeshTree()->TraverseTreeMutable([&](TreeNode* node, void*) {
        Mesh* mesh = Instance().GetMeshByIdMutable(node->GetId());
        if (node->GetIconState() == IconState::VISIBLE ||
            node->GetIconState() == IconState::PARTIAL) {
            mesh->SetEdgeMeshVisibility(false);
            mesh->SetFaceMeshVisibility(false);
            mesh->SetVolumeMeshVisibility(false);
            node->SetIconState(IconState::HIDDEN);
        }
    }, node);

    Instance().SetParentIconState(node);
}

void MeshManager::SetParentIconState(TreeNode* node) {
    if (node->GetParent() == nullptr) {
        return;
    }

    IconState iconState = Instance().GetChildrenVisibility(node->GetParent());

    if (iconState == IconState::HIDDEN) {
        node->GetParentMutable()->SetIconState(IconState::HIDDEN);
    }
    else if (iconState == IconState::PARTIAL) {
        node->GetParentMutable()->SetIconState(IconState::PARTIAL);
    }
    else if (iconState == IconState::VISIBLE) {
        node->GetParentMutable()->SetIconState(IconState::VISIBLE);
    }

    if (node->GetParent()->GetParent()) {
        SetParentIconState(node->GetParentMutable());
    }
}

void MeshManager::DeleteMesh(int32_t id) {
    if (id == 0) {
        SPDLOG_ERROR("Cannot delete the root node.");
        return;
    }

    TreeNode* node = m_MeshTree->GetTreeNodeByIdMutable(id);
    if (node == nullptr) {
        assert(false && "Node is null.");
    }

    Instance().GetMeshTree()->TraverseTreeMutable([&](TreeNode* node, void*) {
        auto it = m_IdToMeshMap.find(node->GetId());
        if (it != m_IdToMeshMap.end()) {
            m_IdToMeshMap.erase(it);
        }
    }, node);

    m_MeshTree->DeleteItem(id);
}

IconState MeshManager::GetChildrenVisibility(const TreeNode* parentNode) const {
    if (parentNode == nullptr) {
        assert(false && "Parent node is null.");
        return IconState::INVALID;
    }

    int32_t childCount = parentNode->GetChildCount();
    int32_t visibleCount = 0;
    int32_t hiddenCount = 0;

    const TreeNode* childNode = parentNode->GetLeftChild();
    while (childNode) {
        if (childNode->GetIconState() == IconState::VISIBLE) {
            ++visibleCount;
        } else if (childNode->GetIconState() == IconState::HIDDEN) {
            ++hiddenCount;
        }
        childNode = childNode->GetRightSibling();
    }

    if (visibleCount == childCount) {
        return IconState::VISIBLE;
    } else if (hiddenCount == childCount) {
        return IconState::HIDDEN;
    } else {
        return IconState::PARTIAL;
    }
}

void MeshManager::SetDisplayMode(int32_t id, MeshDisplayMode mode) {
    Mesh* mesh = GetMeshByIdMutable(id);
    if (mesh == nullptr) {
        return;
    }
    mesh->SetDisplayMode(mode);
}

void MeshManager::SetAllDisplayMode(MeshDisplayMode mode) {
    for (const auto& idToMesh: m_IdToMeshMap) {
        Mesh* mesh = idToMesh.second.get();
        mesh->SetDisplayMode(mode);
    }
}

// ============================================================================
// ✅ 추가: Volume Display Settings Functions
// ============================================================================
void MeshManager::SetSharedVolumeDisplaySettings(const VolumeDisplaySettings& settings) {
    m_SharedVolumeDisplaySettings = settings;
    m_HasSharedVolumeDisplaySettings = true;
}

void MeshManager::EnsureSharedVolumeDisplaySettingsFromMesh(const Mesh& mesh) {
    if (m_HasSharedVolumeDisplaySettings) {
        return;
    }
    m_SharedVolumeDisplaySettings = buildVolumeDisplaySettingsFromMesh(mesh);
    
    double rangeMin = 0.0;
    double rangeMax = 1.0;
    if (GetGlobalVolumeDataRange(rangeMin, rangeMax)) {
        if (rangeMin == rangeMax) {
            rangeMax = rangeMin + 1.0;
        }
        m_SharedVolumeDisplaySettings.window = rangeMax - rangeMin;
        m_SharedVolumeDisplaySettings.level = (rangeMin + rangeMax) * 0.5;
    }
    m_HasSharedVolumeDisplaySettings = true;
    ApplySharedVolumeDisplaySettingsToAllMeshes();
}

void MeshManager::ApplySharedVolumeDisplaySettingsToAllMeshes() {
    if (!m_HasSharedVolumeDisplaySettings) {
        return;
    }
    MeshManager& meshManager = MeshManager::Instance();
    m_MeshTree->TraverseTree([&](const TreeNode* node, void*) {
        Mesh* mesh = meshManager.GetMeshByIdMutable(node->GetId());
        applyVolumeDisplaySettingsToMesh(mesh, m_SharedVolumeDisplaySettings);
    });
}

void MeshManager::applyVolumeDisplaySettingsToMesh(Mesh* mesh, const VolumeDisplaySettings& settings) {
    if (!mesh || !mesh->GetVolumeMeshActor()) {
        return;
    }
    mesh->SetVolumeRenderMode(settings.renderMode);
    mesh->SetVolumeMeshVisibility(settings.volumeVisibility);
    mesh->SetVolumeColorPreset(settings.colorPreset);
    mesh->SetVolumeColorCurve(settings.colorMidpoint, settings.colorSharpness);
    mesh->SetVolumeWindowLevel(settings.window, settings.level);
    mesh->SetVolumeQuality(settings.quality);
    mesh->SetVolumeSampleDistanceIndex(settings.sampleDistanceIndex);
    mesh->SetVolumeRenderOpacityMin(settings.opacityMin);
    mesh->SetVolumeRenderOpacity(settings.opacityMax);
    mesh->SetVolumeMeshColor(settings.meshColor[0], settings.meshColor[1], settings.meshColor[2]);
    mesh->SetVolumeMeshOpacity(settings.meshOpacity);
    mesh->SetVolumeMeshEdgeColor(settings.meshEdgeColor[0], settings.meshEdgeColor[1], settings.meshEdgeColor[2]);
}

bool MeshManager::GetGlobalVolumeDataRange(double& minOut, double& maxOut) const {
    bool hasRange = false;
    double minValue = 0.0;
    double maxValue = 0.0;
    
    m_MeshTree->TraverseTree([&](const TreeNode* node, void*) {
        const Mesh* mesh = GetMeshById(node->GetId());
        if (!mesh || !mesh->GetVolumeMeshActor()) {
            return;
        }
        const double* range = mesh->GetVolumeDataRange();
        if (!range) {
            return;
        }
        if (!hasRange) {
            minValue = range[0];
            maxValue = range[1];
            hasRange = true;
            return;
        }
        minValue = std::min(minValue, range[0]);
        maxValue = std::max(maxValue, range[1]);
    });
    
    if (!hasRange) {
        minOut = 0.0;
        maxOut = 1.0;
        return false;
    }
    minOut = minValue;
    maxOut = maxValue;
    return true;
}

// ============================================================================
// XSF Structure Management (기존 코드 유지)
// ============================================================================
int32_t MeshManager::RegisterXsfStructure(const std::string& fileName) {
    MeshUPtr newMesh = Mesh::New(fileName.c_str(), nullptr, nullptr, nullptr);
    if (newMesh == nullptr) {
        return -1;
    }
    
    newMesh->SetIsXsfStructure(true);
    
    TreeNode* rootNode = m_MeshTree->GetTreeNodeByIdMutable(0);
    if (rootNode == nullptr) {
        return -1;
    }
    
    int32_t newId = newMesh->GetId();
    TreeNode* addedNode = m_MeshTree->InsertItem(newId, newMesh->GetName(), rootNode);
    m_IdToMeshMap[newId] = std::move(newMesh);
    
    m_XsfStructureIds.push_back(newId);
    
    SPDLOG_INFO("Registered XSF structure '{}' with ID {}", fileName, newId);
    
    return newId;
}

void MeshManager::DeleteXsfStructure(int32_t id) {
    if (id == -1 || id == 0) {
        return;
    }
    
    auto it = m_IdToMeshMap.find(id);
    if (it == m_IdToMeshMap.end()) {
        return;
    }
    
    if (!it->second->IsXsfStructure()) {
        SPDLOG_WARN("Mesh ID {} is not an XSF structure", id);
        return;
    }

    TreeNode* node = m_MeshTree->GetTreeNodeByIdMutable(id);
    if (node) {
        m_MeshTree->TraverseTreeMutable([&](TreeNode* target, void*) {
            auto itMesh = m_IdToMeshMap.find(target->GetId());
            if (itMesh != m_IdToMeshMap.end()) {
                m_IdToMeshMap.erase(itMesh);
            }
        }, node);
    }
    
    m_MeshTree->DeleteItem(id);
    
    auto itId = std::find(m_XsfStructureIds.begin(), m_XsfStructureIds.end(), id);
    if (itId != m_XsfStructureIds.end()) {
        m_XsfStructureIds.erase(itId);
    }
    
    SPDLOG_INFO("Deleted XSF structure with ID {}", id);
}

void MeshManager::DeleteAllXsfStructures() {
    if (m_XsfStructureIds.empty()) {
        return;
    }

    const std::vector<int32_t> ids = m_XsfStructureIds;
    for (int32_t id : ids) {
        DeleteXsfStructure(id);
    }
}
