#pragma once

#include "BaseState.h"

class WalkState : public BaseState
{
public:

	WalkState(std::shared_ptr<StateMachine> machine);

	void enter() override;
	void perform(float deltaTime) override;
	void exit() override;
	
	std::string get_state_id() const;

private:

	bool m_areEntitiesOnRange = false;
};