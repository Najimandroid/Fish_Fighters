#pragma once

#include <vector>
#include <memory>
#include <string>

#include "RewardData.h"

class EnemyStageData;

/*
 * StageData
 * ---------
 * Defines the static configuration for a stage:
 *  - What enemies it spawns
 *  - Visuals (textures/backgrounds)
 *  - Stage rules (limits, base health)
 *  - Rewards given when completed
 *
 * StageData acts as a blueprint. During gameplay, Stage uses this data
 * to initialize the battlefield and control wave spawning.
 */

struct StageData
{
    // ===== Identification =====
    int UID = -1;                          // Unique identifier for this stage
    std::string name = "Unknown Stage";    // Display name for the stage

    // ===== Stage Rules =====
    int enemiesLimit = 10;                 // Maximum number of enemies alive at once
    int unitsLimit = 10;                   // Maximum number of player units deployed at once
    float baseHealth = 100.0f;             // Starting health of the enemy base
    float playerBaseHealth = 350.0f;       // Starting health of the player base (optional "playerBaseHealth" in the JSON)
    float length = 960.0f;                 // Distance between the enemy base and the player base

    // ===== Enemy Waves =====
    std::vector<std::shared_ptr<EnemyStageData>> enemies;
    // Defines which enemies spawn, when, and under what conditions

    // ===== Visuals =====
    std::string baseTexture;               // Texture used for the enemy base in this stage
    std::string backgroundTexture;         // Background image for the battlefield (any size, its ground row in backgrounds.json)

    // ===== Music (names of assets/audios/music/music.json, set in FishEditor) =====
    std::string music = "theme_default";   // Plays from the start ("music", optional)
    std::string musicChange;               // Optional: switches to this theme... ("musicChange": { "music", "baseHealth" })
    float musicChangeBaseHealth = 50.f;    // ...once the enemy base is at or below this % of its health

    // ===== Rewards =====
    std::vector<std::shared_ptr<RewardData>> rewards;
    // Rewards given to the player after completing the stage
};
