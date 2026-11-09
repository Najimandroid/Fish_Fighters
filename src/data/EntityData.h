#pragma once

#include <string>
#include <vector>

#include "EntityType.h"
#include "AbilityData.h"

enum AttackType;

/*
 * EntityData
 * ----------
 * Defines the base/template data for a unit or entity in the game.
 *
 * This struct is not used directly during gameplay — instead, it provides
 * the "blueprint" for instantiating units. For example, units on the field
 * will reference values from their EntityData.
 *
 * Use EntityDataLoader to retrieve EntityData by UID.
 */

struct EntityData
{
    // ===== Identification =====
    int UID = -1;                               // Unique identifier for this unit

    // ===== General Information =====
    std::string name = "Unknown Unit";
    std::string description = "???";            // Short description (e.g., summary)

    // ===== Battle Attributes =====
    int cost = 50;                              // Deployment cost (currency required to spawn)
    float cooldown = 1.0f;                      // Time (in seconds) before unit can be deployed again

    int health = 1;                             // Maximum health of the unit

    int attackPower = 1;                        // Base damage per attack
    float attackRange = 1.0f;                   // Effective attack range
    AttackType attackType = static_cast<AttackType>(1); // Attack style/type (single, AoE, etc.)
    float attackFrequency = 1.0f;               // Seconds between two attacks
    float foreswingTime = 0.0f;                 // Delay before attack is executed (wind-up)
    float backswingTime = 0.0f;                 // Delay after attack before returning to idle

	std::pair<float, float> damageZone = { 0.f, 0.f }; // Damage zone offsets (start, end) relative to unit position

    std::vector<EntityType> types;              // Entity's types (abilities target them), empty = no type
    std::vector<AbilityData> abilities;         // Entity's abilities

    float movementSpeed = 1.0f;                 // Movement speed across the battlefield
    int knockbackCount = 1;                     // Number of knockbacks the unit can sustain before dying

    // ===== Visual Attributes =====
    std::string texture;                        // File path to the sprite/texture
    int frameCount = 1;                         // Number of animation frames
    int knockbackFrameIndex = 1;                // Animation frame used during knockback
    float shadowScale = 1.0f;                   // Shadow width compared to the body ("shadowScale", optional, set in FishEditor)

    // ===== Upgrade Attributes =====
    int baseUpgradeCost = 100;                  // Base cost for upgrading this unit
};
