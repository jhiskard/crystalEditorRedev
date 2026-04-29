/**
 * @file features/utilities/brillouin_zone/bz_plot_layer.cpp
 * @brief VTK actor grouping layer for Brillouin Zone visualization.
 */
#include "bz_plot_layer.h"

#include "core/vtk/vtk_viewer.h"

namespace features::utilities::bz {

void BZPlotLayer::ActorGroup::setVisibility(bool vis) {
    visible = vis;
    for (auto& actor : actors) {
        if (actor) {
            actor->SetVisibility(vis ? 1 : 0);
        }
    }
}

void BZPlotLayer::ActorGroup::add(vtkSmartPointer<vtkActor> actor) {
    if (!actor) {
        return;
    }

    core::vtk::VtkViewer::Instance().AddActor(actor);
    actor->SetVisibility(visible ? 1 : 0);
    actors.push_back(std::move(actor));
}

void BZPlotLayer::ActorGroup::clear() {
    for (auto& actor : actors) {
        if (actor) {
            core::vtk::VtkViewer::Instance().RemoveActor(actor);
        }
    }

    actors.clear();
    visible = true;
}

void BZPlotLayer::Actor2DGroup::setVisibility(bool vis) {
    visible = vis;
    for (auto& actor : actors) {
        if (actor) {
            actor->SetVisibility(vis ? 1 : 0);
        }
    }
}

void BZPlotLayer::Actor2DGroup::add(vtkSmartPointer<vtkActor2D> actor) {
    if (!actor) {
        return;
    }

    core::vtk::VtkViewer::Instance().AddActor2D(actor);
    actor->SetVisibility(visible ? 1 : 0);
    actors.push_back(std::move(actor));
}

void BZPlotLayer::Actor2DGroup::clear() {
    for (auto& actor : actors) {
        if (actor) {
            core::vtk::VtkViewer::Instance().RemoveActor2D(actor);
        }
    }

    actors.clear();
    visible = true;
}

BZPlotLayer::~BZPlotLayer() {
    clear();
}

void BZPlotLayer::show() {
    m_isVisible = true;

    m_ibzLines.setVisibility(m_ibzLines.visible);
    m_reciprocalVectors.setVisibility(m_reciprocalVectors.visible);
    m_bandpaths.setVisibility(m_bandpaths.visible);
    m_kpoints.setVisibility(m_kpoints.visible);
    m_labels.setVisibility(m_labels.visible);
}

void BZPlotLayer::hide() {
    m_isVisible = false;

    for (auto& actor : m_ibzLines.actors) {
        if (actor) {
            actor->SetVisibility(0);
        }
    }
    for (auto& actor : m_reciprocalVectors.actors) {
        if (actor) {
            actor->SetVisibility(0);
        }
    }
    for (auto& actor : m_bandpaths.actors) {
        if (actor) {
            actor->SetVisibility(0);
        }
    }
    for (auto& actor : m_kpoints.actors) {
        if (actor) {
            actor->SetVisibility(0);
        }
    }
    for (auto& actor : m_labels.actors) {
        if (actor) {
            actor->SetVisibility(0);
        }
    }
}

void BZPlotLayer::setIBZLinesVisible(bool visible) {
    m_ibzLines.visible = visible;
    if (m_isVisible) {
        m_ibzLines.setVisibility(visible);
    }
}

void BZPlotLayer::setReciprocalVectorsVisible(bool visible) {
    m_reciprocalVectors.visible = visible;
    if (m_isVisible) {
        m_reciprocalVectors.setVisibility(visible);
    }
}

void BZPlotLayer::setBandpathVisible(bool visible) {
    m_bandpaths.visible = visible;
    if (m_isVisible) {
        m_bandpaths.setVisibility(visible);
    }
}

void BZPlotLayer::setKpointsVisible(bool visible) {
    m_kpoints.visible = visible;
    if (m_isVisible) {
        m_kpoints.setVisibility(visible);
    }
}

void BZPlotLayer::setLabelsVisible(bool visible) {
    m_labels.visible = visible;
    if (m_isVisible) {
        m_labels.setVisibility(visible);
    }
}

void BZPlotLayer::addIBZLineActor(vtkSmartPointer<vtkActor> actor) {
    m_ibzLines.add(std::move(actor));
    if (!m_isVisible) {
        hide();
    }
}

void BZPlotLayer::addReciprocalVectorActor(vtkSmartPointer<vtkActor> actor) {
    m_reciprocalVectors.add(std::move(actor));
    if (!m_isVisible) {
        hide();
    }
}

void BZPlotLayer::addBandpathActor(vtkSmartPointer<vtkActor> actor) {
    m_bandpaths.add(std::move(actor));
    if (!m_isVisible) {
        hide();
    }
}

void BZPlotLayer::addKpointActor(vtkSmartPointer<vtkActor> actor) {
    m_kpoints.add(std::move(actor));
    if (!m_isVisible) {
        hide();
    }
}

void BZPlotLayer::addLabelActor(vtkSmartPointer<vtkActor2D> actor) {
    m_labels.add(std::move(actor));
    if (!m_isVisible) {
        hide();
    }
}

void BZPlotLayer::clear() {
    m_ibzLines.clear();
    m_reciprocalVectors.clear();
    m_bandpaths.clear();
    m_kpoints.clear();
    m_labels.clear();

    m_isVisible = false;
}

size_t BZPlotLayer::getTotalActorCount() const {
    return m_ibzLines.size() +
           m_reciprocalVectors.size() +
           m_bandpaths.size() +
           m_kpoints.size() +
           m_labels.size();
}

} // namespace features::utilities::bz
