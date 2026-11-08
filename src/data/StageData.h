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
    std::string backgroundTexture;         // Background image for the battlefield (any size)
    float backgroundGroundRow = -1.f;      // Row of the background image units stand on (optional "backgroundGroundRow"),
                                           // -1 = same place as the 1280 x 1120 template (see background_ground_row)

    // ===== Rewards =====
    std::vector<std::shared_ptr<RewardData>> rewards;
    // Rewards given to the player after completing the stage
};

/*
 * Background placement
 * --------------------
 * Used by Stage::init_background and FishEditor's previews, so both place backgrounds the same way:
 * the image is drawn x1.5, copies side by side, with its ground row on the line units stand on.
 */
inline constexpr float BACKGROUND_SCALE = 1.5f;
inline constexpr float BATTLE_GROUND_Y = 720.f;                       // World y of the units' feet (1080 * 2/3, layer 0)
inline constexpr float DEFAULT_BACKGROUND_GROUND_RATIO = 613.f / 1120.f; // The 1280 x 1120 template: row 613

// Row of the background placed on the ground: the stage's own, or the template's ratio of the image height
inline float background_ground_row(float groundRow, float imageHeight)
{
    return groundRow >= 0.f ? groundRow : imageHeight * DEFAULT_BACKGROUND_GROUND_RATIO;
}
