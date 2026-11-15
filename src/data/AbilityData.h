#pragma once

#include "EntityType.h"

#include <vector>

/*
 * AbilityData
 * ----------
 * Represents an ability.
 * Abilities give boosts towards a certain target. For example, a resistance or a damage boost.
 *
 * Two kinds:
 *  - Passive (STRONG_AGAINST, RESISTANT_AGAINST, THORNS): read by the DamageCalculator
 *  - On hit (PARALYZE_ATTACK, SLOW_ATTACK, WEAKEN_ATTACK): inflict a status through the StatusManager
 * THORNS: an attacker of the target types that hits the entity takes "value" x the entity's max health back
 * (0.05: 5%), e.g. { "type": "THORNS", "targetTypes": [ 4 ], "value": 0.05 } (4: ALL_ENEMIES)
 *
 * In the JSON (units.json forms and enemies.json), for example:
 *   "abilities": [
 *     { "type": "STRONG_AGAINST", "targetTypes": [ 0 ] },
 *     { "type": "WEAKEN_ATTACK", "targetTypes": [ 1 ], "value": 0.5, "chance": 30, "duration": 3 }
 *   ]
 */
enum class AbilityType
{
    NONE,

    STRONG_AGAINST,
    RESISTANT_AGAINST,

    PARALYZE_ATTACK,
    SLOW_ATTACK,
    WEAKEN_ATTACK,

    THORNS,     // Hitting it hurts: the attacker takes a share of its max health

    COUNT // Not an ability: keep it last (used to list every ability type)
};

// Name used in the JSON and the debug tools
inline const char* to_string(AbilityType type)
{
    switch (type)
    {
    case AbilityType::STRONG_AGAINST:     return "STRONG_AGAINST";
    case AbilityType::RESISTANT_AGAINST:  return "RESISTANT_AGAINST";
    case AbilityType::PARALYZE_ATTACK:    return "PARALYZE_ATTACK";
    case AbilityType::SLOW_ATTACK:        return "SLOW_ATTACK";
    case AbilityType::WEAKEN_ATTACK: return "WEAKEN_ATTACK";
    case AbilityType::THORNS:             return "THORNS";
    default:                              return "NONE";
    }
}

// True for abilities that inflict a status when they hit
inline bool is_on_hit(AbilityType type)
{
    return type == AbilityType::PARALYZE_ATTACK
        || type == AbilityType::SLOW_ATTACK
        || type == AbilityType::WEAKEN_ATTACK;
}

struct AbilityData
{
    AbilityType type = AbilityType::NONE;

    std::vector<EntityType> targetTypes; // Applies if the other entity has any of these types (empty = applies to nothing)

    float value = 0.f;      // On hit: strength of the status (e.g. 0.5 = attack x0.5 for WEAKEN); THORNS: share of max health (0.05)

    float chance = 100.f;   // On hit only: chance in % to inflict the status

    float duration = 0.f;   // On hit only: seconds the status lasts
};
