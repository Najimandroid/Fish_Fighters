#pragma once

#include "EditorPanel.h"

/*
 * PropertiesPanel
 * ---------------
 * Edits the selected item with the editor of its kind (UnitEditor, StageEditor...).
 */
class PropertiesPanel : public EditorPanel
{
public:
    const char* get_name() const override { return "Properties"; }
    void draw(EditorContext& ctx) override;
};
