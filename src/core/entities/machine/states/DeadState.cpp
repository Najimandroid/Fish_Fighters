#include "DeadState.h"
#include "../StateMachine.h"
#include "../../BattleEntity.h"

#include <iostream>

// ==================
// Constructor
// ==================
DeadState::DeadState(std::shared_ptr<StateMachine> machine)
{
    m_machine = machine;
}

// ==================
// Enter
// ==================
void DeadState::enter()
{
    // std::cout << "Entering Dead State\n";

    auto entity = m_machine->get_owner().lock();
    if (entity)
        entity->isDead = true; // -> Mark as dead
}

// ==================
// Perform
// ==================
void DeadState::perform(float deltaTime)
{
    // Dead entities perform no actions
}

// ==================
// Exit
// ==================
void DeadState::exit()
{
    // Normally entities should never exit the Dead state.
    // std::cout << "Exiting Dead State\n";
}

// ==================
// Identifier
// ==================
std::string DeadState::get_state_id() const
{
    return "DEAD";
}
