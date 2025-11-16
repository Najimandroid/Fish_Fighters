#include "BattleEnemy.h"

#include <iostream>

// ==================
// Constructor
// ==================
BattleEnemy::BattleEnemy(std::shared_ptr<EntityData> data_, sf::Vector2f magnification_)
{
    // ===== Core Stats =====
    data = data_;
    magnification = magnification_;

    currentHealth = data->health * magnification.x;

    // Knockback threshold setup
    healthLeftBeforeNextKnockback =
        data->knockbackCount > 0 ? data->health - (data->health / data->knockbackCount) : 0.f;

    // Initial spawn position (left side of screen, bottom 2/3)
    position = { 0.0f, 1080.0f * 2 / 3 };

    // Start with attack cooldown ready
    currentAttackCooldown = data->attackFrequency;

    // ===== Sprite & Texture =====
    bool isTextureLoaded = texture.loadFromFile(data->texture);
    sprite.setTexture(texture, true);

    // Origin at sprite center (x) and bottom (y)
    sprite.setOrigin({
        static_cast<float>(texture.getSize().x / data->frameCount / 2),
        static_cast<float>(texture.getSize().y)
        });

    // Initialize first frame
    currentFrameIndex = 0;
    sprite.setTextureRect({
        { static_cast<int>(texture.getSize().x / data->frameCount * currentFrameIndex), 0 },
        { static_cast<int>(texture.getSize().x / data->frameCount), static_cast<int>(texture.getSize().y) }
        });

    // ===== Combat Zones =====
    hitbox.size = {
        static_cast<float>(texture.getSize().x / data->frameCount / 2),
        1080.0f
    };

    float halfFrameWidth = hitbox.size.x;

    attackRangeZone.size = { data->attackRange + hitbox.size.x, 1080.0f };

    float dmgZoneStartPos = data->damageZone.first;
    float dmgZoneWidth = data->damageZone.second;
    damageZone.size = { dmgZoneWidth, 1080.f };

    // ===== Initial Positions =====
    hitbox.position = position;
    attackRangeZone.position = position;
    damageZone.position = { position.x + halfFrameWidth + dmgZoneStartPos, position.y };

#ifdef DEBUG_MODE
    // ===== Debug Rectangles =====
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
BattleEnemy::~BattleEnemy()
{
    // Optional debug
    // std::cout << "BattleEnemy Destructor called\n";
}

// ==================
// Update
// ==================
void BattleEnemy::update(
    float deltaTime,
    const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList
)
{
    // Advance state machine
    stateMachine->update_state(deltaTime);

    // Update position & sprite
    update_position(deltaTime);
    update_sprite();

    // Progress attack and animation timers
    currentAttackCooldown += deltaTime;
    currentFrameCooldown += deltaTime;
}

// ==================
// Position Update
// ==================
void BattleEnemy::update_position(float deltaTime)
{
    float halfFrameWidth = static_cast<float>(texture.getSize().x / data->frameCount / 2);

    if (stateMachine->get_active_state_id() == "KNOCKBACK")
    {
        if (tweenX.progress() < 1.0f && tweenY.progress() < 1.0f)
        {
            sprite.setPosition({ 
                tweenX.peek() + halfFrameWidth,
                tweenY.peek() 
            });
			//std::cout << "Knockback Position: (" << tweenX.peek() + halfFrameWidth << ", " << tweenY.peek() << ")\n";
        }
    }
    else
    {
        sprite.setPosition({
            position.x + halfFrameWidth,
            position.y - static_cast<float>(currentLayer)
            });
    }

    // Sync combat zones
    hitbox.position = position;
    attackRangeZone.position = position;
    damageZone.position = { position.x + hitbox.size.x + data->damageZone.first, position.y };

#ifdef DEBUG_MODE
    // Update debug rectangles
    rHitbox.setPosition(hitbox.position);
    rAttackRangeZone.setPosition(attackRangeZone.position);
    rDamageZone.setPosition(damageZone.position);
#endif
}