#pragma once

#include "states/BaseState.h"

#include <memory>
#include <string>

class BattleEntity;
class Stage;

class StateMachine
{
public:

	void update_state(float deltaTime);
	void set_owner(std::shared_ptr<BattleEntity> owner);
	void change_state(std::unique_ptr<BaseState> newState);

	std::shared_ptr<BattleEntity> get_owner() const;
	std::shared_ptr<Stage> get_stage() const;	
	std::string get_active_state_id() const;

private:

	std::shared_ptr<BattleEntity> m_owner = nullptr;

	std::shared_ptr<Stage> m_stage = nullptr;

	std::unique_ptr<BaseState> m_activeState = nullptr;
};