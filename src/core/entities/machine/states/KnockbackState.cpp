#include "KnockbackState.h"
#include "../StateMachine.h"
#include "../../BattleUnit.h"

#include "DeadState.h"
#include "IdleState.h"

#include <iostream>

// ==================
// Constructor
// ==================
KnockbackState::KnockbackState(std::shared_ptr<StateMachine> machine)
{
    m_machine = machine;
}

// ==================
// Enter
// ==================
void KnockbackState::enter()
{
    // std::cout << "Entering Knockback State\n";

    auto entity = m_machine->get_owner().lock();
    if (!entity) return;

    // Clearing all the targets
    entity->targets.clear();

    // Convert duration in seconds to milliseconds for tweeny::during (expects integer-like durations)
    uint32_t totalMs = static_cast<uint32_t>(m_KNOCKBACK_DURATION * 1000.0f);
    uint32_t halfMs = totalMs / 2;

    // Horizontal knockback:
    // -> Units knocked back to the right
    // -> Enemies knocked back to the left
    if (std::dynamic_pointer_cast<BattleUnit>(entity))
        entity->tweenX = tweeny::from(entity->position.x)
        .to(entity->position.x += m_KNOCKBACK_DISTANCE_PX)
        .during(static_cast<int>(totalMs))
        .via(tweeny::easing::quadraticOut);
    else
        entity->tweenX = tweeny::from(entity->position.x)
        .to(entity->position.x -= m_KNOCKBACK_DISTANCE_PX)
        .during(static_cast<int>(totalMs))
        .via(tweeny::easing::quadraticOut);

    // Vertical bounce effect (use integer ms durations)
    entity->tweenY = tweeny::from(entity->position.y - static_cast<float>(entity->currentLayer))
        .to(entity->position.y - static_cast<float>(entity->currentLayer) - 50.0f)
        .during(static_cast<int>(halfMs)).via(tweeny::easing::quadraticOut)
        .to(entity->position.y - static_cast<float>(entity->currentLayer))
        .during(static_cast<int>(halfMs)).via(tweeny::easing::bounceOut);
}

// ==================
// Perform
// ==================
void KnockbackState::perform(float deltaTime)
{
    auto entity = m_machine->get_owner().lock();
    if (!entity) return;

    m_currentKnockbackCooldown += deltaTime;

    // Step using milliseconds (using the tweeny overload step(int32_t))
    int32_t dt_ms = static_cast<int32_t>(deltaTime * 1000.0f);
    entity->tweenX.step(dt_ms);
    entity->tweenY.step(dt_ms);

    // Once knockback duration has passed, decide next state
    if (m_currentKnockbackCooldown >= m_KNOCKBACK_DURATION)
    {
        if (entity->currentHealth <= 0.0f)
			m_machine->change_state(std::make_unique<DeadState>(m_machine)); // -> death
        else
			m_machine->change_state(std::make_unique<IdleState>(m_machine)); // -> idle recovery
    }
}

// ==================
// Exit
// ==================
void KnockbackState::exit()
{
    auto entity = m_machine->get_owner().lock();
    if (!entity) return;

    if (!entity->isOnShockwave)
    {
        // Reduce health threshold for the next knockback
        float step = entity->data->knockbackCount > 0
            ? entity->data->health / entity->data->knockbackCount
            : 0.f;

        // Decrement until threshold is below current health
        while (entity->healthLeftBeforeNextKnockback >= entity->currentHealth &&
            entity->healthLeftBeforeNextKnockback > 0.0f)
        {
            entity->healthLeftBeforeNextKnockback -= step;
        }
    }
    else
    {
        // Shockwave knockbacks skip normal decrement
        entity->isOnShockwave = false;
    }

    // std::cout << "Exiting Knockback State\n";
}

// ==================
// Identifier
// ==================
std::string KnockbackState::get_state_id() const
{
    return "KNOCKBACK";
}
