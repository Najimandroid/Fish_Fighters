#pragma once

#include <memory>
#include <string>

class StateMachine;
class BattleEntity;

/*
 * BaseState
 * ---------
 * Abstract interface for a state within the StateMachine.
 *
 * Responsibilities:
 *  - Defines the lifecycle of a state:
 *      - enter()   -> called once when the state becomes active
 *      - perform() -> called each frame while active
 *      - exit()    -> called once when leaving the state
 *  - Provides a unique ID string to identify the state
 *  - Holds a reference to the StateMachine that owns it
 *
 * Usage:
 *  - Inherit from BaseState (e.g. IdleState, AttackState)
 *  - Implement enter(), perform(), exit(), and get_state_id()
 *  - Pass the owning StateMachine to the constructor
 */

class BaseState
{
public:
    BaseState() = default;
    explicit BaseState(std::shared_ptr<StateMachine> machine);

    // ===== Lifecycle =====
    virtual void enter() = 0;                      // Called when state is activated
    virtual void perform(float deltaTime) = 0;     // Called every frame while active
    virtual void exit() = 0;                       // Called when state is deactivated

    // ===== Identification =====
    virtual std::string get_state_id() const = 0;  // Unique state identifier (e.g. "IDLE")

protected:

    std::shared_ptr<StateMachine> m_machine; // State machine the state belongs to
};
