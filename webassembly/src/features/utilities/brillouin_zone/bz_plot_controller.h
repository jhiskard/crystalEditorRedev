#pragma once

#include "bz_plot.h"
#include "bz_plot_layer.h"

#include "core/scene/scene_state.h"

#include <map>
#include <vtkSmartPointer.h>

class vtkActor;
class vtkActor2D;

namespace features::utilities::bz {

class BZPlotController {
public:
    explicit BZPlotController(core::scene::SceneState& scene);

    bool Show(
        const std::string& path,
        int npoints,
        bool showVectors,
        bool showLabels,
        std::string& outErrorMessage);

    void Clear();

    bool IsShowing() const { return showing_; }

    bool TryGetCurrentCellInfo(CellInfo& out) const;

    void SetCellInfoForTesting(const CellInfo& cell);

private:
    bool ResolveCellInfo(const std::string& path, CellInfo& outCell) const;

    bool RenderCompleteBZPlot(
        const BZVerticesResult& bzData,
        const double cell[3][3],
        const double icell[3][3],
        bool showVectors,
        bool showLabels,
        const std::string& path,
        int npoints);

    void RenderIBZLines(const BZVerticesResult& bzData);
    void RenderReciprocalVectors(const double icell[3][3]);
    void RenderBandpath(const std::vector<std::array<double, 3>>& kpoints);
    void RenderKpoints(const std::vector<std::array<double, 3>>& kpoints, double radius);
    void RenderSpecialPointLabels(const std::map<std::string, std::array<double, 3>>& specialPoints);

    static vtkSmartPointer<vtkActor> CreateLineActor(
        const double p1[3],
        const double p2[3],
        const double color[3],
        double width);

    static vtkSmartPointer<vtkActor> CreateArrowActor(
        const double start[3],
        const double end[3],
        const double color[3],
        double shaftRadius);

    static vtkSmartPointer<vtkActor2D> CreateTextActor2D(
        const std::string& text,
        const double position[3],
        int fontSize,
        const double color[3]);

    static double CalculateMaxReciprocalVectorLength(const double icell[3][3]);

    core::scene::SceneState& scene_;
    BZPlotLayer layer_;
    BZVerticesResult lastResult_;
    CellInfo currentCell_;
    bool hasCurrentCell_ = false;
    bool showing_ = false;
};

} // namespace features::utilities::bz
