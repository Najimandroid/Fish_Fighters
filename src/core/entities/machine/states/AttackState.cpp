#include "AttackState.h"
#include "../StateMachine.h"
#include "../../BattleUnit.h"
#include "../../../stages/Stage.h"

#include "KnockbackState.h"
#include "WalkState.h"
#include "IdleState.h"

#include <iostream>

AttackState::AttackState(std::shared_ptr<StateMachine> machine)
{
	m_machine = machine;
}

void AttackState::enter()
{
	std::cout << "Entering Attack State\n";
}

void AttackState::perform(float deltaTime)
{
	auto entity = m_machine->get_owner();

	if (entity->currentAttackCooldown < entity->data->attackFrequency)
	{
		m_machine->change_state(std::make_unique<IdleState>(m_machine));
		return;
	}

	if (entity->currentHealth <= 0.0f) { m_machine->change_state(std::make_unique<KnockbackState>(m_machine)); return; }
	if (entity->currentHealth <= entity->healthLeftBeforeNextKnockback) { m_machine->change_state(std::make_unique<KnockbackState>(m_machine)); return; }

	if (m_currentAttackSwingTime <= entity->data->foreswingTime + entity->data->backswingTime)
	{
		if (m_currentAttackSwingTime >= entity->data->foreswingTime) m_isAttackReady = true;
		else m_isAttackReady = false;

		m_currentAttackSwingTime += deltaTime;
	}
	else
	{
		m_machine->change_state(std::make_unique<IdleState>(m_machine));
		return;
	}

	//Attack
	if (!m_isAttackReady || m_hasAttacked) return;
	m_hasAttacked = true;

	//Get targets in damage zone
	BattleEntitiesMap_t entityList;
	if (std::dynamic_pointer_cast<BattleUnit>(entity)) entityList = m_machine->get_stage()->get_enemies();
	else entityList = m_machine->get_stage()->get_units();

	for(auto& pair : entityList)
	{
		auto& enemyList = pair.second;

		for (auto& enemy : enemyList)
		{
			bool isUnitInAttackRange = entity->damageZone.findIntersection(enemy->hitbox).has_value();
			if (isUnitInAttackRange && enemy->stateMachine->get_active_state_id() != "KNOCKBACK")
			{
				//Enemy found
				entity->targets.insert(enemy);
			}
		}
	}

	if (entity->targets.empty()) { m_machine->change_state(std::make_unique<WalkState>(m_machine)); return; }

	//Attack types
	if(entity->data->attackType == 1) //if attackType => single
	{
		auto target = entity->targets.begin().operator*().lock();

		if (target)
			target->currentHealth -= entity->data->attackPower * entity->magnification.y; //Deal damage to the first target
	}
	else
	{
		for (auto& targetPtr : entity->targets)
		{
			if (auto target = targetPtr.lock())
			{
				target->currentHealth -= entity->data->attackPower * entity->magnification.y; //Deal damage to all targets
			}
		}
	}

	entity->currentAttackCooldown = 0.0f;
	entity->targets.clear();
}

void AttackState::exit()
{
	std::cout << "Exiting Attack State\n";
}

std::string AttackState::get_state_id() const
{
	return "ATTACK";
}
