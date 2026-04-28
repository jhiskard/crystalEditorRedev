#include "test_window.h"
#include "vtk_viewer.h"
#include "font_manager.h"
#include "config/log_config.h"

// VTK
#include <vtkConeSource.h>
#include <vtkSphereSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkCellData.h>


TestWindow::TestWindow() {
}

TestWindow::~TestWindow() {
}

void TestWindow::Render(bool* openWindow) {
    ImGui::Begin("Test-Window", openWindow, ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("This is a test window to test the VTK viewer.");

    static float imguiPosition[3] = { 0.0, 0.0, 0.0 };
    ImGui::DragFloat3("Position", imguiPosition);
    ImGui::Separator();

    if (ImGui::Button("Create Cone")) {
        // Create a cone
        vtkNew<vtkConeSource> coneSource;
        coneSource->SetHeight(3.0);
        coneSource->SetRadius(1.0);
        coneSource->SetResolution(100);
        coneSource->SetCenter((double)imguiPosition[0], (double)imguiPosition[1], (double)imguiPosition[2]);
        coneSource->SetDirection(0.0, 1.0, 0.0);
        coneSource->Update();

        // Create colors and initialize to gray
        vtkNew<vtkUnsignedCharArray> colors;
        colors->SetNumberOfComponents(3);
        colors->SetName("Colors");

        vtkPolyData* polyData = coneSource->GetOutput();
        for (vtkIdType i = 0; i < polyData->GetNumberOfCells(); i++)
        {
            double baseColor[3] = {225.0, 225.0, 225.0};
            colors->InsertNextTuple3(baseColor[0], baseColor[1], baseColor[2]);
        }
        
        // Set colors to polyData
        polyData->GetCellData()->SetScalars(colors);

        // Create a mapper
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(coneSource->GetOutputPort());

        // Create an actor
        vtkNew<vtkActor> actor;
        actor->SetMapper(mapper);
        actor->GetProperty()->SetEdgeVisibility(true);
        actor->GetProperty()->SetEdgeColor(0, 0, 0);

        // Add the actor to the scene
        VtkViewer::Instance().AddActor(actor);
    }

    if (ImGui::Button("Sphere Cone")) {
        // Create a cone
        vtkNew<vtkSphereSource> sphereSource;
        sphereSource->SetRadius(1.0);
        sphereSource->SetThetaResolution(24);
        sphereSource->SetCenter((double)imguiPosition[0], (double)imguiPosition[1], (double)imguiPosition[2]);
        sphereSource->Update();

        // Create colors and initialize to gray
        vtkNew<vtkUnsignedCharArray> colors;
        colors->SetNumberOfComponents(3);
        colors->SetName("Colors");

        vtkPolyData* polyData = sphereSource->GetOutput();
        for (vtkIdType i = 0; i < polyData->GetNumberOfCells(); i++)
        {
            double baseColor[3] = {225.0, 225.0, 225.0};
            colors->InsertNextTuple3(baseColor[0], baseColor[1], baseColor[2]);
        }
        
        // Set colors to polyData
        polyData->GetCellData()->SetScalars(colors);

        // Create a mapper
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sphereSource->GetOutputPort());

        // Create an actor
        vtkNew<vtkActor> actor;
        actor->SetMapper(mapper);
        actor->GetProperty()->SetEdgeVisibility(true);
        actor->GetProperty()->SetEdgeColor(0, 0, 0);

        // Add the actor to the scene
        VtkViewer::Instance().AddActor(actor);
    }

    if (ImGui::Button("Memory allocation test")) {
        // This is just to test memory allocation
        // and should be removed in production code
        void* testPtr = malloc(1024 * 1024 * 1024);  // 1GB
        if (testPtr) {
            SPDLOG_INFO("Allocated 1GB of memory");
        }
        else {
            SPDLOG_ERROR("Failed to allocate memory");
        }
        m_TestPtrs.push_back(testPtr);
    }

    ImGui::End();
}