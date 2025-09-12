#pragma once

#include "states/BaseState.h"

#include <memory>
#include <string>

class BattleEntity;
class Stage;

/*
 * StateMachine
 * ------------
 * Manages the active state of a BattleEntity (e.g. IDLE, WALK, ATTACK, KNOCKBACK).
 *
 * Responsibilities:
 *  - Owns the current state (BaseState).
 *  - Delegates update calls to the active state.
 *  - Handles transitions between states (exit -> change -> enter).
 *  - Knows which entity (owner) and stage it belongs to.
 *
 * Lifecycle:
 *  1. The BattleEntity sets itself as the owner.
 *  2. States are changed via change_state().
 *  3. Each update(), the current state's logic is executed.
 */

class StateMachine
{
public:
    // Advance the current state by deltaTime
    void update_state(float deltaTime);

    // Assign an owner entity (BattleUnit, BattleEnemy, or BattleBase)
    void set_owner(std::shared_ptr<BattleEntity> owner);

    // Transition to a new state (handles exit/enter calls automatically)
    void change_state(std::unique_ptr<BaseState> newState);

    // Accessors
    std::weak_ptr<BattleEntity> get_owner() const;
    std::weak_ptr<Stage> get_stage() const;
    std::string get_active_state_id() const;

private:
    // The entity controlled by this state machine
    std::weak_ptr<BattleEntity> m_owner;

    // The stage where the owner exists
    std::weak_ptr<Stage> m_stage;

    // Currently active state (only one at a time)
    std::unique_ptr<BaseState> m_activeState = nullptr;
};
