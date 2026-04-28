#include "mouse_interactor_style.h"
#include "config/log_config.h"
#include "vtk_viewer.h"

#include <vtkObjectFactory.h>
#include <vtkRenderWindowInteractor.h>
// #include <vtkCellPicker.h>
// #include <vtkUnsignedCharArray.h>
// #include <vtkPolyData.h>
// #include <vtkActor.h>
// #include <vtkCellData.h>
// #include <vtkPolyDataMapper.h>
// #include <vtkActorCollection.h>
// #include <vtkRenderer.h>
// #include <vtkRenderWindow.h>


vtkStandardNewMacro(MouseInteractorStyle);

void MouseInteractorStyle::OnLeftButtonDown() {
    // int* clickPos = this->GetInteractor()->GetEventPosition();
    // SPDLOG_INFO("clickPos: {}, {}", clickPos[0], clickPos[1]);

    // vtkNew<vtkCellPicker> picker;
    // picker->SetTolerance(0.0);
    
    // // Pick from this location
    // picker->Pick((double)clickPos[0], (double)clickPos[1], 0.0, this->GetDefaultRenderer());

    // double* worldPosition = picker->GetPickPosition();
    // vtkIdType cellId = picker->GetCellId();

    // if (picker->GetCellId() != -1) {
    //     SPDLOG_INFO("Cell id is: {}", picker->GetCellId());
    //     SPDLOG_INFO("Position picked: {}, {}, {}", worldPosition[0], worldPosition[1], worldPosition[2]);

    //     // Change the colors of selected faces
    //     vtkActor* actor = picker->GetActor();
    //     if (actor) {
    //         vtkPolyData* polyData = vtkPolyData::SafeDownCast(actor->GetMapper()->GetInput());
    //         vtkUnsignedCharArray* colors = vtkUnsignedCharArray::SafeDownCast(
    //             polyData->GetCellData()->GetArray("Colors"));

    //         if (!colors) {
    //             SPDLOG_ERROR("No cell selected!");
    //             return;
    //         }

    //         // Change the selected face color to red
    //         unsigned char red[3] = {255, 0, 0};
    //         colors->SetTuple3(cellId, red[0], red[1], red[2]);

    //         // Update                
    //         colors->Modified();
    //         polyData->Modified();
    //     }
    // }
    // else {
    //     vtkActorCollection* actors = this->GetDefaultRenderer()->GetActors();
    //     actors->InitTraversal();
        
    //     for (int i = 0; i < actors->GetNumberOfItems(); i++) {
    //         vtkActor* actor = actors->GetNextActor();
    //         vtkPolyData* polyData = vtkPolyData::SafeDownCast(actor->GetMapper()->GetInput());
            
    //         if (polyData) {
    //             vtkUnsignedCharArray* colors = vtkUnsignedCharArray::SafeDownCast(
    //                 polyData->GetCellData()->GetArray("Colors"));
                
    //             if (colors) {
    //                 for (vtkIdType j = 0; j < polyData->GetNumberOfCells(); j++) {
    //                     double baseColor[3] = {225.0, 225.0, 225.0};
    //                     colors->SetTuple3(j, baseColor[0], baseColor[1], baseColor[2]);
    //                 }
                    
    //                 colors->Modified();
    //                 polyData->Modified();
    //             }
    //         }
    //     }
    //     SPDLOG_DEBUG("No cell selected!");
    // }
    
    // this->GetInteractor()->GetRenderWindow()->Render();
    
    // Execute callbacks for parent class
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}

void MouseInteractorStyle::OnMouseWheelForward() {
    VtkViewer& viewer = VtkViewer::Instance();
    bool applyLod = !viewer.IsInteractionLodActive();
    if (applyLod) {
        viewer.BeginInteractionLod();
        applyLod = viewer.IsInteractionLodActive();
    }

    this->Dolly(0.95);
    viewer.RequestRender();

    if (applyLod) {
        viewer.EndInteractionLod();
    }
}

void MouseInteractorStyle::OnMouseWheelBackward() {
    VtkViewer& viewer = VtkViewer::Instance();
    bool applyLod = !viewer.IsInteractionLodActive();
    if (applyLod) {
        viewer.BeginInteractionLod();
        applyLod = viewer.IsInteractionLodActive();
    }

    this->Dolly(1.05);
    viewer.RequestRender();

    if (applyLod) {
        viewer.EndInteractionLod();
    }
}
