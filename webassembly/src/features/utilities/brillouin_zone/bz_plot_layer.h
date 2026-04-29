/**
 * @file features/utilities/brillouin_zone/bz_plot_layer.h
 * @brief VTK actor grouping layer for Brillouin Zone visualization.
 */
#pragma once

#include <vector>

#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkSmartPointer.h>

namespace features::utilities::bz {

class BZPlotLayer {
public:
    struct ActorGroup {
        std::vector<vtkSmartPointer<vtkActor>> actors;
        bool visible = true;

        void setVisibility(bool vis);
        void add(vtkSmartPointer<vtkActor> actor);
        void clear();
        size_t size() const { return actors.size(); }
    };

    struct Actor2DGroup {
        std::vector<vtkSmartPointer<vtkActor2D>> actors;
        bool visible = true;

        void setVisibility(bool vis);
        void add(vtkSmartPointer<vtkActor2D> actor);
        void clear();
        size_t size() const { return actors.size(); }
    };

private:
    ActorGroup m_ibzLines;
    ActorGroup m_reciprocalVectors;
    ActorGroup m_bandpaths;
    ActorGroup m_kpoints;
    Actor2DGroup m_labels;
    bool m_isVisible = false;

public:
    BZPlotLayer() = default;
    ~BZPlotLayer();

    void show();
    void hide();
    bool isVisible() const { return m_isVisible; }

    void setIBZLinesVisible(bool visible);
    void setReciprocalVectorsVisible(bool visible);
    void setBandpathVisible(bool visible);
    void setKpointsVisible(bool visible);
    void setLabelsVisible(bool visible);

    bool isIBZLinesVisible() const { return m_ibzLines.visible; }
    bool isReciprocalVectorsVisible() const { return m_reciprocalVectors.visible; }
    bool isBandpathVisible() const { return m_bandpaths.visible; }
    bool isKpointsVisible() const { return m_kpoints.visible; }
    bool isLabelsVisible() const { return m_labels.visible; }

    void addIBZLineActor(vtkSmartPointer<vtkActor> actor);
    void addReciprocalVectorActor(vtkSmartPointer<vtkActor> actor);
    void addBandpathActor(vtkSmartPointer<vtkActor> actor);
    void addKpointActor(vtkSmartPointer<vtkActor> actor);
    void addLabelActor(vtkSmartPointer<vtkActor2D> actor);

    void clear();

    size_t getTotalActorCount() const;
    size_t getIBZLineCount() const { return m_ibzLines.size(); }
    size_t getReciprocalVectorCount() const { return m_reciprocalVectors.size(); }
    size_t getBandpathCount() const { return m_bandpaths.size(); }
    size_t getKpointCount() const { return m_kpoints.size(); }
    size_t getLabelCount() const { return m_labels.size(); }
};

} // namespace features::utilities::bz
