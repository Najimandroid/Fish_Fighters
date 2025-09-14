#include "PlayerData.h"
#include "../core/DataLoader.h"
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

    // Gain a level
    it->second.level += 1;

    // Evolution check
    int lvl = it->second.level;
    if (lvl >= 30)
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
