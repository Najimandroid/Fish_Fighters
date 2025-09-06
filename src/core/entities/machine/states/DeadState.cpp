#include "DeadState.h"
#include "../StateMachine.h"
#include "../../BattleEntity.h"

#include <iostream>

DeadState::DeadState(std::shared_ptr<StateMachine> machine)
{
	m_machine = machine;
}

void DeadState::enter()
{
	//std::cout << "Entering Dead State\n";

	m_machine->get_owner().lock()->isDead = true;
}

void DeadState::perform(float deltaTime)
{
	//Entity is dead, do nothing
}

void DeadState::exit()
{
	//std::cout << "Exiting Dead State\n";
}

std::string DeadState::get_state_id() const
{
	return "DEAD";
}