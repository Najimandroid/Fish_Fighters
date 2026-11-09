#include "BaseState.h"
#include "../StateMachine.h"

// ==================
// Constructor
// ==================
BaseState::BaseState(StateMachine* machine)
{
    m_machine = machine;
}
