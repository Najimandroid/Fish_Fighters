#pragma once

#ifdef DEBUG_MODE

#include "DebugPanel.h"

/*
 * StatusLogPanel
 * --------------
 * Guaranteed / normal chances, every active status on the field, and the StatusManager's log
 * (one line per status, updated when it stacks; misses can be hidden).
 */
class StatusLogPanel : public DebugPanel
{
public:
    StatusLogPanel() : DebugPanel("Status Log") {}

    void draw(DebugContext& context) override;

private:
    bool m_isHidingMisses = false;
};

#endif
