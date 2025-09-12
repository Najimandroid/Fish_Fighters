#pragma once

#include "../data/EntityData.h"
#include "../data/PlayerData.h"
#include "../data/EnemyStageData.h"
#include "../data/StageData.h"

#include <unordered_map>
#include <memory>

/*
 * AttackType enumerates the possible attack behaviors of a unit or enemy.
 */
enum AttackType
{
    SINGLE = 1,  // Single-target attack
    AREA         // Area-of-effect attack
};

/*
 * DataLoader class
 * -----------------
 * DataLoader manages the loading, saving, and access of all game data.
 * Responsibilities include:
 *   - Loading units, enemies, stages, and player progress from JSON files
 *   - Providing easy access to EntityData, StageData, and PlayerData by UID
 *   - Saving player progress when the game terminates
 *
 * Example usage:
 *   std::shared_ptr<DataLoader> loader = std::make_shared<DataLoader>();
 *   loader->load_all();
 *   auto player = loader->get_player_data().lock();
 */

class DataLoader {
public:

    // Loads all game data (units, enemies, stages, player progress)
    bool load_all();

    // Saves necessary data (mainly player progress)
    bool terminate();

    // Accessors
    const std::shared_ptr<EntityData> get_unit_data(int uid) const;
    const std::shared_ptr<EntityData> get_enemy_data(int uid) const;
    const std::shared_ptr<StageData> get_stage_data(int uid) const;
    std::weak_ptr<PlayerData> get_player_data() const;

    // Returns file paths for unit/enemy icons
    std::string get_unit_icon_texture_path(int uid) const;
    std::string get_enemy_icon_texture_path(int uid) const;

private:
    // Internal loading methods
    bool load_units(const std::string& path);
    bool load_enemies(const std::string& path);
    bool load_stages(const std::string& path);
    bool load_player(const std::string& path);

    // Internal saving method
    bool save_player(const std::string& path);

private:

    // ----- Flags -----
    bool m_unitsLoaded = false;
    bool m_enemiesLoaded = false;
    bool m_stagesLoaded = false;
    bool m_playerLoaded = false;

    // ----- Databases -----
    std::unordered_map<int, std::shared_ptr<EntityData>> m_unitsDatabase;
    std::unordered_map<int, std::shared_ptr<EntityData>> m_enemiesDatabase;
    std::unordered_map<int, std::shared_ptr<StageData>> m_stagesDatabase;

    // ----- Player Data -----
    std::shared_ptr<PlayerData> m_playerData = nullptr;
};
