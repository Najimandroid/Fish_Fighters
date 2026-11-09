#pragma once

#ifdef DEBUG_MODE

#include "../DebugContext.h"

/*
 * DebugPanel
 * ----------
 * Base class of every panel in the developer tools window.
 *
 * DebugTools opens a dockable ImGui window named get_name() for each open panel,
 * then calls draw() to fill it. Closing the window sets isOpen to false; the
 * Panels menu opens it again.
 */
class DebugPanel
{
public:
    explicit DebugPanel(const char* name) : m_name(name) {}
    virtual ~DebugPanel() = default;

    // Fills the panel's window (called every frame while the panel is open)
    virtual void draw(DebugContext& context) = 0;

    const char* get_name() const { return m_name; }

public:
    bool isOpen = true;

private:
    const char* m_name; // Also the ImGui window name, used by the dock layout
};

#endif
