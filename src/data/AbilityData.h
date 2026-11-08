#pragma once

#include "EntityType.h"

#include <vector>

/*
 * AbilityData
 * ----------
 * Represents an ability.
 * Abilities give boosts towards a certain target. For example, a resistance or a damage boost.
 *
 * In the JSON (units.json forms and enemies.json), for example:
 *   "abilities": [ { "type": "STRONG_AGAINST", "targetTypes": [ 0 ] } ]
 */
enum class AbilityType
{
    NONE,

    STRONG_AGAINST,
    RESISTANT_AGAINST,

    FREEZE_ATTACK,
    SLOW_ATTACK,
    ATTACK_DOWN_ATTACK,
};


struct AbilityData
{
    AbilityType type = AbilityType::NONE;

    std::vector<EntityType> targetTypes; // Applies if the other entity has any of these types (empty = applies to nothing)

    float value = 0.f;

    float chance = 100.f;

    float duration = 0.f;
};