#include "PlayerData.h"
#include "../core/DataLoader.h"
#include <cmath>
#include <iostream>

/*
 * Unlocks a unit that was waiting to be unlocked.
 * Adds it to ownedUnits at level 1 and removes from the waiting list.
 */
void PlayerData::unlock_unit(int unitUid)
{
    if (unitsWaitingToBeUnlocked.find(unitUid) != unitsWaitingToBeUnlocked.end())
    {
        unitsWaitingToBeUnlocked.erase(unitUid);
        ownedUnits[unitUid] = { 1, 0 };
    }
}

bool PlayerData::spend_shiny_scales(int amount)
{
    if (amount < 0 || shinyScales < amount) return false;
    shinyScales -= amount;
    return true;
}

PlayerData::FishReward PlayerData::receive_fish(int unitUid)
{
    FishReward reward;
    reward.uid = unitUid;
    auto owned = ownedUnits.find(unitUid);
    if (owned == ownedUnits.end())
    {
        unitsWaitingToBeUnlocked.erase(unitUid);
        ownedUnits[unitUid] = { 1, 0 };
        reward.isNew = true;
    }
    else if (owned->second.level >= MAX_UNIT_LEVEL)
    {
        reward.isMaxed = true;
    }
    else
    {
        upgrade_unit(unitUid);
    }
    reward.level = get_unit_level(unitUid);
    reward.form = get_unit_form(unitUid);
    return reward;
}

/*
 * Adds a unit UID to the list of units waiting to be unlocked in the upgrade menu.
 */
void PlayerData::wait_to_be_unlocked(int unitUid)
{
    unitsWaitingToBeUnlocked.insert(unitUid);
}

/*
 * Upgrades an owned unit by incrementing its level.
 * Updates its form if evolving
 */
void PlayerData::upgrade_unit(int unitUid)
{
    auto it = ownedUnits.find(unitUid);
    if (it == ownedUnits.end()) return;

	// Checks if max level reached
    if (it->second.level >= 30) return;

    // Gain a level
    it->second.level += 1;

    // Evolution check
    int lvl = it->second.level;
    if (lvl >= 31) // TODO: Lower it to 30 and add a third form for each unit
        it->second.form = 2;
    else if (lvl >= 10)
        it->second.form = 1;
    else
        it->second.form = 0;
}

/*
 * Equip a unit in a specific slot (0-9). Only works if the unit is owned.
 */
void PlayerData::equip_unit(int unitUid, int slot)
{
    if (slot < 0 || slot > 9) return;
    if (ownedUnits.find(unitUid) != ownedUnits.end())
        equippedUnits[slot] = unitUid;
}

/*
 * Unequip a unit from a slot (0-9).
 */
void PlayerData::unequip_unit(int slot)
{
    if (slot < 0 || slot > 9) return;
    equippedUnits[slot] = -1;
}

/*
 * Checks if a unit is owned by the player.
 */
bool PlayerData::is_unit_owned(int unitUid) const
{
    return ownedUnits.find(unitUid) != ownedUnits.end();
}

/*
 * Returns the level of an owned unit. Returns 0 if the player does not own the unit.
 */
int PlayerData::get_unit_level(int unitUid) const
{
    auto it = ownedUnits.find(unitUid);
    return (it != ownedUnits.end()) ? it->second.level : 0;
}

int PlayerData::get_unit_form(int unitUid) const
{
    auto it = ownedUnits.find(unitUid);
    return (it != ownedUnits.end()) ? it->second.form : 0;
}

/*
 * Adds shells to the player's total.
 */
void PlayerData::gain_shells(int amount)
{
    shells += amount;
}

/*
 * Attempts to spend shells. Returns true if successful, false if insufficient shells.
 */
bool PlayerData::spend_shells(int amount)
{
    if (shells >= amount)
    {
        shells -= amount;
        return true;
    }
    return false;
}

/*
 * Marks a stage as completed.
 * If already completed, only grants shell rewards.
 * Otherwise, applies all rewards (shells, units, etc.).
 */
void PlayerData::complete_stage(std::weak_ptr<StageData> stageData)
{
    if (auto stage = stageData.lock())
    {
        if (completedStages.find(stage->UID) != completedStages.end())
        {
            // Stage already completed: only grant shells
            for (const auto& reward : stage->rewards)
            {
                if (reward->type == "shells")
                {
                    gain_shells(reward->amount);
                    break;
                }
            }
            return;
        }

        // Mark stage as completed
        completedStages.insert(stage->UID);

        // Apply rewards
        for (const auto& reward : stage->rewards)
        {
            if (reward->type == "shells")
            {
                gain_shells(reward->amount);
            }
            else if (reward->type == "unit" && reward->unitUID > 0)
            {
                if (!is_unit_owned(reward->unitUID))
                    wait_to_be_unlocked(reward->unitUID);
            }
        }
    }
    else
    {
        std::cerr << "Issue while completing the stage." << std::endl;
    }
}

/*
 * Returns true if the player has completed the given stage.
 */
bool PlayerData::has_completed_stage(int stageUID)
{
    return completedStages.find(stageUID) != completedStages.end();
}

/*
 * Marks a chapter as completed.
 * If already completed, does nothing.
 */
void PlayerData::complete_chapter(int chapterUID)
{
    if (completedChapters.find(chapterUID) != completedChapters.end()) return;

    completedChapters.insert(chapterUID);
}

/*
 * Sets the currentChapter to the new chapter uid.
 */
void PlayerData::enter_chapter(int chapterUID)
{
    currentChapter = chapterUID;
}

/*
 * Returns true if the player has completed the given chapter.
 */
bool PlayerData::has_completed_chapter(int chapterUID)
{
    return completedChapters.find(chapterUID) != completedChapters.end();
}

/*
 * Remembers an enemy met in battle
 */
void PlayerData::see_enemy(int enemyUID)
{
    seenEnemies.insert(enemyUID);
}

/*
 * Returns true if the player has met this enemy in battle
 */
bool PlayerData::has_seen_enemy(int enemyUID) const
{
    return seenEnemies.find(enemyUID) != seenEnemies.end();
}

/*
 * Health and attack of a unit at this level, compared to level 1: +15 % per level, compounded
 */
int PlayerData::upgrade_cost(int baseUpgradeCost, int level)
{
    if (level <= 0) return baseUpgradeCost;

    const float factor = 1.5f;              // Level-up multiplier
    const float evolutionDiscount = 0.7f;   // Discount after each evolution
    int evolutions = 0;
    for (int evolutionLevel : { 10, 30 })
        if (level >= evolutionLevel) evolutions++;

    const float cost = baseUpgradeCost * std::pow(factor, static_cast<float>(level)) * std::pow(evolutionDiscount, static_cast<float>(evolutions));
    return static_cast<int>(cost);
}

float PlayerData::stat_multiplier(int level)
{
    const float growth = 0.15f;
    return level > 1 ? std::pow(1.f + growth, static_cast<float>(level - 1)) : 1.f;
}
