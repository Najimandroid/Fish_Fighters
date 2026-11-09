#pragma once

#include "DamageCalculator.h"

#include <deque>
#include <string>

class BattleEntity;

/*
 * CombatLogEntry
 * --------------
 * One hit: who hit whom, the DamageCalculator's full breakdown, and the target's health after it.
 */
struct CombatLogEntry
{
    float time = 0.f;               // Stage time (seconds)
    int attackerId = -1;            // BattleEntity::uniqueId
    int targetId = -1;
    std::string attacker;           // Names (bases have no data: "Base")
    std::string target;
    DamageResult result;            // Every multiplier of the hit
    float targetHealthAfter = 0.f;
};

/*
 * CombatLog
 * ---------
 * Keeps the last hits of the stage for the debug tools. Owned by Stage, filled by AttackState
 * right after a hit is applied, cleared when the stage unloads.
 */
class CombatLog
{
public:
    // Records a hit that was just applied to `target`
    void record(const BattleEntity& attacker, const BattleEntity& target, const DamageResult& result, float stageTime);

    const std::deque<CombatLogEntry>& get_entries() const; // Newest last
    void clear();

private:
    std::deque<CombatLogEntry> m_entries;
    static constexpr size_t MAX_ENTRIES = 300;
};
