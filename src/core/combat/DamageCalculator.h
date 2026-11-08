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
    float baseDamage = 0.0f;              // Attacker's attackPower (from EntityData)
    float attackMultiplier = 1.0f;        // Attacker's magnification.y (stage scaling or unit level)
    float strongAgainstMultiplier = 1.0f; // STRONG_AGAINST_MULTIPLIER if the attacker is strong against the target's types
    float resistanceMultiplier = 1.0f;    // RESISTANT_MULTIPLIER if the target is resistant against the attacker's types
    float finalDamage = 0.0f;             // Damage the target will take (every multiplier applied)
};

/*
 * DamageCalculator
 * ----------------
 * The only place in the game where damage is decided.
 *
 * Responsibilities:
 *  - Reads the attacker and the target (never modifies them)
 *  - Applies every damage multiplier, all multiplied together
 *  - Returns a DamageResult; the target then applies it with BattleEntity::take_damage()
 *
 * Owned by Stage. Hits on bases go through it like any other hit (bases have no types).
 *
 * Planned modifiers: statuses (attack down).
 */
class DamageCalculator
{
public:
    // ===== Balancing =====
    // Change these to rebalance every ability of that kind at once
    static constexpr float STRONG_AGAINST_MULTIPLIER = 2.0f; // Damage dealt to a type the attacker is strong against
    static constexpr float RESISTANT_MULTIPLIER = 0.5f;      // Damage taken from a type the target is resistant against

    // Calculates the damage `attacker` deals to `target` with one hit
    DamageResult calculate(const BattleEntity& attacker, const BattleEntity& target) const;
};
