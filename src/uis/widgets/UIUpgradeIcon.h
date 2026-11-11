#pragma once

#include "UIUnitCard.h"
#include "../../data/PlayerData.h"

/*
 * UIUpgradeIcon
 * -------------
 * A unit's card in the upgrade carousel (UIUnitCard): "LEVEL n" (or LOCKED, as a black silhouette),
 * the price of the next level in shells (or MAX), and "IN USE" when the unit is in the lineup.
 */

class UIUpgradeIcon : public UIUnitCard
{
public:

    static constexpr int MAX_LEVEL = PlayerData::MAX_UNIT_LEVEL;

    UIUpgradeIcon(const std::string& iconTexturePath);

    int get_uid() const;

    void set_unit_level(int level);
    void set_upgrade_cost(int cost);
    void set_affordable(bool isAffordable);   // Dims the price when the player doesn't have enough shells
    void set_in_lineup(bool isInLineup);

    void update(float deltaTime) override;

private:

    int m_uid = -1;            // Unique ID of the unit
    int m_upgradeCost = 100;   // Cost to upgrade the unit
    int m_currentLevel = 1;    // Current unit level (0: not unlocked yet)
    bool m_isAffordable = true;
};
