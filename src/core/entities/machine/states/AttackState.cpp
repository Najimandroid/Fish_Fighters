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
AttackState::AttackState(std::shared_ptr<StateMachine> machine)
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
    // Progress foreswing -> attack -> backswing
    if (m_currentAttackSwingTime <= entity->data->foreswingTime + entity->data->backswingTime)
    {
        // Attack becomes "ready" after foreswing
        m_isAttackReady = (m_currentAttackSwingTime >= entity->data->foreswingTime);
        m_currentAttackSwingTime += deltaTime;
    }
    else
    {
        // After swing ends, return to idle
        m_machine->change_state(std::make_unique<IdleState>(m_machine));
        return;
    }

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
        m_machine->change_state(std::make_unique<WalkState>(m_machine));
        return;
    }

    // ===== Apply Damage =====
    if (entity->data->attackType == 1) // Single-target attack
    {
        auto target = entity->targets.begin()->lock();
        if (target)
            target->currentHealth -= entity->data->attackPower * entity->magnification.y;
    }
    else // Area attack (all targets in damage zone)
    {
        for (auto& targetPtr : entity->targets)
        {
            if (auto target = targetPtr.lock())
                target->currentHealth -= entity->data->attackPower * entity->magnification.y;
        }
    }

    // Reset attack cooldown and clear targets
    entity->currentAttackCooldown = 0.0f;
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

    // std::cout << "Exiting Attack State\n";
}

// ==================
// Identifier
// ==================
std::string AttackState::get_state_id() const
{
    return "ATTACK";
}
