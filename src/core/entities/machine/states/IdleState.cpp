#include "IdleState.h"

#include "../StateMachine.h"
#include "../../BattleUnit.h"
#include "../../../stages/Stage.h"

#include "KnockbackState.h"
#include "WalkState.h"
#include "AttackState.h"

#include <iostream>

// ==================
// Constructor
// ==================
IdleState::IdleState(std::shared_ptr<StateMachine> machine)
{
    m_machine = machine;
}

// ==================
// Enter
// ==================
void IdleState::enter()
{
    // Reset flags when entering idle
    m_areEntitiesOnRange = false;

    // std::cout << "Entering Idle State\n";
}

// ==================
// Perform
// ==================
void IdleState::perform(float deltaTime)
{
    auto entity = m_machine->get_owner().lock();
    if (!entity) return;

    // ===== Death / Knockback Check =====
    if (entity->currentHealth <= 0.0f ||
        entity->currentHealth <= entity->healthLeftBeforeNextKnockback)
    {
        m_machine->change_state(std::make_unique<KnockbackState>(m_machine));
        return;
    }

    // ===== Already Has Targets? =====
    if (!entity->targets.empty())
    {
        // If cooldown is ready -> attack
        if (entity->currentAttackCooldown >= entity->data->attackFrequency)
        {
            m_machine->change_state(std::make_unique<AttackState>(m_machine));
        }
        return; // Otherwise remain idle
    }

    // ===== Scan for Enemies =====
    BattleEntitiesMap_t entityList;
    if (std::dynamic_pointer_cast<BattleUnit>(entity))
        entityList = m_machine->get_stage().lock()->get_enemies();
    else
        entityList = m_machine->get_stage().lock()->get_units();

    m_areEntitiesOnRange = false;
    for (auto& pair : entityList)
    {
        for (auto& enemy : pair.second)
        {
            if (entity->attackRangeZone.findIntersection(enemy->hitbox).has_value())
            {
                m_areEntitiesOnRange = true;
                break;
            }
        }
        if (m_areEntitiesOnRange) break;
    }

    // ===== Decide Next State =====
    if (m_areEntitiesOnRange)
    {
        if (entity->currentAttackCooldown >= entity->data->attackFrequency)
            m_machine->change_state(std::make_unique<AttackState>(m_machine));
        // else -> remain Idle until cooldown ready
    }
    else
    {
        m_machine->change_state(std::make_unique<WalkState>(m_machine));
    }
}

// ==================
// Exit
// ==================
void IdleState::exit()
{
    m_areEntitiesOnRange = false;
    // std::cout << "Exiting Idle State\n";
}

// ==================
// Identifier
// ==================
std::string IdleState::get_state_id() const
{
    return "IDLE";
}
