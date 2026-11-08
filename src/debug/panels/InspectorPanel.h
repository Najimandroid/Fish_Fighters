#pragma once

#ifdef DEBUG_MODE

#include "DebugPanel.h"

#include "../DebugWidgets.h"
#include "../../core/entities/status/StatusEffect.h"

/*
 * InspectorPanel
 * --------------
 * Every entity on the field (ID, state, HP, damage per hit). Selecting one shows everything about it
 * (health, knockback threshold, damage breakdown, cooldown, position, types, abilities, statuses)
 * and lets you kill it, give it a status or clear its statuses.
 * The selection is shared with the overlays (outline) and the Combat Log filter.
 */
class InspectorPanel : public DebugPanel
{
public:
    InspectorPanel() : DebugPanel("Inspector") {}

    void draw(DebugContext& context) override;

private:
    void draw_details(DebugContext& context, const DebugEntityRow& row);

private:
    // ----- Status to give to the selected entity -----
    StatusType m_statusToApply = StatusType::WEAKEN;
    float m_statusValue = 0.5f;
    float m_statusDuration = 3.f;
};

#endif
