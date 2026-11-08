#pragma once

class BattleEntity;

/*
 * DamageResult
 * ------------
 * Breakdown of a single hit, as decided by the DamageCalculator.
 * Keeping every step (instead of only the final number) lets the debug tools
 * show why a hit dealt the damage it did.
 */
struct DamageResult
{
    float baseDamage = 0.0f;        // Attacker's attackPower (from EntityData)
    float attackMultiplier = 1.0f;  // Attacker's magnification.y (stage scaling or unit level)
    float finalDamage = 0.0f;       // Damage the target will take
};

/*
 * DamageCalculator
 * ----------------
 * The only place in the game where damage is decided.
 *
 * Responsibilities:
 *  - Reads the attacker and the target (never modifies them)
 *  - Applies every damage multiplier
 *  - Returns a DamageResult; the target then applies it with BattleEntity::take_damage()
 *
 * Owned by Stage. Hits on bases go through it like any other hit.
 *
 * Planned modifiers: entity types, passive traits (strong against, resistant against)
 * and statuses (attack down), all multiplied together.
 */
class DamageCalculator
{
public:
    // Calculates the damage `attacker` deals to `target` with one hit
    DamageResult calculate(const BattleEntity& attacker, const BattleEntity& target) const;
};
