#pragma once

#include "UIUnitCard.h"
#include "../../data/PlayerData.h"

/*
 * UIUpgradeIcon
 * -------------
 * A unit's card in the upgrade carousel (UIUnitCard): "LEVEL n" and its extra levels from the Clam Room ("+3",
 * purple) (or LOCKED, as a black silhouette),
 * the price of the next level in shells (or MAX), and "IN USE" when the unit is in the lineup.
 * The same card shows a base upgrade (its own max level) or a category (BASE, RARE...: a caption, a gold slot).
 */

class UIUpgradeIcon : public UIUnitCard
{
public:

    static constexpr int MAX_LEVEL = PlayerData::MAX_UNIT_LEVEL;

    UIUpgradeIcon(const std::string& iconTexturePath);

    int get_uid() const;

    void set_unit_level(int level);
    void set_extra_levels(int extraLevels);   // The Clam Room's "+3" after the level
    void set_upgrade_cost(int cost);
    void set_affordable(bool isAffordable);   // Dims the price when the player doesn't have enough shells
    void set_in_lineup(bool isInLineup);
    void set_max_level(int maxLevel);                  // A base upgrade's (PlayerData::BASE_UPGRADE_MAX_LEVEL)
    void set_caption(const std::string& caption);      // A category card: this line instead of a level and a price

    void update(float deltaTime) override;

private:

    int m_uid = -1;            // Unique ID of the unit
    int m_upgradeCost = 100;   // Cost to upgrade the unit
    int m_currentLevel = 1;    // Current unit level (0: not unlocked yet)
    int m_extraLevels = 0;     // From the Clam Room
    bool m_isAffordable = true;
    int m_maxLevel = MAX_LEVEL;
    std::string m_caption;
};
