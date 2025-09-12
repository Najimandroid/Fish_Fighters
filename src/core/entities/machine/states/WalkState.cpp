#include "WalkState.h"
#include "../StateMachine.h"
#include "../../BattleUnit.h"
#include "../../../stages/Stage.h"

#include "KnockbackState.h"
#include "IdleState.h"
#include "AttackState.h"

#include <iostream>

// ==================
// Constructor
// ==================
WalkState::WalkState(std::shared_ptr<StateMachine> machine)
{
    m_machine = machine;
}

// ==================
// Enter
// ==================
void WalkState::enter()
{
    m_areEntitiesOnRange = false;
    // std::cout << "Entering Walk State\n";
}

// ==================
// Perform
// ==================
void WalkState::perform(float deltaTime)
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

    // ===== Scan for Enemies =====
    m_areEntitiesOnRange = false;

    // If already has targets, assume enemies are nearby
    if (!entity->targets.empty())
    {
        m_areEntitiesOnRange = true;
    }
    else
    {
        // Otherwise, check attack range zone
        BattleEntitiesMap_t entityList;
        if (std::dynamic_pointer_cast<BattleUnit>(entity))
            entityList = m_machine->get_stage().lock()->get_enemies();
        else
            entityList = m_machine->get_stage().lock()->get_units();

        for (auto& pair : entityList)
        {
            for (auto& enemy : pair.second)
            {
                bool isInRange = entity->attackRangeZone.findIntersection(enemy->hitbox).has_value();
                if (isInRange)
                {
                    m_areEntitiesOnRange = true;
                    break;
                }
            }
            if (m_areEntitiesOnRange) break;
        }
    }

    // ===== Decide Next State =====
    if (m_areEntitiesOnRange)
    {
        if (entity->currentAttackCooldown >= entity->data->attackFrequency)
        {
            m_machine->change_state(std::make_unique<AttackState>(m_machine));
        }
        else
        {
            m_machine->change_state(std::make_unique<IdleState>(m_machine));
        }
        return;
    }

    // ===== Keep Walking =====
    entity->velocity = { entity->data->movementSpeed * 10.0f * deltaTime, 0.0f };

    if (auto unit = std::dynamic_pointer_cast<BattleUnit>(entity))
        entity->position -= entity->velocity; // Units -> move left
    else
        entity->position += entity->velocity; // Enemies -> move right
}

// ==================
// Exit
// ==================
void WalkState::exit()
{
    m_areEntitiesOnRange = false;
    // std::cout << "Exiting Walk State\n";
}

// ==================
// Identifier
// ==================
std::string WalkState::get_state_id() const
{
    return "WALK";
}
