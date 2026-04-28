#pragma once

class AtomsTemplate;

namespace atoms::domain {

class SurroundingAtomManager {
public:
    explicit SurroundingAtomManager(::AtomsTemplate* parent);

    void createSurroundingAtoms();
    void hideSurroundingAtoms();
    bool isVisible() const;
    void setVisible(bool visible);

private:
    ::AtomsTemplate* m_parent = nullptr;
};

} // namespace atoms::domain
