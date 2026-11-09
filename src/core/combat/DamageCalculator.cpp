#include "DamageCalculator.h"

#include "../entities/BattleEntity.h"

#include <algorithm>

namespace
{
    // True if `data` has at least one of `targetTypes`
    bool has_any_type(const EntityData& data, const std::vector<EntityType>& targetTypes)
    {
        return std::any_of(targetTypes.begin(), targetTypes.end(), [&data](EntityType targetType)
            {
                return std::find(data.types.begin(), data.types.end(), targetType) != data.types.end();
            });
    }

    // True if one of `owner`'s abilities of the given type targets one of `other`'s types.
    // Several matching abilities still count once, so multipliers never stack (no x4).
    bool has_ability_against(const BattleEntity& owner, AbilityType abilityType, const BattleEntity& other)
    {
        // Bases have no data, so they have no abilities and no types
        if (!owner.data || !other.data) return false;

        return std::any_of(owner.data->abilities.begin(), owner.data->abilities.end(), [&](const AbilityData& ability)
            {
                return ability.type == abilityType && has_any_type(*other.data, ability.targetTypes);
            });
    }
}

// ==================
// Calculate
// ==================
DamageResult DamageCalculator::calculate(const BattleEntity& attacker, const BattleEntity& target) const
{
    DamageResult result;

    // An attacker without data can't deal damage
    if (!attacker.data) return result;

    result.baseDamage = static_cast<float>(attacker.data->attackPower);

    // Stage scaling for enemies, level scaling for units (set when they spawn)
    result.attackMultiplier = attacker.magnification.y;

    // Attacker strong against one of the target's types
    if (has_ability_against(attacker, AbilityType::STRONG_AGAINST, target))
        result.strongAgainstMultiplier = STRONG_AGAINST_MULTIPLIER;

    // Target resistant against one of the attacker's types
    if (has_ability_against(target, AbilityType::RESISTANT_AGAINST, attacker))
        result.resistanceMultiplier = RESISTANT_MULTIPLIER;

    // Attacker weakened by a WEAKEN status (the StatusManager keeps the strongest value)
    if (const StatusEffect* weaken = attacker.get_status(StatusType::WEAKEN))
        result.weakenMultiplier = weaken->value;

    result.finalDamage = result.baseDamage
        * result.attackMultiplier
        * result.strongAgainstMultiplier
        * result.resistanceMultiplier
        * result.weakenMultiplier;

    return result;
}
