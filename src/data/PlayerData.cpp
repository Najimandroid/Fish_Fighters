#include "PlayerData.h"
#include "../core/DataLoader.h"
#include <algorithm>
#include <cmath>
#include <ctime>
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
    reward.isNew = !is_unit_owned(unitUid) && get_stored_count(unitUid) == 0;
    clamStorage[unitUid] += 1;
    return reward;
}

int PlayerData::get_stored_count(int unitUid) const
{
    auto it = clamStorage.find(unitUid);
    return it != clamStorage.end() ? it->second : 0;
}

PlayerData::StoredUse PlayerData::preview_use(int unitUid, int count) const
{
    StoredUse use;
    count = std::clamp(count, 0, get_stored_count(unitUid));
    if (count <= 0) return use;

    int room = MAX_EXTRA_LEVELS - get_unit_extra_levels(unitUid);
    if (!is_unit_owned(unitUid))
    {
        use.unlocks = true;   // The first one joins the tank (level 1, no extra level yet)
        use.usable = 1;
        room = MAX_EXTRA_LEVELS;
        count -= 1;
    }
    use.extraLevels = std::min(count, std::max(0, room));
    use.usable += use.extraLevels;
    return use;
}

int PlayerData::use_stored_fish(int unitUid, int count)
{
    const StoredUse use = preview_use(unitUid, count);
    if (use.usable <= 0) return 0;
    if (use.unlocks)
    {
        unitsWaitingToBeUnlocked.erase(unitUid);
        ownedUnits[unitUid] = { 1, 0 };
    }
    ownedUnits[unitUid].extraLevels = std::min(MAX_EXTRA_LEVELS, ownedUnits[unitUid].extraLevels + use.extraLevels);
    if ((clamStorage[unitUid] -= use.usable) <= 0) clamStorage.erase(unitUid);
    return use.usable;
}

int PlayerData::sell_stored_fish_for_shells(int unitUid, int count, int shellsEach)
{
    count = std::clamp(count, 0, get_stored_count(unitUid));
    if (count <= 0) return 0;
    gain_shells(count * std::max(0, shellsEach));
    if ((clamStorage[unitUid] -= count) <= 0) clamStorage.erase(unitUid);
    return count;
}

int PlayerData::sell_stored_fish_for_scales(int unitUid, int count)
{
    count = std::clamp(count, 0, get_stored_count(unitUid));
    if (count <= 0) return 0;
    fishScales += count;
    if ((clamStorage[unitUid] -= count) <= 0) clamStorage.erase(unitUid);
    return count;
}

bool PlayerData::craft_shiny_scale()
{
    if (fishScales < FISH_SCALES_PER_SHINY) return false;
    fishScales -= FISH_SCALES_PER_SHINY;
    shinyScales += 1;
    return true;
}

int PlayerData::get_school_rank() const
{
    int power = 0;
    for (const auto& [uid, unit] : ownedUnits) power += unit.level + unit.extraLevels;
    return power;
}

PlayerData::SchoolRankMilestone PlayerData::school_rank_milestone(int index)
{
    const int listed = static_cast<int>(std::size(SCHOOL_RANK_MILESTONES));
    if (index < listed) return SCHOOL_RANK_MILESTONES[std::max(0, index)];
    return { SCHOOL_RANK_MILESTONES[listed - 1].power + SCHOOL_RANK_STEP_AFTER * (index - listed + 1), 1 };
}

int PlayerData::claim_school_rank_rewards()
{
    int given = 0;
    const int power = get_school_rank();
    while (school_rank_milestone(schoolRankRewardsClaimed).power <= power)
    {
        given += school_rank_milestone(schoolRankRewardsClaimed).shinyScales;
        schoolRankRewardsClaimed += 1;
    }
    shinyScales += given;
    return given;
}

namespace
{
    // Today, on the player's clock: YYYY-MM-DD
    std::string today()
    {
        const std::time_t now = std::time(nullptr);
        std::tm local{};
#if defined(_WIN32)
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        char text[16];
        std::strftime(text, sizeof(text), "%Y-%m-%d", &local);
        return text;
    }
}

int PlayerData::ads_left_today() const
{
    return adsDay == today() ? std::max(0, ADS_PER_DAY - adsToday) : ADS_PER_DAY;
}

bool PlayerData::watch_ad()
{
    if (ads_left_today() <= 0) return false;
    if (adsDay != today()) { adsDay = today(); adsToday = 0; }
    adsToday += 1;
    shinyScales += 1;
    return true;
}

bool PlayerData::give_clam_room_gift()
{
    if (hasClamRoomGift) return false;
    hasClamRoomGift = true;
    shinyScales += 1;
    return true;
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
 * Updates its form if evolving, into a form it has: a fish with one form stays in it (no unknown fish)
 */
void PlayerData::upgrade_unit(int unitUid, int formCount)
{
    auto it = ownedUnits.find(unitUid);
    if (it == ownedUnits.end()) return;

	// Checks if max level reached
    if (it->second.level >= MAX_UNIT_LEVEL) return;

    // Gain a level
    it->second.level += 1;

    // Evolution check
    int lvl = it->second.level;
    int form = 0;
    if (lvl >= 31) // TODO: Lower it to 30 and add a third form for each unit
        form = 2;
    else if (lvl >= 10)
        form = 1;
    it->second.form = std::min(form, std::max(1, formCount) - 1);
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

int PlayerData::get_unit_extra_levels(int unitUid) const
{
    auto it = ownedUnits.find(unitUid);
    return (it != ownedUnits.end()) ? it->second.extraLevels : 0;
}

int PlayerData::get_unit_power_level(int unitUid) const
{
    return get_unit_level(unitUid) + get_unit_extra_levels(unitUid);
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
