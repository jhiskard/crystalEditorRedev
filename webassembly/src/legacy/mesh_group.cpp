#include "mesh_group.h"

// Standard library
#include <cassert>

// VTK
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkVertexGlyphFilter.h>


int32_t MeshGroup::m_LastId = -1;

MeshGroupUPtr MeshGroup::New(const char* name, MeshGroupType type,
    vtkSmartPointer<vtkDataSet> dataSet) {
    if (name == nullptr || type == MeshGroupType::INVALID ||
        dataSet == nullptr) {
        return nullptr;
    }

    MeshGroupUPtr meshGroup = MeshGroupUPtr(new MeshGroup());

    meshGroup->m_Id = GenerateNewId();
    meshGroup->m_Name = name;
    meshGroup->m_Type = type;
    meshGroup->m_DataSet = dataSet;

    meshGroup->createGroupMapper();
    meshGroup->createGroupActor();

    return std::move(meshGroup);
}

void MeshGroup::createGroupMapper() {
    assert("DataSet should not be empty for MeshGroup!" && m_DataSet != nullptr);

    if (m_GroupMapper == nullptr) {
        m_GroupMapper = vtkSmartPointer<vtkDataSetMapper>::New();
    }

    if (m_Type == MeshGroupType::NODE) {
        vtkSmartPointer<vtkVertexGlyphFilter> vertexFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
        vertexFilter->SetInputData(m_DataSet);
        vertexFilter->Update();

        m_GroupMapper->SetInputConnection(vertexFilter->GetOutputPort());
    }

    // TODO: Add other mesh group types
}

void MeshGroup::createGroupActor() {
    assert("DataSetMapper should not be empty for MeshGroup!" && m_GroupMapper != nullptr);

    if (m_GroupActor == nullptr) {
        m_GroupActor = vtkSmartPointer<vtkActor>::New();
    }
    m_GroupActor->SetMapper(m_GroupMapper);

    // Set default properties
    m_GroupActor->GetProperty()->SetPointSize(7.0f);
    m_GroupActor->GetProperty()->SetColor(0.7, 0.0, 0.0);
    m_GroupActor->SetVisibility(false);  // Default visibility is false.
}

const char* MeshGroup::GetMeshGroupTypeStr(MeshGroupType type) {
    switch (type) {
        case MeshGroupType::NODE: return "Node";
        case MeshGroupType::EDGE: return "Edge";
        case MeshGroupType::FACE: return "Face";
        case MeshGroupType::VOLUME: return "Volume";
        default: return "Invalid";
    }
}

bool MeshGroup::GetVisibility() const {
    if (m_GroupActor == nullptr) {
        return false;
    }
    return m_GroupActor->GetVisibility();
}

void MeshGroup::SetVisibility(bool visibility) {
    if (m_GroupActor == nullptr) {
        return;
    }
    m_GroupActor->SetVisibility(visibility);
}

const float MeshGroup::GetPointSize() const {
    if (m_GroupActor == nullptr) {
        return 0.0f;
    }
    return m_GroupActor->GetProperty()->GetPointSize();
}

const double* MeshGroup::GetGroupColor() const {
    if (m_GroupActor == nullptr) {
        return nullptr;
    }
    return m_GroupActor->GetProperty()->GetColor();
}

void MeshGroup::SetPointSize(float size) {
    if (m_GroupActor == nullptr) {
        return;
    }
    m_GroupActor->GetProperty()->SetPointSize(size);
}

void MeshGroup::SetGroupColor(float r, float g, float b) {
    if (m_GroupActor == nullptr) {
        return;
    }
    m_GroupActor->GetProperty()->SetColor(
        static_cast<double>(r), static_cast<double>(g), static_cast<double>(b));
}