#pragma once

#include <string>

namespace atoms {
namespace domain {
struct BondGroupInfo;
} // namespace domain

namespace infrastructure {

class VTKRenderer;

/**
 * @brief Bond rendering facade for VTK-based bond group updates.
 *
 * CP2 skeleton only: keeps existing rendering path intact.
 */
class BondRenderer {
public:
    explicit BondRenderer(VTKRenderer* renderer);

    void setRenderer(VTKRenderer* renderer);

    void initializeBondGroup(const std::string& key, float radius);
    void updateBondGroup(const std::string& key, const atoms::domain::BondGroupInfo& group);
    void clearAllBondGroups();

private:
    VTKRenderer* m_vtkRenderer = nullptr;
};

} // namespace infrastructure
} // namespace atoms
