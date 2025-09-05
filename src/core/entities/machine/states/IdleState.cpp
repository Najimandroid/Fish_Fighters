#include "IdleState.h"

#include "../StateMachine.h"
#include "../../BattleUnit.h"
#include "../../../stages/Stage.h"

#include "KnockbackState.h"
#include "WalkState.h"
#include "AttackState.h"


#include <iostream>

IdleState::IdleState(std::shared_ptr<StateMachine> machine)
{
	m_machine = machine;
}

void IdleState::enter()
{
	std::cout << "Entering Idle State\n";
}

void IdleState::perform(float deltaTime)
{
    auto entity = m_machine->get_owner();

    // Check for knockback / death
    if (entity->currentHealth <= 0.0f || entity->currentHealth <= entity->healthLeftBeforeNextKnockback)
    {
        m_machine->change_state(std::make_unique<KnockbackState>(m_machine));
        return;
    }

    // Check if there are targets
    if (!entity->targets.empty())
    {
        // Stay in Idle until attack cooldown is ready
        if (entity->currentAttackCooldown >= entity->data->attackFrequency)
        {
            m_machine->change_state(std::make_unique<AttackState>(m_machine));
        }
        return; // Do not go back to Walk
    }

    // If no targets in range, scan for enemies
    BattleEntitiesMap_t entityList;
    if (std::dynamic_pointer_cast<BattleUnit>(entity))
        entityList = m_machine->get_stage()->get_enemies();
    else
        entityList = m_machine->get_stage()->get_units();

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

    // Decide next state
    if (m_areEntitiesOnRange)
    {
        if (entity->currentAttackCooldown >= entity->data->attackFrequency)
            m_machine->change_state(std::make_unique<AttackState>(m_machine));
        // else stay Idle
    }
    else
    {
        m_machine->change_state(std::make_unique<WalkState>(m_machine));
    }
}


void IdleState::exit()
{
	std::cout << "Exiting Idle State\n";
}

std::string IdleState::get_state_id() const
{
	return "IDLE";
}
