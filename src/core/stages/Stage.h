#pragma once

#include "../../data/EnemyStageData.h"
#include "../../data/BattlefieldLayout.h"
#include "../DataLoader.h"
#include "../entities/BattleBase.h"
#include "../combat/DamageCalculator.h"
#include "../combat/StatusManager.h"
#include "../combat/CombatLog.h"

#include <SFML/Graphics.hpp>

#include <vector>
#include <map>
#include <ranges>
#include <memory>
#include <string>
#include <random>

/*
 * Stage class
 * -----------
 * Manages the currently loaded game level.
 * Responsibilities include:
 *   - Loading and unloading a stage
 *   - Managing enemies, player units, and bases
 *   - Handling enemy spawn logic and timers
 *   - Updating game entities each frame
 *   - Rendering the stage background and entities
 * Only one stage can be active at a time.
 */

using BattleEntitiesMap_t = std::map<int, std::vector<std::shared_ptr<BattleEntity>>>;

// "+50$" rising where an enemy died and dropped cash (drawn by UICashPopups)
struct CashPopup
{
    sf::Vector2f position;   // World position (above the enemy)
    int amount = 0;
    float age = 0.f;         // Seconds since it appeared
};

class DataLoader;
class BattleEnemy;
class BattleUnit;
class EnemyData;
class UIManager;
class AudioManager;

class Stage : public std::enable_shared_from_this<Stage>
{
public:

    // Constructor
    Stage();

    // Initialize stage with references to DataLoader, UIManager and AudioManager
    void init(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<UIManager> uiManager, std::shared_ptr<AudioManager> audio, sf::View* stageCamera);

    // Load a stage by its unique ID
    void load(int uid);

    // Unload the current stage and clear all entities
    void unload();

    // Update stage state each frame
    void update(float deltaTime);

    // Render the stage (background, bases, units, enemies)
    void render(sf::RenderWindow& window);

    // Spawn a new enemy in the stage
    void spawn_enemy(std::shared_ptr<EntityData> enemyData, sf::Vector2f magnification, int layer, bool isBoss, bool bypassLimit);

    // Spawn a new player unit in the stage; false if it can't (units limit, cash, end screen)
    // ignoreCostAndLimit: debug tools only, spawns for free even when the units limit is reached
    bool spawn_unit(std::shared_ptr<EntityData> unitData, bool ignoreCostAndLimit = false);

    // ===== Sounds =====
    // Plays a sound of sfx.json (nothing without audio)
    void play_sound(const std::string& name, float volumeScale = 1.f) const;

    // An attack hit at least one target: the attacker's attack sound, and the clang of a base if one was hit
    void on_attack_landed(const BattleEntity& attacker, bool hasHitBase) const;

    // Generate a random layer for entity spawning
    int generate_random_spawn_layer();

    // Apply boss shockwave effect to all player units
    void generate_boss_shockwave();

    // Update all enemies
    void update_enemies(float deltaTime);

    // Update all player units
    void update_units(float deltaTime);

    // Update both player and enemy bases
    void update_bases(float deltaTime);

    // Increment cash available to the player
    void update_cash(float deltaTime);

    // Upgrade available cash; returns false if insufficient funds
    bool upgrade_cash(int level, int cost);

    // Apply zoom to the stage view
    void apply_zoom(float zoom);

    // Focus on the player base (when entering a stage)
    void focus_on_player_base();

    // Clamp the camera
    void clamp_camera();

    // Check if the player base has been destroyed
    bool is_unit_base_destroyed();

    // Check if the enemy base has been destroyed
    bool is_enemy_base_destroyed();

    // Get references to enemy and unit entities
    BattleEntitiesMap_t& get_enemies();
    BattleEntitiesMap_t& get_units();

    // Get weak pointers to bases
    std::weak_ptr<BattleBase> get_enemy_base() const;
    std::weak_ptr<BattleBase> get_unit_base() const;

    // Get current and maximum cash
    int get_cash() const;
    int get_max_cash() const;

    // The cash dropped by the enemies that just died ("+50$" texts)
    const std::vector<CashPopup>& get_cash_popups() const;
    static constexpr float CASH_POPUP_SECONDS = 1.2f;

    // True when as many units as the stage allows are on the field (deploying shows a message)
    bool is_unit_limit_reached() const;

    // Get the loaded stage's id
	int get_current_uid() const;

    // Get the calculator that decides the damage of every hit
    const DamageCalculator& get_damage_calculator() const;

    // Get the log of the last hits (debug tools)
    CombatLog& get_combat_log();

    // Debug tools only: sets the current cash (clamped to the maximum)
    void set_cash(int cash);

    // Get the manager that applies, stacks and removes statuses
    StatusManager& get_status_manager();

    // Seconds since the stage was loaded
    float get_elapsed_time() const;

    // Check if a stage is currently loaded
    bool is_loaded() const;

private:

    // ----- Stage Data -----
    int m_uid = -1;
    std::string m_stageName = "Unknown Area ???";

    static constexpr int BASE_MAX_CASH = 500;        // Wallet size at the start of every stage
    static constexpr int BASE_CASH_PER_SECOND = 50;  // Income at the start of every stage

    int m_currentCash = 0;                   // Current cash available to the player
    int m_maxCash = BASE_MAX_CASH;           // Maximum cash allowed
	int m_cashPerSecond = BASE_CASH_PER_SECOND; // Cash gained per second (with no upgrades)
    float m_cashRemainder = 0.f;             // Fraction of cash earned but not added yet (frame-rate independent income)
    std::vector<CashPopup> m_cashPopups;     // Cash dropped by enemies, shown for CASH_POPUP_SECONDS

    int m_enemiesLimit = 5;                  // Maximum number of enemies
    int m_unitsLimit = 5;                    // Maximum number of units

    int m_enemiesCount = 0;                  // Current number of enemies
    int m_unitsCount = 0;                    // Current number of units
    int m_nextEntityId = 0;                  // Next BattleEntity::uniqueId to give (reset on unload)

    float m_length = 0.f;                    // Distance between the enemy and player bases

    std::shared_ptr<DataLoader> m_dataLoader;
    std::weak_ptr<UIManager> m_uiManager;
    std::shared_ptr<AudioManager> m_audio;   // Music and sounds of the battle

    // ----- Music -----
    std::string m_musicChange;               // Theme to switch to once the enemy base is low enough ("" = none)
    float m_musicChangeBaseHealth = 50.f;    // ...at or below this % of its health
    bool m_hasMusicChanged = false;

    std::vector<std::shared_ptr<EnemyStageData>> m_enemyStageDatas; // Enemy spawn datas

    // ----- Entities -----
    BattleEntitiesMap_t m_enemies;           // Enemy entities, stored by layer for rendering
    BattleEntitiesMap_t m_units;             // Unit entities, stored by layer

    // ----- Combat -----
    DamageCalculator m_damageCalculator;     // Decides the damage of every hit (units, enemies, bases)
    StatusManager m_statusManager;           // Applies, stacks and removes statuses on units and enemies
    CombatLog m_combatLog;                   // Last hits of the stage, for the debug tools

    // ----- Bases -----
    std::shared_ptr<BattleBase> m_enemyBase; // Enemy base
    std::shared_ptr<BattleBase> m_unitBase;  // Player base

    // ----- Rendering -----
    sf::Texture m_backgroundTexture;         // Stage background texture
    std::vector<sf::Sprite> m_backgroundSprites; // Background sprites
    sf::Texture m_shadowTexture;             // Soft round shadow drawn under every unit and enemy

    // ----- Camera -----
    sf::View* m_stageCamera;                 // Stage camera
    float m_currentZoom = 1.f;               // Camera zoom

    // ----- State -----
    bool m_isLoaded = false;                 // Is a stage currently loaded
    float m_elapsedTime = 0.f;               // Time elapsed since stage load
    bool m_isOnEndScreen = false;            // Is the victory/defeat screen visible

private:

    // Spawn bases at the start of the stage
    void spawn_bases(float health, float playerBaseHealth, const std::string& texture);

    // Switches to the stage's second theme once the enemy base's health is low enough (once per battle)
    void update_music();

    // A dead enemy's cash drop (EntityData::cashDrop) goes to the wallet, up to its size, with a "+X$" popup
    void reward_kill(const BattleEntity& enemy);

    // The background image, made seamless when backgrounds.json asks for it (the file isn't changed)
    void load_background_texture(const BackgroundData& background);

    // Background copies along the stage (any image size), ground row from backgrounds.json (-1 = template ratio)
    void init_background(float groundRow);

    // Soft round shadow on the ground (like The Battle Cats), shape from BattlefieldLayout
    void draw_shadow(sf::RenderWindow& window, const ShadowShape& shape) const;

    // The shadow of a unit or an enemy: under its body, at its feet, following the sprite along x (knockback)
    void draw_entity_shadow(sf::RenderWindow& window, const BattleEntity& entity, bool isUnit) const;

    // The shadow of a base: under its whole image
    void draw_base_shadow(sf::RenderWindow& window, const BattleBase& base) const;
};
