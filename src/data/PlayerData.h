#pragma once

#include <unordered_map>
#include <unordered_set>
#include <array>
#include <memory>

class StageData;

/*
 * PlayerData
 * ----------
 * Stores all persistent player-related data:
 * - Owned units and their levels
 * - Equipped units for stages
 * - Currency (shells)
 * - Completed stages
 *
 * Provides methods for unit management, currency management, and progression tracking.
 */

class PlayerData
{
public:

    // ===== Units =====

    // Unlocks a unit from the "waiting to unlock" list
    void unlock_unit(int unitUid); 

    // Adds a unit to the waiting list (to be unlocked later)
    void wait_to_be_unlocked(int unitUid);   

    // Increases the level of an owned unit
    void upgrade_unit(int unitUid);         

    // Equips a unit to a specific slot (0-9)
    void equip_unit(int unitUid, int slot);    

    // Removes a unit from a slot
    void unequip_unit(int slot);                   
    
    // Checks if the player owns the unit
    bool is_unit_owned(int unitUid) const;

    // Returns the level of the unit, 0 if not owned
    int get_unit_level(int unitUid) const;

    // Returns the form of the unit, 0 being its default form
    int get_unit_form(int unitUid) const;

    // ===== Shells (currency) =====

    // Increases the player's shell count
    void gain_shells(int amount);

    // Decreases shells if enough, returns false if insufficient
    bool spend_shells(int amount);

    // Shiny Scales (the Clam Room's currency): false if there aren't enough
    bool spend_shiny_scales(int amount);

    // A fish out of a clam: a new one joins at level 1, one already owned gains a level (its form follows);
    // at the max level nothing changes (isMaxed: the caller gives the Shiny Scale back)
    struct FishReward { int uid = -1; bool isNew = false; bool isMaxed = false; int level = 0; int form = 0; };
    FishReward receive_fish(int unitUid);

    // ===== Progression =====

    // Marks a stage as completed and applies rewards
    void complete_stage(std::weak_ptr<StageData> stageData);

    // Returns true if the player has completed the given stage
    bool has_completed_stage(int stageUID);

    // Marks a chapter as completed
    void complete_chapter(int chapterUID);

    // Sets the currentChapter to the new chapter uid
    void enter_chapter(int chapterUID);

    // Returns true if the player has completed the given chapter
    bool has_completed_chapter(int chapterUID);

    // Remembers an enemy met in battle (the chapter map and the encyclopedia stop showing it in black)
    void see_enemy(int enemyUID);

    // Returns true if the player has met this enemy in battle
    bool has_seen_enemy(int enemyUID) const;

    // ===== Levels =====
    static constexpr int MAX_UNIT_LEVEL = 30;

    // Health and attack of a unit at this level, compared to level 1 (x1.15 per level)
    static float stat_multiplier(int level);

    // Shells to go from level to level + 1 (level 0: unlocking it, the base cost): x1.5 per level, x0.7 after
    // each evolution (levels 10 and 30). The Upgrade screen and FishEditor's cost estimate both use it.
    static int upgrade_cost(int baseUpgradeCost, int level);

public:
    // Currency
    int shells = 0;
    int shinyScales = 0;                   // The Clam Room's currency
    static constexpr int TEST_STARTING_SHINY_SCALES = 100;   // A save without Shiny Scales starts with these (testing)

    // Units
    struct OwnedUnit 
    {
        int level = 1;
        int form = 0;
    };

    std::unordered_map<int, OwnedUnit> ownedUnits;      // Key: unit UID, Value: unit infos
    std::unordered_set<int> unitsWaitingToBeUnlocked; // Units awaiting unlock in the upgrade menu
    std::array<int, 10> equippedUnits;            // Equipped units in slots (max 10). -1 = empty slot

    // Progression
    std::unordered_set<int> completedStages;      // Set of stage UIDs completed by the player
    std::unordered_set<int> completedChapters;      // Set of chapter UIDs completed by the player
    int currentChapter = 1;                         // Current chapter selected by the player
    int lastStage = -1;                             // Stage the map opens on: the last one played
                                                    // (-1 after a first win: the newest stage)
    std::unordered_set<int> seenEnemies;            // Enemy UIDs met in battle
};
