#pragma once

#ifdef DEBUG_MODE

#include "DebugPanel.h"

/*
 * OverlayPanel
 * ------------
 * Toggles what DebugTools draws on top of the battlefield in the game window:
 * hitboxes, attack ranges, damage zones, labels and the selection outline.
 */
class OverlayPanel : public DebugPanel
{
public:
    OverlayPanel() : DebugPanel("Overlays") {}

    void draw(DebugContext& context) override;
};

#endif
