#include "BaseState.h"
#include "../StateMachine.h"

// ==================
// Constructor
// ==================
BaseState::BaseState(std::shared_ptr<StateMachine> machine)
{
    m_machine = machine;
}
