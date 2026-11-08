#pragma once

#ifdef DEBUG_MODE

#include "DebugPanel.h"

#include "../../data/AbilityData.h"
#include "../../data/EntityType.h"

#include <utility>
#include <vector>

/*
 * SpawnerPanel
 * ------------
 * Spawn any defined unit (and form) or enemy, after editing its types and abilities.
 * Edits are made on a copy of its EntityData: the data loaded from the JSON never changes.
 */
class SpawnerPanel : public DebugPanel
{
public:
    SpawnerPanel() : DebugPanel("Spawner") {}

    void draw(DebugContext& context) override;

private:
    void draw_unit_spawner(DebugContext& context);
    void draw_enemy_spawner(DebugContext& context);

private:
    // ----- Unit -----
    std::pair<int, int> m_selectedUnit = { -1, -1 }; // {UID, form}
    std::pair<int, int> m_editedUnit = { -1, -1 };   // Unit the editor was filled from
    std::vector<EntityType> m_unitTypes;
    std::vector<AbilityData> m_unitAbilities;
    bool m_isFreeUnitSpawn = true;

    // ----- Enemy -----
    int m_selectedEnemy = -1;                        // UID
    int m_editedEnemy = -1;                          // Enemy the editor was filled from
    std::vector<EntityType> m_enemyTypes;
    std::vector<AbilityData> m_enemyAbilities;
    float m_hpMultiplier = 1.f;
    float m_attackMultiplier = 1.f;
    int m_enemyLayer = -1;                           // -1 = random
    bool m_isBoss = false;
    bool m_isBypassingEnemyLimit = false;
};

#endif
