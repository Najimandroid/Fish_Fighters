#pragma once

#include "BaseState.h"

class KnockbackState : public BaseState
{
public:

	KnockbackState(std::shared_ptr<StateMachine> machine);

	void enter() override;
	void perform(float deltaTime) override;
	void exit() override;

	std::string get_state_id() const;

private:

	float m_currentKnockbackCooldown = 0.0f;

	float m_knockbackDuration = 1.0f; //knockback duration in seconds
	float m_knockbackDistancePx = 150.0f; //knockback distance in pixels
};