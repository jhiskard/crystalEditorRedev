#pragma once

#include "macro/ptr_macro.h"
#include "enum/mesh_group_enum.h"

// Standard library
#include <cstdint>
#include <string>

// VTK
#include <vtkDataSet.h>

class vtkDataSetMapper;
class vtkActor;


DECLARE_PTR(MeshGroup)
class MeshGroup {
public:
    static MeshGroupUPtr New(const char* name, MeshGroupType type,
        vtkSmartPointer<vtkDataSet> dataSet);
    ~MeshGroup() = default;

    // Getters
    int32_t GetId() const { return m_Id; }
    const char* GetName() const { return m_Name.c_str(); }
    MeshGroupType GetGroupType() const { return m_Type; }
    static const char* GetMeshGroupTypeStr(MeshGroupType type);
    vtkSmartPointer<vtkActor> GetGroupActor() const { return m_GroupActor; }
    bool GetVisibility() const;
    const float GetPointSize() const;
    const double* GetGroupColor() const;

    // Setters
    void SetName(const char* name) { m_Name = name; }
    void SetVisibility(bool visibility);
    void SetPointSize(float size);
    void SetGroupColor(float r, float g, float b);

    static int32_t GenerateNewId() { return ++m_LastId; }

private:
    int32_t m_Id { -1 };
    std::string m_Name;
    MeshGroupType m_Type { MeshGroupType::INVALID };

    vtkSmartPointer<vtkDataSet> m_DataSet;
    vtkSmartPointer<vtkDataSetMapper> m_GroupMapper;
    vtkSmartPointer<vtkActor> m_GroupActor;

    static int32_t m_LastId;

    MeshGroup() = default;

    MeshGroup(const MeshGroup&) = delete;
    MeshGroup& operator=(const MeshGroup&) = delete;
    MeshGroup(MeshGroup&&) = delete;
    MeshGroup& operator=(MeshGroup&&) = delete;

    void createGroupMapper();
    void createGroupActor();
};