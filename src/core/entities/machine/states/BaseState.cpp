#include "BaseState.h"
#include "../StateMachine.h"

BaseState::BaseState(std::shared_ptr<StateMachine> machine)
{
	m_machine = machine;
}

BaseState::~BaseState()
{
	m_machine = nullptr;
}