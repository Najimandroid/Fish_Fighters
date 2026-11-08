#include "StatusManager.h"

#include "../entities/BattleEntity.h"
#include "../DataLoader.h"

#include <algorithm>

// ==================
// Init
// ==================
void StatusManager::init(std::shared_ptr<DataLoader> dataLoader)
{
    m_dataLoader = dataLoader;
}

// ==================
// Requests
// ==================
void StatusManager::apply_on_hit_abilities(const BattleEntity& attacker, BattleEntity& target, float stageTime)
{
    // Bases have no data, so no abilities and no types
    if (!attacker.data || !target.data) return;

    for (const AbilityData& ability : attacker.data->abilities)
    {
        if (!is_on_hit(ability.type)) continue;

        // The ability only works on targets with one of its target types
        bool targetsThisEntity = std::any_of(ability.targetTypes.begin(), ability.targetTypes.end(), [&target](EntityType type)
            {
                return std::find(target.data->types.begin(), target.data->types.end(), type) != target.data->types.end();
            });

        if (targetsThisEntity) request(attacker, target, ability, stageTime);
    }
}

StatusOutcome StatusManager::request(const BattleEntity& source, BattleEntity& target, const AbilityData& ability, float stageTime)
{
    StatusLogEntry entry;
    entry.time = stageTime;
    entry.source = source.data ? source.data->name : "Unknown";
    entry.target = target.data ? target.data->name : "Unknown";
    entry.status = status_for_ability(ability.type);
    entry.chance = ability.chance;
    entry.guaranteed = m_isGuaranteedMode;
    entry.value = ability.value;
    entry.duration = ability.duration;

    if (entry.status == StatusType::NONE) return StatusOutcome::MISSED; // Passive ability: nothing to inflict

    // Chance roll: success if roll < chance (a 100% chance always succeeds)
    std::uniform_real_distribution<float> distribution(0.f, 100.f);
    entry.roll = distribution(m_randomGenerator);

    if (m_isGuaranteedMode || entry.roll < ability.chance)
        entry.outcome = apply(target, entry.status, ability.value, ability.duration);
    else
        entry.outcome = StatusOutcome::MISSED;

    m_log.push_back(entry);
    if (m_log.size() > MAX_LOG_ENTRIES) m_log.pop_front();

    return entry.outcome;
}

StatusOutcome StatusManager::apply(BattleEntity& target, StatusType type, float value, float duration)
{
    auto it = std::find_if(target.m_activeStatuses.begin(), target.m_activeStatuses.end(), [type](const StatusEffect& status)
        {
            return status.type == type;
        });

    if (it == target.m_activeStatuses.end())
    {
        target.m_activeStatuses.push_back({ type, duration, value });
        return StatusOutcome::APPLIED;
    }

    // Already affected: durations add up, the strongest value is kept
    it->durationRemaining += duration;
    if (is_stronger(type, value, it->value)) it->value = value;

    return StatusOutcome::STACKED;
}

// ==================
// Update
// ==================
void StatusManager::update(float deltaTime, const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList)
{
    if (deltaTime <= 0.f) return;

    for (const auto& [layer, entities] : entityList)
    {
        for (const auto& entity : entities)
        {
            auto& statuses = entity->m_activeStatuses;

            for (auto& status : statuses) status.durationRemaining -= deltaTime;

            statuses.erase(std::remove_if(statuses.begin(), statuses.end(), [](const StatusEffect& status)
                {
                    return status.durationRemaining <= 0.f;
                }), statuses.end());
        }
    }
}

// ==================
// Rules
// ==================
bool StatusManager::is_stronger(StatusType type, float newValue, float currentValue) const
{
    StrongestRule rule = StrongestRule::LOWEST;
    if (m_dataLoader) rule = m_dataLoader->get_status_data(type)->strongest;

    return (rule == StrongestRule::LOWEST) ? newValue < currentValue : newValue > currentValue;
}

StatusType StatusManager::status_for_ability(AbilityType abilityType)
{
    switch (abilityType)
    {
    case AbilityType::PARALYZE_ATTACK:    return StatusType::PARALYZED;
    case AbilityType::SLOW_ATTACK:        return StatusType::SLOW;
    case AbilityType::ATTACK_DOWN_ATTACK: return StatusType::ATTACK_DOWN;
    default:                              return StatusType::NONE;
    }
}

// ==================
// Debug
// ==================
void StatusManager::set_guaranteed_mode(bool isGuaranteed)
{
    m_isGuaranteedMode = isGuaranteed;
}

bool StatusManager::is_guaranteed_mode() const
{
    return m_isGuaranteedMode;
}

const std::deque<StatusLogEntry>& StatusManager::get_log() const
{
    return m_log;
}

void StatusManager::clear_log()
{
    m_log.clear();
}
