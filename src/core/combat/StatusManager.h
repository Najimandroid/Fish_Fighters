#pragma once

#include "../entities/status/StatusEffect.h"
#include "../../data/AbilityData.h"

#include <deque>
#include <functional>
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
 * When a status stacks, the line of the status it stacks on is updated instead of adding a new one.
 */
struct StatusLogEntry
{
    float time = 0.f;                    // Stage time (seconds) of the request, or of the last stack
    float firstTime = 0.f;               // Stage time the status was first applied
    int sourceId = -1;                   // Attacker's BattleEntity::uniqueId
    int targetId = -1;                   // Target's BattleEntity::uniqueId
    std::string source;                  // Attacker's name
    std::string target;                  // Target's name
    StatusType status = StatusType::NONE;
    float roll = 0.f;                    // Random roll (0-100) of the last request, compared with the chance
    float chance = 0.f;                  // Chance of the ability (%)
    bool guaranteed = false;             // Guaranteed mode was on
    float value = 0.f;                   // Value requested (after stacking: the strongest value now active)
    float duration = 0.f;                // Duration requested (after stacking: time left)
    int stackCount = 1;                  // How many times the status landed on this target
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
 *  - PARALYZED: puts the target in ParalyzedState, which lasts until the status expires
 *  - Keeps a log of every request for the debug tools (one line per status, updated when it stacks)
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

    // Called each time an on-hit ability lands its status (applied or stacked): the Stage plays its sound
    void set_on_status_landed(std::function<void(StatusType)> callback);

    // ===== Debug =====
    // Applies a status directly (no ability, no chance roll), logged with "Debug" as the source
    StatusOutcome debug_apply(BattleEntity& target, StatusType type, float value, float duration, float stageTime);

    // Removes every status of `target`
    void debug_clear(BattleEntity& target);

    void set_guaranteed_mode(bool isGuaranteed);     // true = every chance is 100%
    bool is_guaranteed_mode() const;
    const std::deque<StatusLogEntry>& get_log() const; // Newest last
    void clear_log();

    // Status inflicted by an on-hit ability (NONE for passive abilities)
    static StatusType status_for_ability(AbilityType abilityType);

private:
    // Adds the status, or stacks it if the target already has it
    StatusOutcome apply(BattleEntity& target, StatusType type, float value, float duration);

    // Puts the target in ParalyzedState (cancels walking or attacking)
    void enter_paralysis(BattleEntity& target);

    // Adds a request to the log; a stack updates the line of the status it stacks on
    void add_to_log(StatusLogEntry entry, const BattleEntity& target);

    // True if `newValue` is stronger than `currentValue` for this status
    bool is_stronger(StatusType type, float newValue, float currentValue) const;

private:
    std::shared_ptr<DataLoader> m_dataLoader;

    bool m_isGuaranteedMode = false;
    std::mt19937 m_randomGenerator{ std::random_device{}() };

    std::deque<StatusLogEntry> m_log;
    static constexpr size_t MAX_LOG_ENTRIES = 200;

    std::function<void(StatusType)> m_onStatusLanded;
};
