#include "bond_renderer.h"

#include "vtk_renderer.h"
#include "../domain/bond_manager.h"

namespace atoms {
namespace infrastructure {

BondRenderer::BondRenderer(VTKRenderer* renderer)
    : m_vtkRenderer(renderer) {}

void BondRenderer::setRenderer(VTKRenderer* renderer) {
    m_vtkRenderer = renderer;
}

void BondRenderer::initializeBondGroup(const std::string& key, float radius) {
    if (!m_vtkRenderer) {
        return;
    }
    m_vtkRenderer->initializeBondGroup(key, radius);
}

void BondRenderer::updateBondGroup(const std::string& key, const atoms::domain::BondGroupInfo& group) {
    if (!m_vtkRenderer) {
        return;
    }
    m_vtkRenderer->updateBondGroup(key,
                                   group.transforms1,
                                   group.transforms2,
                                   group.colors1,
                                   group.colors2);
}

void BondRenderer::clearAllBondGroups() {
    if (!m_vtkRenderer) {
        return;
    }
    m_vtkRenderer->clearAllBondGroups();
}

} // namespace infrastructure
} // namespace atoms
