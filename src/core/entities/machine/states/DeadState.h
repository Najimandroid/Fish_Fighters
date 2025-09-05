#pragma once

#include "BaseState.h"

class DeadState : public BaseState
{
public:

	DeadState(std::shared_ptr<StateMachine> machine);

	void enter() override;
	void perform(float deltaTime) override;
	void exit() override;

	std::string get_state_id() const;
};