#include "StateMachine.h"

#include "../BattleEntity.h"
#include "../../stages/Stage.h"

void StateMachine::update_state(float deltaTime)
{
	if(m_activeState)
		m_activeState->perform(deltaTime);
}

void StateMachine::set_owner(std::shared_ptr<BattleEntity> owner)
{
    m_owner = owner;
	m_stage = owner->currentStage;
}

void StateMachine::change_state(std::unique_ptr<BaseState> newState)
{
    if (m_activeState)
    {
        m_activeState->exit();
    }

    m_activeState = std::move(newState);

    if (m_activeState)
        m_activeState->enter();
}

std::weak_ptr<BattleEntity> StateMachine::get_owner() const
{
	return m_owner;
}

std::weak_ptr<Stage> StateMachine::get_stage() const
{
    return m_stage;
}

std::string StateMachine::get_active_state_id() const
{
    return m_activeState->get_state_id();
}
