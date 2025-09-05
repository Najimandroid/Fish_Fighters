#pragma once

#include <memory>
#include <string>

class StateMachine;
class BattleEntity;

class BaseState
{
public:

	BaseState() = default;
	BaseState(std::shared_ptr<StateMachine> machine);
	virtual ~BaseState();

	virtual void enter() = 0;
	virtual void perform(float deltaTime) = 0;
	virtual void exit() = 0;

	virtual std::string get_state_id() const = 0;

protected:

	std::shared_ptr<StateMachine> m_machine = nullptr;
};