#pragma once

#include "BaseState.h"

/*
 * DeadState
 * ---------
 * Represents a BattleEntity that has died.
 *
 * Responsibilities:
 *  - Marks the entity as dead (isDead = true).
 *  - Disables further actions (perform does nothing).
 *  - Acts as a terminal state — once in DeadState, the entity
 *    does not transition to any other state.
 */

class DeadState : public BaseState
{
public:
    explicit DeadState(std::shared_ptr<StateMachine> machine);

    void enter() override;                       // Mark entity as dead
    void perform(float deltaTime) override;      // No-op (dead entities do nothing)
    void exit() override;                        // (Usually never called)

    std::string get_state_id() const override;   // Always returns "DEAD"
};
