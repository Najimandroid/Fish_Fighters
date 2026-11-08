#pragma once

#include "EditorPanel.h"

/*
 * ValidationPanel
 * ---------------
 * The Validator's report: does the game load the data, and every error and warning.
 * Clicking an issue selects what it's about.
 */
class ValidationPanel : public EditorPanel
{
public:
    const char* get_name() const override { return "Validation"; }
    void draw(EditorContext& ctx) override;

    bool isCheckRequested = false;   // "Check now" was clicked (the EditorApp runs the Validator)

private:
    bool m_isShowingWarnings = true;
};
