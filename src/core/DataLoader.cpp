#include "DataLoader.h"

#include "../external/nlohmann/json.hpp"

#include <fstream>
#include <iostream>

using json = nlohmann::json;

bool DataLoader::load_all() 
{
    return load_units("game_data/units.json") &&
           load_enemies("game_data/enemies.json") &&
           load_stages("game_data/stages.json");
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
        std::shared_ptr<UnitData> data = std::make_shared<UnitData>();

        data->UID = value.at("UID").get<int>();

        data->name = value.at("name").get<std::string>();
        data->description = value.at("description").get<std::string>();

        data->health = value.at("health").get<float>();

        data->attackPower = value.at("attackPower").get<float>();
        data->attackRange = value.at("attackRange").get<float>();
        data->attackType = static_cast<AttackType>(value.at("attackType").get<int>());
        data->attackFrequency = value.at("attackFrequency").get<float>();

        data->movementSpeed = value.at("movementSpeed").get<float>();

        data->knockbackCount = value.at("knockbackCount").get<int>();

        data->texture = value.at("texture").get<std::string>();
        data->frameCount = value.at("frameCount").get<int>();

        m_unitsDataBase[data->UID] = data;
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
        std::shared_ptr<EnemyData> data = std::make_shared<EnemyData>();

        data->UID = value.at("UID").get<int>();

        data->name = value.at("name").get<std::string>();
        data->description = value.at("description").get<std::string>();

        data->health = value.at("health").get<float>();

        data->attackPower = value.at("attackPower").get<float>();
        data->attackRange = value.at("attackRange").get<float>();
        data->attackType = static_cast<AttackType>(value.at("attackType").get<int>());
        data->attackFrequency = value.at("attackFrequency").get<float>();

        data->movementSpeed = value.at("movementSpeed").get<float>();

        data->knockbackCount = value.at("knockbackCount").get<int>();

        data->texture = value.at("texture").get<std::string>();

        m_enemiesDataBase[data->UID] = data;
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

        m_stagesDataBase[data->UID] = data;
    }

    m_stagesLoaded = true;
    return true;
}

const std::shared_ptr<UnitData> DataLoader::get_unit_data(int uid) const
{
    auto it = m_unitsDataBase.find(uid);

    if (it != m_unitsDataBase.end()) {
        return it->second;
    }

    return std::make_shared<UnitData>();
}

const std::shared_ptr<EnemyData> DataLoader::get_enemy_data(int uid) const
{
    auto it = m_enemiesDataBase.find(uid);

    if (it != m_enemiesDataBase.end()) {
        return it->second;
    }

    return std::make_shared<EnemyData>();
}

const std::shared_ptr<StageData> DataLoader::get_stage_data(int uid) const
{
    auto it = m_stagesDataBase.find(uid);

    if (it != m_stagesDataBase.end()) {
        return it->second;
    }

    return std::make_shared<StageData>();
}
