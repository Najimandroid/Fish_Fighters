#include "BattleUnit.h"

#include <iostream>

// ==================
// Constructor
// ==================
BattleUnit::BattleUnit(std::shared_ptr<EntityData> data_, sf::Vector2f magnification_)
{
    // ===== Core Stats =====
    data = data_;
    magnification = magnification_;

    // Scale health by magnification
    currentHealth = data->health * magnification.x;

    // Determine first knockback threshold
    healthLeftBeforeNextKnockback =
        data->knockbackCount > 0 ? data->health - (data->health / data->knockbackCount) : 0.f;

    // Initial spawn position (hardcoded: right side of screen, bottom 2/3)
    position = { 1920.f - 200.f, 1080.f * 2 / 3 };

    // Start with attack cooldown ready (so unit can attack instantly)
    currentAttackCooldown = data->attackFrequency;

    // ===== Sprite & Texture =====
    bool isTextureLoaded = texture.loadFromFile(data->texture);
    sprite.setTexture(texture, true);

    // Origin at sprite center (x) and bottom (y)
    sprite.setOrigin({
        static_cast<float>(texture.getSize().x / data->frameCount / 2),
        static_cast<float>(texture.getSize().y)
        });

    // Initialize first frame of animation
    currentFrameIndex = 0;
    sprite.setTextureRect({
        { static_cast<int>(texture.getSize().x / data->frameCount * currentFrameIndex), 0 },
        { static_cast<int>(texture.getSize().x / data->frameCount), static_cast<int>(texture.getSize().y) }
        });

    // ===== Combat Zones =====
    // Hitbox: half the sprite width, full vertical coverage
    hitbox.size = {
        static_cast<float>(texture.getSize().x / data->frameCount / 2),
        1080.f
    };

    // Attack and damage zones: extend forward by attack range
    attackRangeZone.size = { data->attackRange + hitbox.size.x, 1080.f };
    damageZone.size = { data->attackRange + hitbox.size.x, 1080.f };

    // ===== Initial Positions =====
    hitbox.position = position;
    attackRangeZone.position = { position.x - data->attackRange + hitbox.size.x, position.y };
    damageZone.position = { position.x - data->attackRange + hitbox.size.x, position.y };

#ifdef DEBUG_MODE
    // ===== Debug Rendering =====
    rHitbox.setSize(hitbox.size);
    rHitbox.setPosition(position);

    rAttackRangeZone.setSize(attackRangeZone.size);
    rAttackRangeZone.setPosition(position);

    rDamageZone.setSize(damageZone.size);
    rDamageZone.setPosition(position);
#endif
}

// ==================
// Destructor
// ==================
BattleUnit::~BattleUnit()
{
    // Optional debug
    // std::cout << "BattleUnit Destructor called\n";
}

// ==================
// Update
// ==================
void BattleUnit::update(
    float deltaTime,
    const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList
)
{
    // Advance state machine (handles attack/walk/idle transitions)
    stateMachine->update_state(deltaTime);

    // Update position (including knockback and hitboxes)
    update_position();
    // Update sprite animation
    update_sprite();

    // Progress attack and animation timers
    currentAttackCooldown += deltaTime;
    currentFrameCooldown += deltaTime;
}

// ==================
// Position Update
// ==================
void BattleUnit::update_position()
{
    if (stateMachine->get_active_state_id() == "KNOCKBACK")
    {
        // Knockback uses tween animations
        if (tweenX.progress() < 1.0f && tweenY.progress() < 1.0f)
        {
            sprite.setPosition({ tweenX.step(1), tweenY.step(1) });
        }
    }
    else
    {
        // Normal position: x,y minus current layer (layer = depth offset)
        sprite.setPosition({ position.x, position.y - static_cast<float>(currentLayer) });
    }

    // Sync combat zones with position
    hitbox.position = position;
    attackRangeZone.position = { position.x - data->attackRange, position.y };
    damageZone.position = { position.x - data->attackRange, position.y };

#ifdef DEBUG_MODE
    // Update debug rectangles to follow entity
    rHitbox.setPosition(hitbox.position);
    rAttackRangeZone.setPosition(attackRangeZone.position);
    rDamageZone.setPosition(damageZone.position);
#endif
}
