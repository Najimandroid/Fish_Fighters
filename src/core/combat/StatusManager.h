#pragma once

#include "../entities/status/StatusEffect.h"
#include "../../data/AbilityData.h"

#include <deque>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

class BattleEntity;
class DataLoader;

/*
 * StatusOutcome
 * -------------
 * What happened to one status request.
 */
enum class StatusOutcome
{
    APPLIED,    // The target didn't have it: added
    STACKED,    // The target already had it: durations added, strongest value kept
    MISSED      // The chance roll failed
};

/*
 * StatusLogEntry
 * --------------
 * One status request, kept for the debug tools.
 */
struct StatusLogEntry
{
    float time = 0.f;                    // Stage time (seconds) of the request
    std::string source;                  // Attacker's name
    std::string target;                  // Target's name
    StatusType status = StatusType::NONE;
    float roll = 0.f;                    // Random roll (0-100), compared with the chance
    float chance = 0.f;                  // Chance of the ability (%)
    bool guaranteed = false;             // Guaranteed mode was on
    float value = 0.f;                   // Value requested
    float duration = 0.f;                // Duration requested
    StatusOutcome outcome = StatusOutcome::MISSED;
};

/*
 * StatusManager
 * -------------
 * Decides whether a status lands, how it stacks and when it ends.
 *
 * Responsibilities:
 *  - Rolls the chance of on-hit abilities (or always succeeds in guaranteed mode)
 *  - Stacking: the same status again adds the durations and keeps the strongest value,
 *    using the status's "strongest" rule from statuses.json
 *  - Counts down every active status and removes expired ones
 *  - Keeps a log of every request for the debug tools
 *
 * Statuses are stored on each entity (BattleEntity::get_statuses()), which can read them
 * but never change them: only this class can. They disappear with the entity, so nothing
 * here can point to an entity that no longer exists.
 *
 * Owned by Stage.
 */
class StatusManager
{
public:
    // Gives access to the status definitions (statuses.json)
    void init(std::shared_ptr<DataLoader> dataLoader);

    // Applies every on-hit ability of `attacker` that targets one of `target`'s types
    void apply_on_hit_abilities(const BattleEntity& attacker, BattleEntity& target, float stageTime);

    // Rolls `ability`'s chance and applies its status to `target`
    StatusOutcome request(const BattleEntity& source, BattleEntity& target, const AbilityData& ability, float stageTime);

    // Counts down every status of every entity in `entityList` and removes expired ones
    void update(float deltaTime, const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList);

    // ===== Debug =====
    void set_guaranteed_mode(bool isGuaranteed);     // true = every chance is 100%
    bool is_guaranteed_mode() const;
    const std::deque<StatusLogEntry>& get_log() const; // Newest last
    void clear_log();

    // Status inflicted by an on-hit ability (NONE for passive abilities)
    static StatusType status_for_ability(AbilityType abilityType);

private:
    // Adds the status, or stacks it if the target already has it
    StatusOutcome apply(BattleEntity& target, StatusType type, float value, float duration);

    // True if `newValue` is stronger than `currentValue` for this status
    bool is_stronger(StatusType type, float newValue, float currentValue) const;

private:
    std::shared_ptr<DataLoader> m_dataLoader;

    bool m_isGuaranteedMode = false;
    std::mt19937 m_randomGenerator{ std::random_device{}() };

    std::deque<StatusLogEntry> m_log;
    static constexpr size_t MAX_LOG_ENTRIES = 200;
};
