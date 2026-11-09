#pragma once

#ifdef DEBUG_MODE

#include "DebugPanel.h"

/*
 * StagePanel
 * ----------
 * Menus: load any defined stage, add shells.
 * Battle: exit the stage, win or lose instantly, set the battle cash, see both bases' health.
 */
class StagePanel : public DebugPanel
{
public:
    StagePanel() : DebugPanel("Stage") {}

    void draw(DebugContext& context) override;

private:
    void draw_menus(DebugContext& context);
    void draw_battle(DebugContext& context);

private:
    int m_selectedStageUid = -1; // DEBUG stage by default
    int m_shellAmount = 1000;
};

#endif
