#include "CombatLog.h"

#include "../entities/BattleEntity.h"

// ==================
// Record
// ==================
void CombatLog::record(const BattleEntity& attacker, const BattleEntity& target, const DamageResult& result, float stageTime)
{
    CombatLogEntry entry;
    entry.time = stageTime;
    entry.attackerId = attacker.uniqueId;
    entry.targetId = target.uniqueId;
    entry.attacker = attacker.data ? attacker.data->name : "Base";
    entry.target = target.data ? target.data->name : "Base";
    entry.result = result;
    entry.targetHealthAfter = target.currentHealth;

    m_entries.push_back(entry);
    if (m_entries.size() > MAX_ENTRIES) m_entries.pop_front();
}

// ==================
// Accessors
// ==================
const std::deque<CombatLogEntry>& CombatLog::get_entries() const
{
    return m_entries;
}

void CombatLog::clear()
{
    m_entries.clear();
}
