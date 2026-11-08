#include "BattleBase.h"

#include <iostream>

// ==================
// Constructor
// ==================
BattleBase::BattleBase(float health_, std::string texture_)
{
    // ===== Core Stats =====
    maxHealth = health_;
    currentHealth = maxHealth;

    // Base positioned near left side (default),
    // but can be shifted externally (e.g., enemy base on the right)
    position = { 0.0f, 360.0f };

    // Layer determines rendering depth
    currentLayer = 51;

    // ===== Combat Zones =====
    // Bases only act as hit targets (no attack range/damage zone)
    hitbox.size = { 200.0f, 720.0f };
    attackRangeZone.size = { 0.0f, 720.0f };
    damageZone.size = { 0.0f, 720.0f };

    // Initial zone positions
    hitbox.position = position;
    attackRangeZone.position = position;
    damageZone.position = position;

    // ===== Sprite =====
    bool isTextureLoaded = texture.loadFromFile(texture_);
    sprite.setTexture(texture, true);
}

// ==================
// Update
// ==================
void BattleBase::update(
    float deltaTime,
    const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList
)
{
    // Victory and defeat are detected in Stage::update()

#ifdef DEBUG_MODE
    // Debugging health in console
    // std::cout << "[Base Health] = " << currentHealth << "\n";
#endif

    // Sync sprite + hitboxes
    update_position(deltaTime);
}

// ==================
// Position Update
// ==================
void BattleBase::update_position(float deltaTime)
{
    // Base is static, only shifted vertically by layer offset
    sprite.setPosition({
        position.x,
        position.y - static_cast<float>(currentLayer)
        });

    // Sync combat zones with position
    hitbox.position = position;
    attackRangeZone.position = position;
    damageZone.position = position;
}
