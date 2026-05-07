#pragma once

namespace features::data::slice {

class SliceController;

class SliceUI {
public:
    explicit SliceUI(SliceController& controller);
    void Render(bool* open);

private:
    void SyncFromController();

    SliceController& controller_;
    char filePathInput_[256] = "CHGCAR";
    int planeIndex_ = 0;
    float position_ = 0.5f;
    int millerH_ = 1;
    int millerK_ = 1;
    int millerL_ = 1;
    bool hadData_ = false;
};

} // namespace features::data::slice
