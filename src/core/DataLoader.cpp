#include "DataLoader.h"

#include "../external/nlohmann/json.hpp"

#include <fstream>
#include <iostream>

using json = nlohmann::json;

bool DataLoader::load_all() 
{
    return load_units("game_data/units.json") &&
           load_enemies("game_data/enemies.json") &&
           load_stages("game_data/stages.json") &&
		   load_player("game_data/player.json");
}

bool DataLoader::terminate()
{
    return save_player("game_data/player.json");
}

bool DataLoader::load_units(const std::string& path)
{
    if (m_unitsLoaded)
    {
        std::cout << "Units data already loaded\n";
        return false;
    }

    std::ifstream file(path);

    if (!file) 
    {
        std::cerr << "Failed to load units from: " << path << "\n";
        return false;
    }

    json j;
    file >> j;

    for (auto& [_, value] : j.items()) 
    {
        std::shared_ptr<EntityData> data = std::make_shared<EntityData>();

        data->UID = value.at("UID").get<int>();

        data->name = value.at("name").get<std::string>();
        data->description = value.at("description").get<std::string>();
		data->baseUpgradeCost = value.at("baseUpgradeCost").get<int>();

        data->cost = value.at("cost").get<int>();
        data->cooldown = value.at("cooldown").get<int>();

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

        m_unitsDatabase[data->UID] = data;
    }

    m_unitsLoaded = true;
    return true;
}

bool DataLoader::load_enemies(const std::string& path)
{
    if (m_enemiesLoaded)
    {
        std::cout << "Enemies data already loaded\n";
        return false;
    }

    std::ifstream file(path);

    if (!file) 
    {
        std::cerr << "Failed to load enemies from: " << path << "\n";
        return false;
    }

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

bool DataLoader::load_stages(const std::string& path)
{
    if (m_stagesLoaded)
    {
        std::cout << "Stages data already loaded\n";
        return false;
    }

    std::ifstream file(path);

    if (!file)
    {
        std::cerr << "Failed to load stages from: " << path << "\n";
        return false;
    }

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
        
        //Just to avoid resizing the vector too many times
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

        m_stagesDatabase[data->UID] = data;
    }

    m_stagesLoaded = true;
    return true;
}

bool DataLoader::load_player(const std::string& path)
{
    if (m_playerData)
    {
        std::cout << "Player data already loaded\n";
        return false;
    }

    std::ifstream file(path);
    if (!file)
    {
        std::cerr << "Failed to load player data from: " << path << "\n";
        return false;
    }

    json j;
    file >> j;

	m_playerData = std::make_shared<PlayerData>();

    //Tries to load the data:
    try
    {
        //Shells
        m_playerData->shells = j.at("money").at("shells").get<int>();

        //Owned units
        m_playerData->ownedUnits.clear();
        for (auto& [uidStr, unitData] : j.at("units").at("owned").items())
        {
            int uid = std::stoi(uidStr);
            int level = unitData.at("level").get<int>();
            m_playerData->ownedUnits[uid] = level;
        }

        //Equipped units
        auto equippedArray = j.at("units").at("equipped");
        for (size_t i = 0; i < equippedArray.size() && i < m_playerData->equippedUnits.size(); i++)
        {
            m_playerData->equippedUnits[i] = equippedArray[i].get<int>();
        }

        //Completed stages
        m_playerData->completedStages.clear();
        for (auto& stageUid : j.at("progression").at("completedStages"))
        {
            m_playerData->completedStages.insert(stageUid.get<int>());
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing player data: " << e.what() << "\n";
        return false;
    }

    m_playerLoaded = true;
    return true;
}

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
    for (auto& [uid, level] : m_playerData->ownedUnits)
    {
        j["units"]["owned"][std::to_string(uid)] = {
            {"uid", uid},
            {"level", level}
        };
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

    std::ofstream file(path, std::ios::trunc); // écrase le fichier existant
    if (!file)
    {
        std::cerr << "Failed to open player data file for saving: " << path << "\n";
        return false;
    }

    file << j.dump(4);
    return true;
}

const std::shared_ptr<EntityData> DataLoader::get_unit_data(int uid) const
{
    auto it = m_unitsDatabase.find(uid);

    if (it != m_unitsDatabase.end()) {
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

std::weak_ptr<PlayerData> DataLoader::get_player_data() const
{
    return m_playerData;
}

std::string DataLoader::get_unit_icon_texture_path(int uid) const
{
    return "assets/images/textures/icons/units/icon_" + std::to_string(uid) + ".png";
}

std::string DataLoader::get_enemy_icon_texture_path(int uid) const
{
    return "assets/images/textures/icons/enemies/icon_" + std::to_string(uid) + ".png";
}