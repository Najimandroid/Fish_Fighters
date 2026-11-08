#pragma once

#ifdef DEBUG_MODE

#include "DebugPanel.h"

/*
 * CombatLogPanel
 * --------------
 * Every recent hit with the DamageCalculator's full breakdown:
 * base damage, level/stage, strong against, resistant and weaken multipliers, final damage, HP left.
 * Can be filtered to the entity selected in the Inspector.
 */
class CombatLogPanel : public DebugPanel
{
public:
    CombatLogPanel() : DebugPanel("Combat Log") {}

    void draw(DebugContext& context) override;

private:
    bool m_isOnlyShowingSelected = false;
};

#endif
