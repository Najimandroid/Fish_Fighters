#pragma once

#include "BaseState.h"

class AttackState : public BaseState
{
public:

	AttackState(std::shared_ptr<StateMachine> machine);

	void enter() override;
	void perform(float deltaTime) override;
	void exit() override;

	std::string get_state_id() const;

private:

	bool m_isAttackReady = false;
	bool m_hasAttacked = false;

	float m_currentAttackSwingTime = 0.0f;
};