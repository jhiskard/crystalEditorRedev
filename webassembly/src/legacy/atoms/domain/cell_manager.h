#pragma once

#include <vector>
#include <vtkSmartPointer.h>
#include <vtkActor.h>

class AtomsTemplate;


struct CellInfo {
    float matrix[3][3];    // 3x3 matrix for lattice vectors
    float invmatrix[3][3]; // 3x3 inverse matrix for converting cartesian to fractional coordinates
    bool modified;         // Flag to track modifications
    bool hasCell;          // Flag to track whether a unit cell exists
    
    CellInfo() {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                matrix[i][j] = (i == j) ? 1.0f : 0.0f;
                invmatrix[i][j] = (i == j) ? 1.0f : 0.0f; // Identity matrix is its own inverse
            }
        }
        modified = false;
        hasCell = false;
    }
};

extern CellInfo cellInfo;
extern std::vector<vtkSmartPointer<vtkActor>> cellEdgeActors;
extern bool cellVisible;

namespace atoms::domain {
void setCellVisible(bool visible);
bool isCellVisible();
bool hasUnitCell();
void setCellMatrix(const float matrix[3][3]);
void getCellMatrix(float out[3][3]);
void getCellInverse(float out[3][3]);
void setCellModified(bool modified);
bool isCellModified();
void applyCellChanges(::AtomsTemplate* parent);
void calculateInverseMatrix(const float matrix[3][3], float inverse[3][3]);
void cartesianToFractional(const float cartesian[3], float fractional[3], const float invmatrix[3][3]);
void fractionalToCartesian(const float fractional[3], float cartesian[3], const float matrix[3][3]);
} // namespace atoms::domain
