#pragma once

/*
 * BattleResult
 * ------------
 * How the last battle ended, for the results screen (Stage fills it when a base falls).
 */
struct BattleResult
{
    int stageUID = -1;
    bool isVictory = false;
    bool isFirstClear = false;          // Won for the first time (all rewards, not only shells)
    int shells = 0;                     // Shells earned
    int newUnitUID = -1;                // A fish unlocked by this win (-1: none)
    bool opensClamRoom = false;         // This win opened the Clam Room (a "clamRoom" stage reward, first clear)
    float enemyBaseHealth = 0.f;        // What was left of the enemy base (defeat: "how close was it")
    float enemyBaseMaxHealth = 1.f;
};
