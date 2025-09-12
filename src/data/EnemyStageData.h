#pragma once

#include <SFML/System/Vector2.hpp>

/*
 * EnemyStageData
 * --------------
 * Represents the spawn data for a single enemy type within a stage wave.
 *
 * The Stage uses EnemyStageData to determine when and how enemies should spawn:
 *  - Spawn timing (start time, respawn intervals)
 *  - Enemy count (fixed number or infinite)
 *  - Scaling multipliers for health/attack
 *  - Special flags (boss enemies, bypassing limits)
 */

class EnemyStageData {
public:

    // ===== Identification =====
    int UID = -1;                  // Unique identifier for the enemy unit (matches EntityData UID)

    // ===== Spawn Control =====
    int amount = 0;                 // How many enemies to spawn (-1 = infinite spawning)
    float respawnTime = 0;          // Time between consecutive spawns (seconds)
    float spawnStart = 0;           // Time at which spawning begins (seconds since stage start)

    int layer = -1;                 // Battlefield layer to spawn on (-1 = any available layer)

    // ===== Conditional Spawning =====
    float baseHealthThreshold = 100; // Enemy spawns only if the base health % is <= this value
    // Example: 100 = always spawns, 50 = spawns when base is at 50% HP or below

// ===== Attribute Scaling =====
    sf::Vector2f magnification = { 1.f, 1.f };
    // x = multiplier for enemy HP
    // y = multiplier for enemy attack

    // ===== Special Flags =====
    bool isBoss = false;            // Marks this enemy as a boss (may change the music, always create a shockwave)
    bool bypassEnemyLimit = false;  // If true, spawns even when the stage’s max enemy limit is reached

    // ===== Internal Logic (runtime only) =====
    int spawnedCount = 0;           // How many of this enemy have already spawned
    float currentTimer = 0.f;       // Timer tracking respawn intervals
    bool hasStarted = false;        // Whether the spawning for this enemy has already begun
};
