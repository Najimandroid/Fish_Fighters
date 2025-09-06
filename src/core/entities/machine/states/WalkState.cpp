#include "WalkState.h"
#include "../StateMachine.h"
#include "../../BattleUnit.h"
#include "../../../stages/Stage.h"

#include "KnockbackState.h"
#include "IdleState.h"
#include "AttackState.h"

#include <iostream>

WalkState::WalkState(std::shared_ptr<StateMachine> machine)
{
	m_machine = machine;
}

void WalkState::enter()
{
	//std::cout << "Entering Walk State\n";
}

void WalkState::perform(float deltaTime)
{
	auto entity = m_machine->get_owner().lock();

	if (entity->currentHealth <= 0.0f) { m_machine->change_state(std::make_unique<KnockbackState>(m_machine)); return; }
	if (entity->currentHealth <= entity->healthLeftBeforeNextKnockback) { m_machine->change_state(std::make_unique<KnockbackState>(m_machine)); return; }

	//Check the attack range
	if (!entity->targets.empty()) m_areEntitiesOnRange = true;
	else
	{
		BattleEntitiesMap_t entityList;
		if (std::dynamic_pointer_cast<BattleUnit>(entity)) entityList = m_machine->get_stage().lock()->get_enemies();
		else entityList = m_machine->get_stage().lock()->get_units();

		for (auto& pair : entityList)
		{

			auto& enemyList = pair.second;

			for (auto& enemy : enemyList)
			{
				bool isUnitInAttackRange = entity->attackRangeZone.findIntersection(enemy->hitbox).has_value();
				if (isUnitInAttackRange)
				{
					m_areEntitiesOnRange = true;
					break;
				}
			}
		}
	}

	if (entity->currentAttackCooldown >= entity->data->attackFrequency && m_areEntitiesOnRange == true)
	{
		m_machine->change_state(std::make_unique<AttackState>(m_machine));
		return;
	}
	else if (m_areEntitiesOnRange == true)
	{
		m_machine->change_state(std::make_unique<IdleState>(m_machine));
		return;
	}


	entity->velocity = { entity->data->movementSpeed * 10.0f * deltaTime, 0.0f }; //*10.0f to make them more speedy

	if (auto unit = std::dynamic_pointer_cast<BattleUnit>(entity))
		entity->position -= entity->velocity;
	else
		entity->position += entity->velocity;
}

void WalkState::exit()
{
	//std::cout << "Exiting Walk State\n";
}

std::string WalkState::get_state_id() const
{
	return "WALK";
}
