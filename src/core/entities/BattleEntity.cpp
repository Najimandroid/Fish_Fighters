#include "BattleEntity.h"

#include <iostream>

// ==================
// Constructor
// ==================
BattleEntity::BattleEntity() :

    // Initialize collision zones with default dimensions
    hitbox(sf::FloatRect(position, { 1.0f, 720.0f })),
    attackRangeZone(sf::FloatRect(position, { 1.0f, 720.0f })),
    damageZone(sf::FloatRect(position, { 1.0f, 720.0f })),

    // Initialize render data
    texture(sf::Texture()),
    sprite(sf::Sprite(texture)),

    // Initialize state machine
    stateMachine(std::make_shared<StateMachine>())
{
#ifdef DEBUG_MODE
    // Debug rendering: show hitbox and ranges
    rHitbox.setPosition(position);
    rHitbox.setSize(hitbox.size);
    rHitbox.setOutlineColor(sf::Color::Blue);
    rHitbox.setFillColor(sf::Color::Transparent);
    rHitbox.setOutlineThickness(1.f);

    rAttackRangeZone.setPosition(position);
    rAttackRangeZone.setSize(attackRangeZone.size);
    rAttackRangeZone.setOutlineColor(sf::Color::Yellow);
    rAttackRangeZone.setFillColor(sf::Color::Transparent);
    rAttackRangeZone.setOutlineThickness(2.f);

    rDamageZone.setPosition(position);
    rDamageZone.setSize(damageZone.size);
    rDamageZone.setOutlineColor(sf::Color::Red);
    rDamageZone.setFillColor(sf::Color::Transparent);
    rDamageZone.setOutlineThickness(3.f);
#endif
}

// ==================
// Destructor
// ==================
BattleEntity::~BattleEntity()
{
    // Optional: debug logging when entities are destroyed
    // std::cout << "BattleEntity Destructor called\n";
}

// ==================
// Initialization
// ==================
void BattleEntity::init_state_machine()
{
    // Link the state machine to this entity
    stateMachine->set_owner(shared_from_this());
}

void BattleEntity::set_current_stage(std::shared_ptr<Stage> stage)
{
    // Store reference to the current stage (weak to avoid circular refs)
    currentStage = stage;
}

// ==================
// Target Ordering
// ==================
// Used for sorting entities in the target set based on their X position
bool BattleEntity::WeakPtrPosXLess::operator()(const std::weak_ptr<BattleEntity>& a, const std::weak_ptr<BattleEntity>& b) const
{
    auto aptr = a.lock();
    auto bptr = b.lock();

    // If both are expired, consider them equal
    if (!aptr && !bptr) return false;
    // If only one exists, the valid one is considered "smaller"
    if (!aptr) return false;
    if (!bptr) return true;

    // Compare positions along the X-axis
    if (aptr->position.x < bptr->position.x) return true;
    if (aptr->position.x > bptr->position.x) return false;

    // If positions are equal, compare raw pointers for consistency
    return aptr.get() < bptr.get();
}

// ==================
// Sprite Animation
// ==================
void BattleEntity::update_sprite()
{
    // The knockback animation bypasses cooldowns
    if (currentFrameCooldown >= timeUntilNextFrame ||
        stateMachine->get_active_state_id() == "KNOCKBACK")
    {
        currentFrameCooldown = 0.0f;
    }
    else
    {
        return; // Not time to update frame yet
    }

    // Update animation depending on current state
    if (stateMachine->get_active_state_id() == "IDLE")
    {
        currentFrameIndex = 0;
    }
    else if (stateMachine->get_active_state_id() == "WALK")
    {
        // Cycle through walking frames
        if (currentFrameIndex < data->knockbackFrameIndex - 1)
            currentFrameIndex++;
        else
            currentFrameIndex = 0; // Loop back
    }
    else if (stateMachine->get_active_state_id() == "ATTACK")
    {
        // Attack frames start from knockback frame index
        if (currentFrameIndex <= data->knockbackFrameIndex)
            currentFrameIndex = data->knockbackFrameIndex;

        // Advance frames if within texture bounds
        if (currentFrameIndex * sprite.getTextureRect().size.x <  texture.getSize().x - sprite.getTextureRect().size.x)
        {
            currentFrameIndex++;
        }
        else
        {
            currentFrameIndex = 0; // Reset to idle
        }
    }
    else if (stateMachine->get_active_state_id() == "KNOCKBACK")
    {
        // Shockwave forces idle frame, otherwise knockback frame
        if (isOnShockwave)
            currentFrameIndex = 0;
        else
            currentFrameIndex = data->knockbackFrameIndex;
    }

    // Apply the new texture rectangle for the current frame
    sprite.setTextureRect({
        { static_cast<int>(texture.getSize().x / data->frameCount * currentFrameIndex), 0 },
        { static_cast<int>(texture.getSize().x / data->frameCount), static_cast<int>(texture.getSize().y) }
        });
}
