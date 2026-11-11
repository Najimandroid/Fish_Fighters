#pragma once

#include "../data/EntityData.h"
#include "../data/PlayerData.h"
#include "../data/EnemyStageData.h"
#include "../data/StageData.h"
#include "../data/BackgroundData.h"
#include "../data/BattlefieldLayout.h"
#include "../data/ChapterData.h"
#include "../data/Settings.h"
#include "entities/status/StatusEffect.h"

#include <unordered_map>
#include <map>
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

    // Folder the JSON files are read from and saved to (default "game_data/").
    // The editor points it at a temporary copy to check its changes with this same loader.
    void set_data_folder(const std::string& folder);

    // Turns error_log.txt on or off (the editor reads get_last_error() instead)
    void set_error_log_enabled(bool isEnabled);

    // Message of the last error (empty if none since load_all() started)
    const std::string& get_last_error() const;

    // Loads all game data (units, enemies, stages, player progress)
    bool load_all();

    // Saves necessary data (mainly player progress)
    bool terminate();

    // Saves player progress right away (after victories, upgrades, equip changes)
    bool save_progress();

    // The player's options (settings.json): loading never fails (defaults for anything missing)
    void load_settings();
    bool save_settings();
    Settings& get_settings();

    // Accessors
    const std::shared_ptr<EntityData> get_unit_data(int uid, int form) const;
    const std::shared_ptr<EntityData> get_enemy_data(int uid) const;
    const std::shared_ptr<StageData> get_stage_data(int uid) const;
    const std::shared_ptr<ChapterData> get_chapter_data(int uid) const;

    // Settings of a background image (backgrounds.json): ground row, seamless overlap. Defaults if not listed
    BackgroundData get_background_data(const std::string& texture) const;
    const std::shared_ptr<StatusData> get_status_data(StatusType type) const;
    std::weak_ptr<PlayerData> get_player_data() const;

    // Every unit form (key: {UID, form}) and every enemy (key: UID), read-only (used by the debug tools)
    const std::map<std::pair<int, int>, std::shared_ptr<EntityData>>& get_units_database() const;
    const std::unordered_map<int, std::shared_ptr<EntityData>>& get_enemies_database() const;
    const std::unordered_map<int, std::shared_ptr<StageData>>& get_stages_database() const;

    // Returns file paths for unit/enemy icons
    std::string get_unit_icon_texture_path(int uid, int form) const;
    std::string get_enemy_icon_texture_path(int uid) const;

private:
    // Internal loading methods
    bool load_units(const std::string& path);
    bool load_enemies(const std::string& path);
    bool load_stages(const std::string& path);
    bool load_player(const std::string& path);
    bool load_chapters(const std::string& path);
    bool load_backgrounds(const std::string& path);   // Optional file
    bool load_statuses(const std::string& path);

    // Internal saving method
    bool save_player(const std::string& path);

    // Prints an error, keeps it for get_last_error() and writes it to error_log.txt
    void report_error(const std::string& message);

    std::string player_save_path() const;

private:

    // ----- Files -----
    std::string m_dataFolder = "game_data/";
    bool m_isWritingErrorLog = true;
    std::string m_lastError;

    // ----- Flags -----
    bool m_unitsLoaded = false;
    bool m_enemiesLoaded = false;
    bool m_stagesLoaded = false;
    bool m_chaptersLoaded = false;
    bool m_playerLoaded = false;
    bool m_statusesLoaded = false;

    // ----- Databases -----
    std::map<std::pair<int, int>, std::shared_ptr<EntityData>> m_unitsFormsDatabase;
    std::unordered_map<int, std::shared_ptr<EntityData>> m_enemiesDatabase;
    std::unordered_map<int, std::shared_ptr<StageData>> m_stagesDatabase;
    std::unordered_map<int, std::shared_ptr<ChapterData>> m_chaptersDatabase;
    std::unordered_map<StatusType, std::shared_ptr<StatusData>> m_statusesDatabase;
    std::unordered_map<std::string, std::shared_ptr<BackgroundData>> m_backgroundsDatabase;   // Key: image path

    // ----- Player Data -----
    std::shared_ptr<PlayerData> m_playerData = nullptr;

    // ----- Options -----
    Settings m_settings;
};
