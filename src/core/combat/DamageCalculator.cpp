#include "DamageCalculator.h"

#include "../entities/BattleEntity.h"

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

    // The target isn't read yet: it will be for types and resistances
    result.finalDamage = result.baseDamage * result.attackMultiplier;

    return result;
}
