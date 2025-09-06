#include "KnockbackState.h"
#include "../StateMachine.h"

#include "../../BattleUnit.h"

#include "DeadState.h"
#include "IdleState.h"

#include <iostream>

KnockbackState::KnockbackState(std::shared_ptr<StateMachine> machine)
{
	m_machine = machine;
}

void KnockbackState::enter()
{
	//std::cout << "Entering Knockback State\n";

	auto entity = m_machine->get_owner().lock();

	if (std::dynamic_pointer_cast<BattleUnit>(entity))
		entity->tweenX = tweeny::from(entity->position.x).to(entity->position.x += m_knockbackDistancePx).during(60.0f * m_knockbackDuration).via(tweeny::easing::quadraticOut);
	else 
		entity->tweenX = tweeny::from(entity->position.x).to(entity->position.x -= m_knockbackDistancePx).during(60.0f * m_knockbackDuration).via(tweeny::easing::quadraticOut);
	
	entity->tweenY = tweeny::from(entity->position.y - static_cast<float>(entity->currentLayer)).to(entity->position.y - static_cast<float>(entity->currentLayer) - 50.0f)
		.during(30.0f * m_knockbackDuration).via(tweeny::easing::quadraticOut)
		.to(entity->position.y - static_cast<float>(entity->currentLayer)).during(30.0f * m_knockbackDuration).via(tweeny::easing::bounceOut);
}

void KnockbackState::perform(float deltaTime)
{
	auto entity = m_machine->get_owner().lock();

	m_currentKnockbackCooldown += deltaTime;

	if (m_currentKnockbackCooldown >= m_knockbackDuration)
	{
		if(entity->currentHealth <= 0.0f)
		{
			m_machine->change_state(std::make_unique<DeadState>(m_machine));
			return;
			//std::cout << "Knockback led to death.\n";
		}
		else
		{
			m_machine->change_state(std::make_unique<IdleState>(m_machine));
			return;
		}
	}
}

void KnockbackState::exit()
{
	auto entity = m_machine->get_owner().lock();

	if (entity->isOnShockwave == false)
	{
		float step = entity->data->health / entity->data->knockbackCount;

		while (entity->healthLeftBeforeNextKnockback >= entity->currentHealth && entity->healthLeftBeforeNextKnockback > 0.0f)
			entity->healthLeftBeforeNextKnockback -= step;
	}
	else
		entity->isOnShockwave = false;

	//std::cout << "Exiting Knockback State\n";
}

std::string KnockbackState::get_state_id() const
{
	return "KNOCKBACK";
}
