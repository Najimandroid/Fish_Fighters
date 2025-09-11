#pragma once

#include <string>

enum AttackType;

/*
 * EntityData represents the template for all units in the game.
 * Use EntityDataLoader to retrieve the data of a specific unit by UID.
 */

struct EntityData
{
    // Unique identifier for the unit
    int UID = -1;

    // General information
    std::string name = "Unknown Unit";
    std::string description = "???";

    // Battle attributes
    int cost = 50;                             // Deployment cost
    int cooldown = 1;                          // Deployment cooldown

    int health = 1;                            // Full health

    int attackPower = 1;                       // Damage dealt per attack
    float attackRange = 1.0f;
    AttackType attackType = static_cast<AttackType>(1); // Type of attack (single, area, etc.)
    float attackFrequency = 1.0f;              // Attacks per second
    float foreswingTime = 0.0f;                // Time before attack is executed
    float backswingTime = 0.0f;                // Time after attack is executed before idle

    float movementSpeed = 1.0f;
    int knockbackCount = 1;                    // Number of knockbacks before dying

    // Visual attributes
    std::string texture;                        // File path to sprite/texture
    int frameCount = 1;                         // Number of animation frames
    int knockbackFrameIndex = 1;                // Frame index for knockback animation

    // Upgrade-related
    int baseUpgradeCost = 100;                  // Base cost for unit upgrades
};
