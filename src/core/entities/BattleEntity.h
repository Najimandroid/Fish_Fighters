#pragma once

#include "tweeny.h"
#include "machine/StateMachine.h"
#include "../../data/EntityData.h"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <set>
#include <memory>
#include <map>

class Stage;

/*
 * BattleEntity
 * ------------
 * Abstract base class for any entity that can appear on the battlefield:
 *  - Player units
 *  - Enemy units
 *
 * Each BattleEntity:
 *  - Owns stats from EntityData (hp, attack, cooldown, etc.)
 *  - Tracks its state using a StateMachine (idle, walk, attack, knockback...)
 *  - Handles animations and knockback tweening
 *  - Interacts with Stage (spawning, targeting, combat resolution)
 */

class BattleEntity : public std::enable_shared_from_this<BattleEntity>
{
public:
    BattleEntity();
    virtual ~BattleEntity();

    // ====== Core lifecycle ======

    // Called each frame by Stage
    // Handles movement, combat logic, targeting...
    virtual void update(float deltaTime, const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList) = 0;

    // Updates the position of hitboxes/sprites based on entity movement
    virtual void update_position(float deltaTime) = 0;

    // Handles sprite frame updates (animation)
    virtual void update_sprite();

    // ====== Initialization ======

    // Attaches StateMachine to this entity
    void init_state_machine();              

    // Registers which stage this entity belongs to
    void set_current_stage(std::shared_ptr<Stage> stage); 

public:
    // ====== Core Data ======
    std::shared_ptr<EntityData> data = nullptr;    // Entity template stats (cost, hp, attack, etc.)
    std::shared_ptr<StateMachine> stateMachine;    // Controls entity behavior
    std::weak_ptr<Stage> currentStage;             // Current stage reference (for spawning, targeting)

    // ====== Stats ======
    float currentHealth = 1.0f;                    // Remaining HP
    float currentAttackCooldown = 0.0f;            // Attack cooldown (seconds)
    bool isDead = false;                           // Marked for removal

    // Knockback logic
    tweeny::tween<float> tweenX;
    tweeny::tween<float> tweenY;
    bool isOnShockwave = false;                    // Active when a boss spawns
    float healthLeftBeforeNextKnockback = 0.0f;    // Threshold to trigger knockback

    // Battlefield layer (front/middle/back)
    int currentLayer = 0;

    // Multipliers (from stage modifiers or EnemyStageData)
    sf::Vector2f magnification = { 1.f, 1.f };     // x = hp multiplier, y = attack multiplier

    // ====== Position & Movement ======
    sf::Vector2f position;                         // World position
    sf::Vector2f velocity;                         // Current velocity

    // ====== Collision & Ranges ======
    sf::FloatRect hitbox;                          // Entity's body for collisions
    sf::FloatRect attackRangeZone;                 // Range in which it can start attacking enemies
    sf::FloatRect damageZone;                      // Area where its attack actually deals damage

    // ====== Target Tracking ======
    struct WeakPtrPosXLess {
        // Ordering functor for target set (sorts entities by X position)
        bool operator()(const std::weak_ptr<BattleEntity>& a, const std::weak_ptr<BattleEntity>& b) const;
    };
    std::set<std::weak_ptr<BattleEntity>, WeakPtrPosXLess> targets;

    // ====== Rendering ======
    sf::Texture texture;
    sf::Sprite sprite;

    // Animation
    float timeUntilNextFrame = 0.1f;               // Frame duration
    float currentFrameCooldown = 0.0f;             // Timer for switching frames
    int currentFrameIndex = 0;                     // Current animation frame index

#ifdef DEBUG_MODE
    // ====== Debug Rendering ======
    sf::RectangleShape rHitbox;
    sf::RectangleShape rAttackRangeZone;
    sf::RectangleShape rDamageZone;
#endif
};
