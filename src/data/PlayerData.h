#pragma once

#include <unordered_map>
#include <unordered_set>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

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

    // Increases the level of an owned unit. It evolves at levels 10 and 30 into the forms it has
    // (formCount: DataLoader::get_unit_form_count; a fish with one form stays in it)
    void upgrade_unit(int unitUid, int formCount);

    // Equips a unit to a specific slot (0-9)
    void equip_unit(int unitUid, int slot);    

    // Removes a unit from a slot
    void unequip_unit(int slot);                   
    
    // Checks if the player owns the unit
    bool is_unit_owned(int unitUid) const;

    // Returns the level of the unit, 0 if not owned (the level bought with shells: prices and evolutions)
    int get_unit_level(int unitUid) const;

    // The extra levels the Clam Room gave it (the "+3" after its level)
    int get_unit_extra_levels(int unitUid) const;

    // Level + extra levels: what its health and attack grow with
    int get_unit_power_level(int unitUid) const;

    // Returns the form of the unit, 0 being its default form
    int get_unit_form(int unitUid) const;

    // ===== Shells (currency) =====

    // Increases the player's shell count
    void gain_shells(int amount);

    // Decreases shells if enough, returns false if insufficient
    bool spend_shells(int amount);

    // Shiny Scales (the Clam Room's currency): false if there aren't enough
    bool spend_shiny_scales(int amount);

    // ===== The Clam Room: Clam Storage, Fish Scales, School Rank =====

    // A fish out of a clam goes into the Clam Storage (isNew: a kind the player has neither in the tank nor stored)
    struct FishReward { int uid = -1; bool isNew = false; };
    FishReward receive_fish(int unitUid);

    // How many of a kind wait in the Clam Storage
    int get_stored_count(int unitUid) const;

    // What using stored fish would do: the first one of a kind not owned unlocks it, each other one is +1 extra
    // level up to MAX_EXTRA_LEVELS (the others can only be sold)
    struct StoredUse { int usable = 0; bool unlocks = false; int extraLevels = 0; };
    StoredUse preview_use(int unitUid, int count) const;

    // Uses up to count stored fish of a kind (preview_use says how many go): returns how many were used
    int use_stored_fish(int unitUid, int count);

    // Sells stored fish: for shells (shellsEach each) or for one Fish Scale each. Returns how many were sold
    int sell_stored_fish_for_shells(int unitUid, int count, int shellsEach);
    int sell_stored_fish_for_scales(int unitUid, int count);

    // FISH_SCALES_PER_SHINY Fish Scales make one Shiny Scale: false if there aren't enough
    static constexpr int FISH_SCALES_PER_SHINY = 10;
    bool craft_shiny_scale();

    // School Rank: every owned fish's level and extra levels added up. Its milestones give Shiny Scales (like
    // The Battle Cats' User Rank gives Rare Tickets): SCHOOL_RANK_MILESTONES, then one more every SCHOOL_RANK_STEP_AFTER
    int get_school_rank() const;
    struct SchoolRankMilestone { int power; int shinyScales; };
    static constexpr SchoolRankMilestone SCHOOL_RANK_MILESTONES[] = { { 10, 1 }, { 25, 1 }, { 50, 1 }, { 77, 7 }, { 100, 1 }, { 125, 1 } };
    static constexpr int SCHOOL_RANK_STEP_AFTER = 25;
    static SchoolRankMilestone school_rank_milestone(int index);    // The index-th one (past the list: every SCHOOL_RANK_STEP_AFTER)

    // Gives the Shiny Scales of the milestones reached and not claimed yet: returns how many
    int claim_school_rank_rewards();

    // The Clam Room's welcome gift (one Shiny Scale for the first roll), once: true if it was given now
    bool give_clam_room_gift();

    // Rewarded ads (phones): ADS_PER_DAY a day (the count starts again each day). watch_ad counts one and gives its
    // reward (a Shiny Scale); false when none is left today
    static constexpr int ADS_PER_DAY = 3;
    int ads_left_today() const;
    bool watch_ad();

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
    static constexpr int MAX_EXTRA_LEVELS = 10;       // The Clam Room's "+n" goes up to this

    // Shells a stored fish sells for (x its baseUpgradeCost)
    static constexpr int SPARE_FISH_SHELLS_FACTOR = 5;

    // Health and attack of a unit at this level, compared to level 1 (x1.15 per level)
    static float stat_multiplier(int level);

    // Shells to go from level to level + 1 (level 0: unlocking it, the base cost): x1.5 per level, x0.7 after
    // each evolution (levels 10 and 30). The Upgrade screen and FishEditor's cost estimate both use it.
    static int upgrade_cost(int baseUpgradeCost, int level);

    // ===== Pearls: the rare currency (like The Battle Cats' cat food), spent in the Pearl Shop (UIShopMenu) =====
    // They come from looking at a new fish in the encyclopedia, the shop's free videos (phones), and later its packs
    static constexpr int PEARLS_PER_NEW_FISH = 10;     // A fish owned and seen in the encyclopedia for the first time
    static constexpr int PEARLS_PER_AD = 5;            // The shop's free offer, SHOP_ADS_PER_DAY a day
    static constexpr int SHOP_ADS_PER_DAY = 3;
    static constexpr int PEARLS_FOR_SCALE = 25;        // One Shiny Scale
    static constexpr int PEARLS_FOR_TEN_SCALES = 225;  // Ten at once: one of them free
    bool discover_fish(int unitUid);                   // +PEARLS_PER_NEW_FISH the first time (false: seen before or not owned)
    bool is_fish_discovered(int unitUid) const;
    int undiscovered_fish() const;                     // Owned fish not looked at yet (their pearls are waiting)
    int shop_ads_left_today() const;
    bool watch_shop_ad();                              // +PEARLS_PER_AD (false: none left today)
    bool buy_shiny_scales(int count);                  // 1 or 10 for pearls (false: not enough)

    // ===== Base upgrades (the Upgrade screen's BASE cards, like The Battle Cats' base upgrades) =====
    // Base Health: the fish bowl's health in every stage, +15 % a level. Wallet Up: the wallet holds more and fills
    // faster from the start of every stage, +10 % a level. Both from level 1 to BASE_UPGRADE_MAX_LEVEL
    enum class BaseUpgrade { HEALTH, WALLET, COUNT };
    static constexpr int BASE_UPGRADE_MAX_LEVEL = 10;
    static constexpr int BASE_UPGRADE_COST = 500;     // Shells from level 1 to 2, x1.5 a level after that
    static int base_upgrade_cost(int level);          // From level to level + 1
    static float base_health_factor(int level);       // The bowl's health at this level, compared to level 1
    static float wallet_factor(int level);            // The wallet's size and filling speed, compared to level 1
    int get_base_upgrade_level(BaseUpgrade upgrade) const;
    void upgrade_base(BaseUpgrade upgrade);           // One level up (never past the max)

    // ===== Energy (like The Battle Cats' energy): playing a stage costs some (StageData::energyCost) =====
    // It comes back by itself, one point every ENERGY_REGEN_SECONDS on the real clock (the game closed too), up to
    // the max: ENERGY_BASE_MAX, and one more every SCHOOL_RANKS_PER_ENERGY of School Rank (like the User Rank raising
    // The Battle Cats' max energy). A full refill costs ENERGY_REFILL_PEARLS. Stored as an amount and the moment it
    // was counted (energyTime): what came back since is worked out from the clock when asked
    static constexpr int ENERGY_BASE_MAX = 50;
    static constexpr int SCHOOL_RANKS_PER_ENERGY = 2;
    static constexpr int ENERGY_REGEN_SECONDS = 60;
    static constexpr int ENERGY_REFILL_PEARLS = 20;
    int get_max_energy() const;
    int get_energy() const;                           // Now (what came back since energyTime counted)
    int seconds_to_next_energy() const;               // Until the next point comes back (0: full)
    int seconds_to_energy(int amount) const;          // Until there is this much (0: there is, -1: more than the max)
    bool spend_energy(int amount);                    // false (nothing spent) if there isn't enough
    bool refill_energy();                             // Back to the max for ENERGY_REFILL_PEARLS (false: not enough
                                                      // pearls, or already full)
    void fill_energy();                               // Back to the max for free (a new save, the debug tools)

public:
    // Currency
    int shells = 0;
    int shinyScales = 0;                   // The Clam Room's currency
    int fishScales = 0;                    // From selling stored fish: FISH_SCALES_PER_SHINY make a Shiny Scale
    std::map<int, int> clamStorage;        // Fish out of the clam waiting to be used or sold: unit UID -> how many
    int schoolRankRewardsClaimed = 0;           // School Rank milestones already rewarded
    bool hasClamRoomGift = false;          // The welcome Shiny Scale was given
    std::string adsDay;                    // The day of the ads counted (YYYY-MM-DD), and how many
    int adsToday = 0;
    int pearls = 0;                        // The rare currency (Pearl Shop)
    std::unordered_set<int> discoveredFish;// Fish already looked at in the encyclopedia (their pearls given)
    std::string shopAdsDay;                // The day of the shop's free videos counted, and how many
    int shopAdsToday = 0;
    std::array<int, static_cast<size_t>(BaseUpgrade::COUNT)> baseUpgradeLevels{ 1, 1 };   // Saved in "progression"
    int energy = ENERGY_BASE_MAX;          // The energy when energyTime counted it (get_energy: now)
    std::int64_t energyTime = 0;           // Then, in seconds since 1970 (std::time); points come back from there

    // Units
    struct OwnedUnit
    {
        int level = 1;
        int form = 0;
        int extraLevels = 0;   // From the Clam Room (saved as "extraLevels")
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
