#include "cell_manager.h"
#include "../../config/log_config.h"
#include "../atoms_template.h"
#include "atom_manager.h"


CellInfo cellInfo;
std::vector<vtkSmartPointer<vtkActor>> cellEdgeActors;
bool cellVisible = false;

namespace atoms::domain {

void setCellVisible(bool visible) {
    cellVisible = visible;
}

bool isCellVisible() {
    return cellVisible;
}

bool hasUnitCell() {
    return cellInfo.hasCell;
}

void setCellMatrix(const float matrix[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            cellInfo.matrix[i][j] = matrix[i][j];
        }
    }
    calculateInverseMatrix(cellInfo.matrix, cellInfo.invmatrix);
}

void getCellMatrix(float out[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            out[i][j] = cellInfo.matrix[i][j];
        }
    }
}

void getCellInverse(float out[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            out[i][j] = cellInfo.invmatrix[i][j];
        }
    }
}

void setCellModified(bool modified) {
    cellInfo.modified = modified;
}

bool isCellModified() {
    return cellInfo.modified;
}

void applyCellChanges(::AtomsTemplate* parent) {
    if (!parent) {
        SPDLOG_ERROR("applyCellChanges called with null parent");
        return;
    }

    SPDLOG_DEBUG("Recalculating inverse matrix and fractional coordinates");
    calculateInverseMatrix(cellInfo.matrix, cellInfo.invmatrix);

    parent->clearUnitCell();
    parent->createUnitCell(cellInfo.matrix);

    for (auto& atom : createdAtoms) {
        cartesianToFractional(atom.position,      atom.fracPosition,     cellInfo.invmatrix);
        cartesianToFractional(atom.tempPosition,  atom.tempFracPosition, cellInfo.invmatrix);
    }

    for (auto& atom : surroundingAtoms) {
        cartesianToFractional(atom.position,      atom.fracPosition,     cellInfo.invmatrix);
        cartesianToFractional(atom.tempPosition,  atom.tempFracPosition, cellInfo.invmatrix);
    }

    if (parent->isSurroundingsVisible()) {
        SPDLOG_DEBUG("Regenerating surrounding atoms due to cell change");
        parent->hideSurroundingAtoms();
        parent->createSurroundingAtoms();
    }

    SPDLOG_INFO("Cell matrix update completed within batch");
}

void calculateInverseMatrix(const float matrix[3][3], float inverse[3][3]) {
    float det = matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1])
              - matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0])
              + matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
    
    if (std::abs(det) < 1e-6f) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                inverse[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
        SPDLOG_WARN("Matrix is singular or near-singular. Inverse calculation failed.");
        return;
    }
    
    float invDet = 1.0f / det;
    
    inverse[0][0] = invDet * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]);
    inverse[0][1] = invDet * (matrix[0][2] * matrix[2][1] - matrix[0][1] * matrix[2][2]);
    inverse[0][2] = invDet * (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]);
    
    inverse[1][0] = invDet * (matrix[1][2] * matrix[2][0] - matrix[1][0] * matrix[2][2]);
    inverse[1][1] = invDet * (matrix[0][0] * matrix[2][2] - matrix[0][2] * matrix[2][0]);
    inverse[1][2] = invDet * (matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2]);
    
    inverse[2][0] = invDet * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
    inverse[2][1] = invDet * (matrix[0][1] * matrix[2][0] - matrix[0][0] * matrix[2][1]);
    inverse[2][2] = invDet * (matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]);
}

void cartesianToFractional(const float cartesian[3], float fractional[3], const float invmatrix[3][3]) {
    for (int i = 0; i < 3; i++) {
        fractional[i] = 0.0f;
        for (int j = 0; j < 3; j++) {
            fractional[i] += cartesian[j] * invmatrix[j][i];
        }
    }
}

void fractionalToCartesian(const float fractional[3], float cartesian[3], const float matrix[3][3]) {
    for (int i = 0; i < 3; i++) {
        cartesian[i] = 0.0f;
        for (int j = 0; j < 3; j++) {
            cartesian[i] += fractional[j] * matrix[j][i];
        }
    }
}

} // namespace atoms::domain
