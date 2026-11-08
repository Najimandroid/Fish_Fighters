#include "DataLoader.h"

#include "../external/nlohmann/json.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

namespace
{
    const std::string ERROR_LOG_PATH = "error_log.txt";

    /*
     * JSON readers that refuse to lose data silently.
     * nlohmann's get<int>() truncates 1.75 to 1, so whole numbers are checked here.
     */
    int read_int_value(const json& value, const std::string& name)
    {
        if (value.is_number_integer()) return value.get<int>();

        if (value.is_number_float())
        {
            double number = value.get<double>();
            if (number == std::floor(number)) return static_cast<int>(number);
            throw std::runtime_error("\"" + name + "\" must be a whole number, got " + value.dump());
        }

        throw std::runtime_error("\"" + name + "\" must be a number, got " + value.dump());
    }

    float read_float_value(const json& value, const std::string& name)
    {
        if (!value.is_number()) throw std::runtime_error("\"" + name + "\" must be a number, got " + value.dump());
        return value.get<float>();
    }

    int read_int(const json& object, const std::string& key) { return read_int_value(object.at(key), key); }
    float read_float(const json& object, const std::string& key) { return read_float_value(object.at(key), key); }

    // Accepts true/false or 1/0
    bool read_flag(const json& object, const std::string& key)
    {
        const json& value = object.at(key);
        if (value.is_boolean()) return value.get<bool>();
        return read_int_value(value, key) != 0;
    }

    /*
     * Entity types, e.g. "types": [ -1, 0 ].
     * Optional: a missing list means no type. -1 (EntityType::NONE) entries are skipped.
     */
    std::vector<EntityType> read_types(const json& object, const std::string& key)
    {
        std::vector<EntityType> types;
        if (!object.contains(key)) return types;

        const json& list = object.at(key);
        if (!list.is_array()) throw std::runtime_error("\"" + key + "\" must be a list, got " + list.dump());

        for (const json& value : list)
        {
            int number = read_int_value(value, key);
            if (number < static_cast<int>(EntityType::NONE) || number >= static_cast<int>(EntityType::COUNT))
                throw std::runtime_error("\"" + key + "\" contains an unknown entity type: " + std::to_string(number));

            EntityType type = static_cast<EntityType>(number);
            if (type != EntityType::NONE) types.push_back(type);
        }
        return types;
    }

    // Ability types are written by name in the JSON so they stay readable
    AbilityType read_ability_type(const json& ability)
    {
        const std::string name = ability.at("type").get<std::string>();

        for (int i = static_cast<int>(AbilityType::NONE) + 1; i < static_cast<int>(AbilityType::COUNT); ++i)
        {
            if (name == to_string(static_cast<AbilityType>(i))) return static_cast<AbilityType>(i);
        }
        throw std::runtime_error("Unknown ability type \"" + name + "\"");
    }

    /*
     * Abilities, e.g. "abilities": [ { "type": "STRONG_AGAINST", "targetTypes": [ 0 ] } ].
     * Optional. value, chance and duration are only needed by on-hit abilities,
     * and are checked so a typo can't create an ability that silently does nothing.
     * Without "targetTypes", an ability targets `defaultTargets` (enemies: every fish).
     * If there's no default either (units), the ability would affect nothing, so it's an error.
     */
    std::vector<AbilityData> read_abilities(const json& object, const std::vector<EntityType>& defaultTargets)
    {
        std::vector<AbilityData> abilities;
        if (!object.contains("abilities")) return abilities;

        for (const json& abilityValue : object.at("abilities"))
        {
            AbilityData ability;
            ability.type = read_ability_type(abilityValue);
            ability.targetTypes = read_types(abilityValue, "targetTypes");

            if (ability.targetTypes.empty()) ability.targetTypes = defaultTargets;
            if (ability.targetTypes.empty())
                throw std::runtime_error(std::string(to_string(ability.type)) + " needs \"targetTypes\" (e.g. [ 0 ] for SHELLED)");

            if (abilityValue.contains("value")) ability.value = read_float(abilityValue, "value");
            if (abilityValue.contains("chance")) ability.chance = read_float(abilityValue, "chance");
            if (abilityValue.contains("duration")) ability.duration = read_float(abilityValue, "duration");

            if (is_on_hit(ability.type))
            {
                const std::string name = to_string(ability.type);
                if (ability.duration <= 0.f) throw std::runtime_error(name + " needs a \"duration\" above 0");
                if (ability.chance <= 0.f || ability.chance > 100.f) throw std::runtime_error(name + " needs a \"chance\" between 0 and 100");
                if (ability.type != AbilityType::PARALYZE_ATTACK && ability.value <= 0.f)
                    throw std::runtime_error(name + " needs a \"value\" above 0 (e.g. 0.5 for x0.5)");
            }

            abilities.push_back(ability);
        }
        return abilities;
    }
}

/*
 * Prints an error, keeps it for get_last_error(), and appends it to error_log.txt
 * next to the executable (release builds have no console) unless that log is turned off.
 */
void DataLoader::report_error(const std::string& message)
{
    std::cerr << message << "\n";
    m_lastError = message;

    if (!m_isWritingErrorLog) return;
    std::ofstream log(ERROR_LOG_PATH, std::ios::app);
    if (log) log << message << "\n";
}

void DataLoader::set_data_folder(const std::string& folder)
{
    m_dataFolder = folder;
    if (!m_dataFolder.empty() && m_dataFolder.back() != '/' && m_dataFolder.back() != '\\') m_dataFolder += '/';
}

void DataLoader::set_error_log_enabled(bool isEnabled)
{
    m_isWritingErrorLog = isEnabled;
}

const std::string& DataLoader::get_last_error() const
{
    return m_lastError;
}

std::string DataLoader::player_save_path() const
{
    return m_dataFolder + "player.json";
}

/*
 * Loads all game data: units, enemies, stages, and player data.
 * Returns true if all data loaded successfully.
 * Errors (missing files, broken JSON, wrong values) are written to error_log.txt.
 */
bool DataLoader::load_all()
{
    auto load = [this](bool (DataLoader::*loader)(const std::string&), const std::string& path)
        {
            try
            {
                if ((this->*loader)(path)) return true;
                report_error("Could not load " + path);
            }
            catch (const std::exception& e)
            {
                report_error("Could not load " + path + ": " + e.what());
            }
            return false;
        };

    m_lastError.clear();

    return load(&DataLoader::load_statuses, m_dataFolder + "statuses.json") &&
        load(&DataLoader::load_units, m_dataFolder + "units.json") &&
        load(&DataLoader::load_enemies, m_dataFolder + "enemies.json") &&
        load(&DataLoader::load_stages, m_dataFolder + "stages.json") &&
        load(&DataLoader::load_chapters, m_dataFolder + "chapters.json") &&
        load(&DataLoader::load_player, player_save_path());
}

/*
 * Save any persistent data (player progression).
 * Called when game terminates.
 */
bool DataLoader::terminate()
{
    return save_player(player_save_path());
}

/*
 * Saves player progress now.
 * Called after victories, upgrades and when leaving the equip menu,
 * so a crash doesn't lose the whole session.
 */
bool DataLoader::save_progress()
{
    return save_player(player_save_path());
}

/*
 * Load units data from JSON.
 * Populates m_unitsDatabase with EntityData.
 */
bool DataLoader::load_units(const std::string& path)
{
    if (m_unitsLoaded) return false;

    std::ifstream file(path);
    if (!file) { std::cerr << "Failed to load units\n"; return false; }

    json j; file >> j;

    for (auto& [_, value] : j.items())
    {
        int UID = read_int(value, "UID");
        int baseUpgradeCost = read_int(value, "baseUpgradeCost");

        for (auto& formValue : value.at("forms"))
        {
            auto data = std::make_shared<EntityData>();

            data->UID = UID;
            data->baseUpgradeCost = baseUpgradeCost;

            data->name = formValue.at("name").get<std::string>();
            data->description = formValue.at("description").get<std::string>();

            data->cost = read_int(formValue, "cost");
            data->cooldown = read_float(formValue, "cooldown");

            data->health = read_int(formValue, "health");
            data->attackPower = read_int(formValue, "attackPower");
            data->attackRange = read_float(formValue, "attackRange");
            data->attackType = static_cast<AttackType>(read_int(formValue, "attackType"));
            data->attackFrequency = read_float(formValue, "attackFrequency");
            data->foreswingTime = read_float(formValue, "foreswing");
            data->backswingTime = read_float(formValue, "backswing");

            if(formValue.contains("damageZone") && formValue["damageZone"].is_array() && formValue["damageZone"].size() == 2)
            {
                data->damageZone.first = read_float_value(formValue["damageZone"][0], "damageZone");
                data->damageZone.second = read_float_value(formValue["damageZone"][1], "damageZone");
			}
            else
            {
                data->damageZone.first = 0.f;
                data->damageZone.second = data->attackRange;
            }

            // Every fish has the hidden FISH type, nothing else
            if (formValue.contains("types"))
                throw std::runtime_error(data->name + ": units can't have \"types\", every fish automatically has the hidden FISH type");
            data->types = { EntityType::FISH };

            // Unit abilities must say which enemy types they target
            data->abilities = read_abilities(formValue, {});

            data->movementSpeed = read_float(formValue, "movementSpeed");
            data->knockbackCount = read_int(formValue, "knockbackCount");

            data->texture = formValue.at("texture").get<std::string>();
            data->frameCount = read_int(formValue, "frameCount");
            data->knockbackFrameIndex = read_int(formValue, "knockbackFrameIndex");

            int formID = read_int(formValue, "form");
            m_unitsFormsDatabase[{UID, formID}] = data;
        }
    }

    m_unitsLoaded = true;
    return true;
}

/*
 * Load enemies data from JSON.
 * Populates m_enemiesDatabase with EntityData.
 */
bool DataLoader::load_enemies(const std::string& path)
{
    if (m_enemiesLoaded) return false;

    std::ifstream file(path);
    if (!file) { std::cerr << "Failed to load enemies\n"; return false; }

    json j;
    file >> j;

    for (auto& [_, value] : j.items()) 
    {
        std::shared_ptr<EntityData> data = std::make_shared<EntityData>();

        data->UID = read_int(value, "UID");

        data->name = value.at("name").get<std::string>();
        data->description = value.at("description").get<std::string>();

        data->health = read_int(value, "health");

        data->attackPower = read_int(value, "attackPower");
        data->attackRange = read_float(value, "attackRange");
        data->attackType = static_cast<AttackType>(read_int(value, "attackType"));
        data->attackFrequency = read_float(value, "attackFrequency");
        data->foreswingTime = read_float(value, "foreswing");
        data->backswingTime = read_float(value, "backswing");

        if (value.contains("damageZone") && value["damageZone"].is_array() && value["damageZone"].size() == 2)
        {
            data->damageZone.first = read_float_value(value["damageZone"][0], "damageZone");
            data->damageZone.second = read_float_value(value["damageZone"][1], "damageZone");
        }
        else
        {
            data->damageZone.first = 0.f;
            data->damageZone.second = data->attackRange;
        }

        data->types = read_types(value, "types");
        if (std::find(data->types.begin(), data->types.end(), EntityType::FISH) != data->types.end())
            throw std::runtime_error(data->name + ": FISH is reserved for units");

        // Enemy abilities target every fish unless they say otherwise
        data->abilities = read_abilities(value, { EntityType::FISH });

        data->movementSpeed = read_float(value, "movementSpeed");
        data->knockbackCount = read_int(value, "knockbackCount");

        data->texture = value.at("texture").get<std::string>();
        data->frameCount = read_int(value, "frameCount");
        data->knockbackFrameIndex = read_int(value, "knockbackFrameIndex");

        m_enemiesDatabase[data->UID] = data;
    }

    m_enemiesLoaded = true;
    return true;
}

/*
 * Load stages data from JSON.
 * Populates m_stagesDatabase with StageData.
 */
bool DataLoader::load_stages(const std::string& path)
{
    if (m_stagesLoaded) return false;

    std::ifstream file(path);
    if (!file) { std::cerr << "Failed to load stages\n"; return false; }

    json j;
    file >> j;

    for (auto& [_, value] : j.items())
    {
        std::shared_ptr<StageData> data = std::make_shared<StageData>();

        data->UID = read_int(value, "UID");

        data->name = value.at("stageName").get<std::string>();

        data->enemiesLimit = read_int(value, "enemiesLimit");
        data->unitsLimit = read_int(value, "unitsLimit");

        data->baseHealth = read_float(value, "baseHealth");
        if (value.contains("playerBaseHealth")) data->playerBaseHealth = read_float(value, "playerBaseHealth");

        data->length = read_float(value, "length");

        data->baseTexture = value.at("baseTexture").get<std::string>();
        data->backgroundTexture = value.at("backgroundTexture").get<std::string>();
        
        // Just to avoid resizing the vector too many times
        size_t numberOfDifferentEnemies = value.at("numberOfDifferentEnemies").get<size_t>();
        data->enemies.reserve(numberOfDifferentEnemies);

        for (const auto& enemyValue : value.at("enemies"))
        {
            std::shared_ptr<EnemyStageData> enemyData = std::make_shared<EnemyStageData>();

            enemyData->UID = read_int(enemyValue, "UID");

            enemyData->amount = read_int(enemyValue, "amount");
            enemyData->respawnTime = read_float(enemyValue, "respawnTime");
            enemyData->spawnStart = read_float(enemyValue, "spawnStart");

            enemyData->layer = read_int(enemyValue, "layer");

            enemyData->baseHealthThreshold = read_float(enemyValue, "baseHealth");

            enemyData->magnification.x = read_float_value(enemyValue.at("magnification").at(0), "magnification");
            enemyData->magnification.y = read_float_value(enemyValue.at("magnification").at(1), "magnification");

            enemyData->isBoss = read_flag(enemyValue, "isBoss");
            enemyData->bypassEnemyLimit = read_flag(enemyValue, "bypassEnemyLimit");

            enemyData->currentTimer = enemyData->respawnTime;

            data->enemies.push_back(enemyData);
        }

        for (const auto& rewardValue : value.at("rewards"))
        {
            std::shared_ptr<RewardData> reward = std::make_shared<RewardData>();

            reward->type = rewardValue.at("type").get<std::string>();

            if (reward->type == "shells")
            {
                reward->amount = read_int(rewardValue, "amount");
            }
            else if (reward->type == "unit")
            {
                reward->unitUID = read_int(rewardValue, "UID");
            }

            data->rewards.push_back(reward);
        }

        m_stagesDatabase[data->UID] = data;
    }

    m_stagesLoaded = true;
    return true;
}

/*
 * Load status definitions from JSON.
 * Populates m_statusesDatabase with StatusData. Every StatusType must be defined.
 */
bool DataLoader::load_statuses(const std::string& path)
{
    if (m_statusesLoaded) return false;

    std::ifstream file(path);
    if (!file) return false;

    json j;
    file >> j;

    for (auto& [name, value] : j.items())
    {
        auto data = std::make_shared<StatusData>();

        for (int i = static_cast<int>(StatusType::NONE) + 1; i < static_cast<int>(StatusType::COUNT); ++i)
        {
            if (name == to_string(static_cast<StatusType>(i))) data->type = static_cast<StatusType>(i);
        }
        if (data->type == StatusType::NONE) throw std::runtime_error("Unknown status \"" + name + "\"");

        data->name = value.at("name").get<std::string>();
        data->description = value.at("description").get<std::string>();
        data->iconIndex = read_int(value, "iconIndex");

        const std::string strongest = value.at("strongest").get<std::string>();
        if (strongest == "lowest") data->strongest = StrongestRule::LOWEST;
        else if (strongest == "highest") data->strongest = StrongestRule::HIGHEST;
        else throw std::runtime_error("\"strongest\" of " + name + " must be \"lowest\" or \"highest\", got \"" + strongest + "\"");

        m_statusesDatabase[data->type] = data;
    }

    // A status without a definition would have no name, icon or stacking rule
    for (int i = static_cast<int>(StatusType::NONE) + 1; i < static_cast<int>(StatusType::COUNT); ++i)
    {
        StatusType type = static_cast<StatusType>(i);
        if (m_statusesDatabase.find(type) == m_statusesDatabase.end())
            throw std::runtime_error(std::string("Status ") + to_string(type) + " is missing");
    }

    m_statusesLoaded = true;
    return true;
}

/*
 * Load chapters data from JSON.
 * Populates m_chaptersDatabase with ChapterData.
 */
bool DataLoader::load_chapters(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;

    nlohmann::json data;
    file >> data;

    for (auto& [key, value] : data.items())
    {
        auto chapter = std::make_shared<ChapterData>();

        chapter->UID = read_int(value, "UID");

        chapter->name = value.at("name").get<std::string>();
        chapter->description = value.at("description").get<std::string>();

        chapter->mapTexture = value.at("mapTexture").get<std::string>();

        for (const auto& stageJson : value.at("stages"))
        {
            ChapterData::StageMapData stageMapData;

            stageMapData.UID = read_int(stageJson, "UID");
            stageMapData.position = { read_float(stageJson, "x"), read_float(stageJson, "y") };

            chapter->stages.push_back(stageMapData);
        }

        m_chaptersDatabase[chapter->UID] = chapter;
    }

    m_chaptersLoaded = true;
    return true;
}

/*
 * Load player data from JSON
 * Populates shells, owned units, equipped units, completed stages
 */
bool DataLoader::load_player(const std::string& path)
{
    if (m_playerLoaded) return false;

	m_playerData = std::make_shared<PlayerData>();
    m_playerData->equippedUnits.fill(-1);

    std::ifstream file(path);
    if (!file)
    {
        // First launch (or deleted save): start a new game with the first fish equipped
        report_error("No save found at " + path + ", creating a new one");

        m_playerData->ownedUnits[1] = { 1, 0 };
        m_playerData->equippedUnits[0] = 1;

        m_playerLoaded = true;
        return save_player(path);
    }

    //Tries to load the data:
    try
    {
        json j;
        file >> j;

        // Shells
        m_playerData->shells = read_int(j.at("money"), "shells");

        // Owned units
        m_playerData->ownedUnits.clear();
        for (auto& [uidStr, unitData] : j.at("units").at("owned").items())
        {
            int uid = std::stoi(uidStr);
            int level = read_int(unitData, "level");
            int form = unitData.contains("form") ? read_int(unitData, "form") : 0;

            m_playerData->ownedUnits[uid] = { level, form };
        }

        // Units waiting to be unlocked
        m_playerData->unitsWaitingToBeUnlocked.clear();
        for (auto& [uidStr, _] : j.at("units").at("waitingToBeUnlocked").items())
        {
            int uid = std::stoi(uidStr);
            m_playerData->unitsWaitingToBeUnlocked.insert(uid);
        }

        // Equipped units
        auto equippedArray = j.at("units").at("equipped");
        for (size_t i = 0; i < equippedArray.size() && i < m_playerData->equippedUnits.size(); i++)
        {
            m_playerData->equippedUnits[i] = read_int_value(equippedArray[i], "equipped");
        }

        // Completed stages
        m_playerData->completedStages.clear();
        for (auto& stageUid : j.at("progression").at("completedStages"))
        {
            m_playerData->completedStages.insert(read_int_value(stageUid, "completedStages"));
        }

        // Completed chapters (missing in older saves)
        m_playerData->completedChapters.clear();
        if (j.at("progression").contains("completedChapters"))
        {
            for (auto& chapterUid : j.at("progression").at("completedChapters"))
            {
                m_playerData->completedChapters.insert(read_int_value(chapterUid, "completedChapters"));
            }
        }

        // Current chapter
        m_playerData->currentChapter = read_int(j.at("progression"), "currentChapter");
    }
    catch (const std::exception& e)
    {
        // The save is left untouched (m_playerLoaded stays false, so nothing overwrites it)
        report_error("Error parsing player data: " + std::string(e.what()));
        return false;
    }

    m_playerLoaded = true;
    return true;
}

/*
 * Save player data and write the new data into player.json
 * Saves shells, owned units, equipped units, completed stages etc...
 */
bool DataLoader::save_player(const std::string& path)
{
    if(!m_playerLoaded) 
    {
        std::cout << "Player data not loaded, cannot save\n";
        return false;
	}

    json j;

    // Shells
    j["money"]["shells"] = m_playerData->shells;

    // Owned units
    for (auto& [uid, unitInfo] : m_playerData->ownedUnits)
    {
        j["units"]["owned"][std::to_string(uid)] = {
            {"uid", uid},
            {"level", unitInfo.level},
            {"form", unitInfo.form}
        };
    }

    //Waiting to be unlocked units
    j["units"]["waitingToBeUnlocked"] = json::object(); //making sure the key exists, even if empty
    for (int uid : m_playerData->unitsWaitingToBeUnlocked)
    {
        j["units"]["waitingToBeUnlocked"][std::to_string(uid)] = { {"uid", uid} };
    }

    // Equipped units
    j["units"]["equipped"] = json::array();
    for (int uid : m_playerData->equippedUnits)
    {
        j["units"]["equipped"].push_back(uid);
    }

    // Completed stages
    j["progression"]["completedStages"] = json::array();
    for (int stageUid : m_playerData->completedStages)
    {
        j["progression"]["completedStages"].push_back(stageUid);
    }

    // Completed chapters
    j["progression"]["completedChapters"] = json::array();
    for (int chapterUid : m_playerData->completedChapters)
    {
        j["progression"]["completedChapters"].push_back(chapterUid);
    }

    // Current chapter
    j["progression"]["currentChapter"] = m_playerData->currentChapter;

    // Write to a temporary file first, then replace the save,
    // so a crash in the middle of writing can't corrupt player.json
    const std::string tempPath = path + ".tmp";
    {
        std::ofstream file(tempPath, std::ios::trunc);
        if (!file)
        {
            report_error("Failed to open player data file for saving: " + tempPath);
            return false;
        }

        file << j.dump(4);
        file.flush();
        if (!file)
        {
            report_error("Failed to write player data to " + tempPath);
            return false;
        }
    }

    std::error_code error;
    std::filesystem::rename(tempPath, path, error);
    if (error)
    {
        report_error("Failed to replace " + path + ": " + error.message());
        std::filesystem::remove(tempPath, error);
        return false;
    }

    return true;
}

const std::shared_ptr<EntityData> DataLoader::get_unit_data(int uid, int form = 0) const
{
    auto it = m_unitsFormsDatabase.find({ uid, form });

    if (it != m_unitsFormsDatabase.end()) {
        return it->second;
    }

    return std::make_shared<EntityData>();
}

const std::shared_ptr<EntityData> DataLoader::get_enemy_data(int uid) const
{
    auto it = m_enemiesDatabase.find(uid);

    if (it != m_enemiesDatabase.end()) {
        return it->second;
    }

    return std::make_shared<EntityData>();
}

const std::shared_ptr<StageData> DataLoader::get_stage_data(int uid) const
{
    auto it = m_stagesDatabase.find(uid);

    if (it != m_stagesDatabase.end()) {
        return it->second;
    }

    return std::make_shared<StageData>();
}

const std::shared_ptr<ChapterData> DataLoader::get_chapter_data(int uid) const
{
    auto it = m_chaptersDatabase.find(uid);

    if (it != m_chaptersDatabase.end()) {
        return it->second;
    }

    return std::make_shared<ChapterData>();
}

const std::shared_ptr<StatusData> DataLoader::get_status_data(StatusType type) const
{
    auto it = m_statusesDatabase.find(type);

    if (it != m_statusesDatabase.end()) {
        return it->second;
    }

    return std::make_shared<StatusData>();
}

const std::map<std::pair<int, int>, std::shared_ptr<EntityData>>& DataLoader::get_units_database() const
{
    return m_unitsFormsDatabase;
}

const std::unordered_map<int, std::shared_ptr<EntityData>>& DataLoader::get_enemies_database() const
{
    return m_enemiesDatabase;
}

const std::unordered_map<int, std::shared_ptr<StageData>>& DataLoader::get_stages_database() const
{
    return m_stagesDatabase;
}

std::weak_ptr<PlayerData> DataLoader::get_player_data() const
{
    return m_playerData;
}

std::string DataLoader::get_unit_icon_texture_path(int uid, int form = 0) const
{
    return "assets/images/textures/icons/units/icon_" + std::to_string(uid) + '_' + std::to_string(form) + ".png";
}

std::string DataLoader::get_enemy_icon_texture_path(int uid) const
{
    return "assets/images/textures/icons/enemies/icon_" + std::to_string(uid) + ".png";
}