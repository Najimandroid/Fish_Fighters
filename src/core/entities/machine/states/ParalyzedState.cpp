#include "ParalyzedState.h"
#include "../StateMachine.h"
#include "../../BattleEntity.h"

#include "KnockbackState.h"
#include "IdleState.h"

// ==================
// Constructor
// ==================
ParalyzedState::ParalyzedState(StateMachine* machine)
{
    m_machine = machine;
}

// ==================
// Enter
// ==================
void ParalyzedState::enter()
{
    auto entity = m_machine->get_owner().lock();
    if (!entity) return;

    // Cancels any attack in progress
    entity->targets.clear();
}

// ==================
// Perform
// ==================
void ParalyzedState::perform(float deltaTime)
{
    auto entity = m_machine->get_owner().lock();
    if (!entity) return;

    // ===== Death / Knockback Check =====
    // Paralyzed entities can still be knocked back and die
    if (entity->currentHealth <= 0.0f ||
        entity->currentHealth <= entity->healthLeftBeforeNextKnockback)
    {
        m_machine->change_state(std::make_unique<KnockbackState>(m_machine));
        return;
    }

    // ===== Paralysis Over? =====
    // The StatusManager removes the status when its duration runs out
    if (!entity->get_status(StatusType::PARALYZED))
    {
        m_machine->change_state(std::make_unique<IdleState>(m_machine));
    }
}

// ==================
// Exit
// ==================
void ParalyzedState::exit()
{
}

// ==================
// Identifier
// ==================
std::string ParalyzedState::get_state_id() const
{
    return "PARALYZED";
}
