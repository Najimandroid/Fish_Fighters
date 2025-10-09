#pragma once

#include "../../data/EnemyStageData.h"
#include "../DataLoader.h"
#include "../entities/BattleBase.h"

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

class DataLoader;
class BattleEnemy;
class BattleUnit;
class EnemyData;
class UIManager;

class Stage : public std::enable_shared_from_this<Stage>
{
public:

    // Constructor
    Stage();

    // Initialize stage with references to DataLoader and UIManager
    void init(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<UIManager> uiManager, sf::View* stageCamera);

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

    // Spawn a new player unit in the stage
    void spawn_unit(std::shared_ptr<EntityData> unitData);

    // Remove an enemy from the stage
    void remove_enemy(BattleEnemy battleEnemy);

    // Remove a unit from the stage
    void remove_unit(BattleUnit battleUnit);

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

    // Get the loaded stage's id
	int get_current_uid() const;

    // Check if a stage is currently loaded
    bool is_loaded() const;

private:

    // ----- Stage Data -----
    int m_uid = -1;
    std::string m_stageName = "Unknown Area ???";

    int m_currentCash = 0;                   // Current cash available to the player
    int m_maxCash = 500;                     // Maximum cash allowed
	int m_cashPerSecond = 50;                // Cash gained per second (with no upgrades)

    int m_enemiesLimit = 5;                  // Maximum number of enemies
    int m_unitsLimit = 5;                    // Maximum number of units

    int m_enemiesCount = 0;                  // Current number of enemies
    int m_unitsCount = 0;                    // Current number of units

    float m_length = 0.f;                    // Distance between the enemy and player bases

    std::shared_ptr<DataLoader> m_dataLoader;
    std::weak_ptr<UIManager> m_uiManager;

    std::vector<std::shared_ptr<EnemyStageData>> m_enemyStageDatas; // Enemy spawn datas

    // ----- Entities -----
    BattleEntitiesMap_t m_enemies;           // Enemy entities, stored by layer for rendering
    BattleEntitiesMap_t m_units;             // Unit entities, stored by layer

    // ----- Bases -----
    std::shared_ptr<BattleBase> m_enemyBase; // Enemy base
    std::shared_ptr<BattleBase> m_unitBase;  // Player base

    // ----- Rendering -----
    sf::Texture m_backgroundTexture;         // Stage background texture
    std::vector<sf::Sprite> m_backgroundSprites; // Background sprites

    // ----- Camera -----
    sf::View* m_stageCamera;                 // Stage camera
    float m_currentZoom = 1.f;               // Camera zoom

    // ----- State -----
    bool m_isLoaded = false;                 // Is a stage currently loaded
    float m_elapsedTime = 0.f;               // Time elapsed since stage load
    bool m_isOnEndScreen = false;            // Is the victory/defeat screen visible

private:

    // Spawn bases at the start of the stage
    void spawn_bases(float health, const std::string& texture);

    void init_background(const std::string& texturePath);
};
