#include "StateMachine.h"

#include "../BattleEntity.h"
#include "../../stages/Stage.h"

// ==================
// Update State
// ==================
void StateMachine::update_state(float deltaTime)
{
    if (m_activeState)
        m_activeState->perform(deltaTime); // Delegate behavior to active state
}

// ==================
// Set Owner
// ==================
void StateMachine::set_owner(std::shared_ptr<BattleEntity> owner)
{
    m_owner = owner;
    m_stage = owner->currentStage; // Track stage through the owner
}

// ==================
// Change State
// ==================
void StateMachine::change_state(std::unique_ptr<BaseState> newState)
{
    // Exit current state if one exists
    if (m_activeState)
    {
        m_activeState->exit();
    }

    // Replace with new state
    m_activeState = std::move(newState);

    // Enter new state if valid
    if (m_activeState)
        m_activeState->enter();
}

// ==================
// Accessors
// ==================
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
    if (m_activeState == nullptr) return "NONE";
    return m_activeState->get_state_id();
}

