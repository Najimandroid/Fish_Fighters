#pragma once

/*
 * AbilityData
 * ----------
 * Represents an ability.
 * Abilities give boosts towards a certain target. For example, a resistance or a damage boost.
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

    EntityType targetType = EntityType::NONE;

    float value = 0.f;

    float chance = 100.f;

    float duration = 0.f;
};