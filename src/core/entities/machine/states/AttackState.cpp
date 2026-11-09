#include "AttackState.h"
#include "../StateMachine.h"
#include "../../BattleUnit.h"
#include "../../../stages/Stage.h"

#include "KnockbackState.h"
#include "WalkState.h"
#include "IdleState.h"

#include <iostream>

// ==================
// Constructor
// ==================
AttackState::AttackState(StateMachine* machine)
{
    m_machine = machine;
}

// ==================
// Enter
// ==================
void AttackState::enter()
{
    // Reset attack swing when entering
    m_isAttackReady = false;
    m_hasAttacked = false;
    m_currentAttackSwingTime = 0.0f;

    // std::cout << "Entering Attack State\n";
}

// ==================
// Perform
// ==================
void AttackState::perform(float deltaTime)
{
    auto entity = m_machine->get_owner().lock();
    if (!entity) return;

    // ===== Attack Cooldown Check =====
    // If attack cooldown not ready, return to idle
    if (entity->currentAttackCooldown < entity->data->attackFrequency)
    {
        m_machine->change_state(std::make_unique<IdleState>(m_machine));
        return;
    }

    // ===== Death / Knockback Check =====
    if (entity->currentHealth <= 0.0f ||
        entity->currentHealth <= entity->healthLeftBeforeNextKnockback)
    {
        m_machine->change_state(std::make_unique<KnockbackState>(m_machine));
        return;
    }

    // ===== Swing Timing =====
    // Foreswing -> the hit (once, as soon as the foreswing is over) -> backswing.
    // The hit never depends on landing inside [foreswing, foreswing + backswing]: with a backswing of 0
    // that window is a single instant the frames jump over, and the attack would never hit.
    m_currentAttackSwingTime += deltaTime;

    if (m_hasAttacked && m_currentAttackSwingTime >= entity->data->foreswingTime + entity->data->backswingTime)
    {
        // After swing ends, reset attack cooldown and return to idle
        entity->currentAttackCooldown = 0.0f;
        m_machine->change_state(std::make_unique<IdleState>(m_machine));
        return;
    }

    // Attack becomes "ready" after foreswing
    m_isAttackReady = (m_currentAttackSwingTime >= entity->data->foreswingTime);

    // ===== Attack Execution =====
    if (!m_isAttackReady || m_hasAttacked) return; // Not ready yet or already attacked
    m_hasAttacked = true;

    // Choose enemy list based on entity type
    BattleEntitiesMap_t entityList;
    if (std::dynamic_pointer_cast<BattleUnit>(entity))
        entityList = m_machine->get_stage().lock()->get_enemies();
    else
        entityList = m_machine->get_stage().lock()->get_units();

    // Collect valid targets in damage zone
    for (auto& pair : entityList)
    {
        auto& enemyList = pair.second;
        for (auto& enemy : enemyList)
        {
            bool isInRange = entity->damageZone.findIntersection(enemy->hitbox).has_value();
            if (isInRange && enemy->stateMachine->get_active_state_id() != "KNOCKBACK")
            {
                entity->targets.insert(enemy);
            }
        }
    }

    // ===== No Targets? =====
    if (entity->targets.empty())
    {
        // Doesn't apply damage but still does the attack animation (attack missed)
        return;
    }

    // ===== Apply Damage and On-Hit Abilities =====
    // The DamageCalculator decides the amount, the target only applies it.
    // Then the StatusManager rolls the attacker's on-hit abilities (the status affects the next hits).
    if (auto stage = m_machine->get_stage().lock())
    {
        const DamageCalculator& damageCalculator = stage->get_damage_calculator();
        StatusManager& statusManager = stage->get_status_manager();
        int hitCount = 0;
        bool hasHitBase = false;

        auto hit = [&](BattleEntity& target)
            {
                const DamageResult result = damageCalculator.calculate(*entity, target);
                target.take_damage(result);
                stage->get_combat_log().record(*entity, target, result, stage->get_elapsed_time());
                statusManager.apply_on_hit_abilities(*entity, target, stage->get_elapsed_time());
                ++hitCount;
                if (dynamic_cast<BattleBase*>(&target)) hasHitBase = true;
            };

        if (entity->data->attackType == 1) // Single-target attack
        {
            auto target = entity->targets.begin()->lock();
            if (target)
                hit(*target);
        }
        else // Area attack (all targets in damage zone)
        {
            for (auto& targetPtr : entity->targets)
            {
                if (auto target = targetPtr.lock())
                    hit(*target);
            }
        }

        // One sound per attack, even when an area attack hits several targets
        if (hitCount > 0) stage->on_attack_landed(*entity, hasHitBase);
    }

    // Clear targets
    entity->targets.clear();
}

// ==================
// Exit
// ==================
void AttackState::exit()
{
    // Reset attack flags on exit
    m_isAttackReady = false;
    m_hasAttacked = false;
    m_currentAttackSwingTime = 0.0f;

    auto entity = m_machine->get_owner().lock();
    if(entity)
        entity->targets.clear();

    // std::cout << "Exiting Attack State\n";
}

// ==================
// Identifier
// ==================
std::string AttackState::get_state_id() const
{
    return "ATTACK";
}
