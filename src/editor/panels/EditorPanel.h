#pragma once

class EditorContext;

/*
 * EditorPanel
 * -----------
 * One dockable window of the editor. Panels only talk through the EditorContext.
 */
class EditorPanel
{
public:
    virtual ~EditorPanel() = default;

    virtual const char* get_name() const = 0;     // Window title, also the ImGui ID (keep it stable)
    virtual void draw(EditorContext& ctx) = 0;    // Called inside the panel's ImGui window

    bool isOpen = true;
};
