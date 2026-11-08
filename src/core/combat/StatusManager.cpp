#include "StatusManager.h"

#include "../entities/BattleEntity.h"
#include "../entities/machine/StateMachine.h"
#include "../entities/machine/states/ParalyzedState.h"
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
    entry.firstTime = stageTime;
    entry.sourceId = source.uniqueId;
    entry.targetId = target.uniqueId;
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

    add_to_log(entry, target);
    return entry.outcome;
}

StatusOutcome StatusManager::debug_apply(BattleEntity& target, StatusType type, float value, float duration, float stageTime)
{
    StatusLogEntry entry;
    entry.time = stageTime;
    entry.firstTime = stageTime;
    entry.targetId = target.uniqueId;
    entry.source = "Debug";
    entry.target = target.data ? target.data->name : "Unknown";
    entry.status = type;
    entry.chance = 100.f;
    entry.guaranteed = true;
    entry.value = value;
    entry.duration = duration;

    if (type == StatusType::NONE || !target.data) return StatusOutcome::MISSED; // Bases never get statuses

    entry.outcome = apply(target, type, value, duration);
    add_to_log(entry, target);
    return entry.outcome;
}

void StatusManager::debug_clear(BattleEntity& target)
{
    // A paralyzed entity leaves ParalyzedState by itself once the status is gone
    target.m_activeStatuses.clear();
}

void StatusManager::add_to_log(StatusLogEntry entry, const BattleEntity& target)
{
    if (entry.outcome == StatusOutcome::STACKED)
    {
        // Stacking updates the line of the status it stacks on instead of adding a new one
        const StatusEffect* status = target.get_status(entry.status);
        entry.value = status ? status->value : entry.value;             // Strongest value now active
        entry.duration = status ? status->durationRemaining : entry.duration; // Time left after stacking

        auto previous = std::find_if(m_log.rbegin(), m_log.rend(), [&entry](const StatusLogEntry& line)
            {
                return line.targetId == entry.targetId && line.status == entry.status && line.outcome != StatusOutcome::MISSED;
            });

        if (previous != m_log.rend())
        {
            entry.firstTime = previous->firstTime;
            entry.stackCount = previous->stackCount + 1;
            m_log.erase(std::next(previous).base()); // Moved to the end so the newest change shows first
        }
    }

    m_log.push_back(entry);
    if (m_log.size() > MAX_LOG_ENTRIES) m_log.pop_front();
}

StatusOutcome StatusManager::apply(BattleEntity& target, StatusType type, float value, float duration)
{
    auto it = std::find_if(target.m_activeStatuses.begin(), target.m_activeStatuses.end(), [type](const StatusEffect& status)
        {
            return status.type == type;
        });

    StatusOutcome outcome = StatusOutcome::APPLIED;

    if (it == target.m_activeStatuses.end())
    {
        target.m_activeStatuses.push_back({ type, duration, value });
    }
    else
    {
        // Already affected: durations add up, the strongest value is kept
        it->durationRemaining += duration;
        if (is_stronger(type, value, it->value)) it->value = value;
        outcome = StatusOutcome::STACKED;
    }

    if (type == StatusType::PARALYZED) enter_paralysis(target);

    return outcome;
}

void StatusManager::enter_paralysis(BattleEntity& target)
{
    if (!target.stateMachine) return;

    // A knocked back entity goes to ParalyzedState when its knockback ends (KnockbackState checks the status),
    // a dead one stays dead, and an already paralyzed one just keeps the longer duration
    const std::string state = target.stateMachine->get_active_state_id();
    if (state == "KNOCKBACK" || state == "DEAD" || state == "PARALYZED") return;

    // Leaving any other state cancels what the entity was doing (walking, attacking...)
    target.stateMachine->change_state(std::make_unique<ParalyzedState>(target.stateMachine.get()));
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
    case AbilityType::WEAKEN_ATTACK: return StatusType::WEAKEN;
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
