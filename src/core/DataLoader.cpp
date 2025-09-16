#include "DataLoader.h"

#include "../external/nlohmann/json.hpp"

#include <fstream>
#include <iostream>

using json = nlohmann::json;

/*
 * Loads all game data: units, enemies, stages, and player data.
 * Returns true if all data loaded successfully.
 */
bool DataLoader::load_all()
{
    return load_units("game_data/units.json") &&
        load_enemies("game_data/enemies.json") &&
        load_stages("game_data/stages.json") &&
        load_chapters("game_data/chapters.json") &&
        load_player("game_data/player.json");
}

/*
 * Save any persistent data (player progression).
 * Called when game terminates.
 */
bool DataLoader::terminate()
{
    return save_player("game_data/player.json");
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
        int UID = value.at("UID").get<int>();
        int baseUpgradeCost = value.at("baseUpgradeCost").get<int>();

        for (auto& formValue : value.at("forms"))
        {
            auto data = std::make_shared<EntityData>();

            data->UID = UID;
            data->baseUpgradeCost = baseUpgradeCost;

            data->name = formValue.at("name").get<std::string>();
            data->description = formValue.at("description").get<std::string>();

            data->cost = formValue.at("cost").get<int>();
            data->cooldown = formValue.at("cooldown").get<int>();

            data->health = formValue.at("health").get<int>();
            data->attackPower = formValue.at("attackPower").get<int>();
            data->attackRange = formValue.at("attackRange").get<float>();
            data->attackType = static_cast<AttackType>(formValue.at("attackType").get<int>());
            data->attackFrequency = formValue.at("attackFrequency").get<float>();
            data->foreswingTime = formValue.at("foreswing").get<float>();
            data->backswingTime = formValue.at("backswing").get<float>();

            data->movementSpeed = formValue.at("movementSpeed").get<float>();
            data->knockbackCount = formValue.at("knockbackCount").get<int>();

            data->texture = formValue.at("texture").get<std::string>();
            data->frameCount = formValue.at("frameCount").get<int>();
            data->knockbackFrameIndex = formValue.at("knockbackFrameIndex").get<int>();

            int formID = formValue.at("form").get<int>();
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

        data->UID = value.at("UID").get<int>();

        data->name = value.at("name").get<std::string>();
        data->description = value.at("description").get<std::string>();

        data->health = value.at("health").get<int>();

        data->attackPower = value.at("attackPower").get<int>();
        data->attackRange = value.at("attackRange").get<float>();
        data->attackType = static_cast<AttackType>(value.at("attackType").get<int>());
        data->attackFrequency = value.at("attackFrequency").get<float>();
        data->foreswingTime = value.at("foreswing").get<float>();
        data->backswingTime = value.at("backswing").get<float>();

        data->movementSpeed = value.at("movementSpeed").get<float>();
        data->knockbackCount = value.at("knockbackCount").get<int>();

        data->texture = value.at("texture").get<std::string>();
        data->frameCount = value.at("frameCount").get<int>();
        data->knockbackFrameIndex = value.at("knockbackFrameIndex").get<int>();

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

        data->UID = value.at("UID").get<int>();

        data->name = value.at("stageName").get<std::string>();

        data->enemiesLimit = value.at("enemiesLimit").get<int>();
        data->unitsLimit = value.at("unitsLimit").get<int>();

        data->baseHealth = value.at("baseHealth").get<float>();

        data->baseTexture = value.at("baseTexture").get<std::string>();
        data->backgroundTexture = value.at("backgroundTexture").get<std::string>();
        
        // Just to avoid resizing the vector too many times
        size_t numberOfDifferentEnemies = value.at("numberOfDifferentEnemies").get<size_t>();
        data->enemies.reserve(numberOfDifferentEnemies);

        for (const auto& enemyValue : value.at("enemies"))
        {
            std::shared_ptr<EnemyStageData> enemyData = std::make_shared<EnemyStageData>();

            enemyData->UID = enemyValue.at("UID").get<int>();

            enemyData->amount = enemyValue.at("amount").get<int>();
            enemyData->respawnTime = enemyValue.at("respawnTime").get<float>();
            enemyData->spawnStart = enemyValue.at("spawnStart").get<float>();

            enemyData->layer = enemyValue.at("layer").get<int>();

            enemyData->baseHealthThreshold = enemyValue.at("baseHealth").get<float>();

            enemyData->magnification.x = enemyValue["magnification"][0];
            enemyData->magnification.y = enemyValue["magnification"][1];

            enemyData->isBoss = (bool)enemyValue.at("isBoss").get<int>();
            enemyData->bypassEnemyLimit = (bool)enemyValue.at("bypassEnemyLimit").get<int>();

            enemyData->currentTimer = enemyData->respawnTime;

            data->enemies.push_back(enemyData);
        }

        for (const auto& rewardValue : value.at("rewards"))
        {
            std::shared_ptr<RewardData> reward = std::make_shared<RewardData>();

            reward->type = rewardValue.at("type").get<std::string>();

            if (reward->type == "shells")
            {
                reward->amount = rewardValue.at("amount").get<int>();
            }
            else if (reward->type == "unit")
            {
                reward->unitUID = rewardValue.at("UID").get<int>();
            }

            data->rewards.push_back(reward);
        }

        m_stagesDatabase[data->UID] = data;
    }

    m_stagesLoaded = true;
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

        chapter->UID = value["UID"];

        chapter->name = value["name"];
        chapter->description = value["description"];

        chapter->mapTexture = value["mapTexture"].get<std::string>();

        for (const auto& stageJson : value["stages"])
        {
            ChapterData::StageMapData stageMapData;

            stageMapData.UID = stageJson["UID"].get<int>();
            stageMapData.position = { stageJson["x"].get<float>(), stageJson["y"].get<float>() };

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

    std::ifstream file(path);
    if (!file) { std::cerr << "Failed to load the player\n"; return false; }

    json j;
    file >> j;

	m_playerData = std::make_shared<PlayerData>();

    //Tries to load the data:
    try
    {
        // Shells
        m_playerData->shells = j.at("money").at("shells").get<int>();

        // Owned units
        m_playerData->ownedUnits.clear();
        for (auto& [uidStr, unitData] : j.at("units").at("owned").items())
        {
            int uid = std::stoi(uidStr);
            int level = unitData.at("level").get<int>();
            int form = unitData.contains("form") ? unitData.at("form").get<int>() : 0;

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
            m_playerData->equippedUnits[i] = equippedArray[i].get<int>();
        }

        // Completed stages
        m_playerData->completedStages.clear();
        for (auto& stageUid : j.at("progression").at("completedStages"))
        {
            m_playerData->completedStages.insert(stageUid.get<int>());
        }

        // Current chapter
        m_playerData->currentChapter = j.at("progression").at("currentChapter").get<int>();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing player data: " << e.what() << "\n";
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

    // Current chapter
    j["progression"]["currentChapter"] = m_playerData->currentChapter;

    std::ofstream file(path, std::ios::trunc); // Write onto the existing file
    if (!file)
    {
        std::cerr << "Failed to open player data file for saving: " << path << "\n";
        return false;
    }

    file << j.dump(4);
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