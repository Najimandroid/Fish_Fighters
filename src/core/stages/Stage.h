#pragma once

#include "../../data/EnemyStageData.h"
#include "../DataLoader.h"
#include "../BattleBase.h"

#include <SFML/Graphics.hpp>

#include <vector>
#include <map>
#include <ranges>
#include <memory>
#include <string>
#include <random>

/*
* The Stage class is used to handle everything regarding the currently loaded level. It handles enemies, units, bases, etc...
* The Stage class will load any level found in game_data/stages.json. Only one level can be loaded at a time.
*/

class DataLoader;
class BattleEnemy;
class BattleUnit;
class EnemyData;

class Stage
{
public:

	Stage();

	void init(std::shared_ptr<DataLoader> dataLoader);

	void load(int uid);
	void unload();
	void update(float deltaTime);
	void render(sf::RenderWindow& window);

	void spawn_enemy(std::shared_ptr<EnemyData> enemyData, sf::Vector2f magnification, int layer, bool isBoss,bool bypassLimit);
	void spawn_unit(std::shared_ptr<UnitData> unitData); //Todo: add some kind of UserDatas to keep track of the level of each unit
	void remove_enemy(BattleEnemy battleEnemy);
	void remove_unit(BattleUnit battleUnit);

	int generate_random_spawn_layer();
	void generate_boss_shockwave();

	void update_enemies(float deltaTime);
	void update_units(float deltaTime);
	void update_bases(float deltaTime);

private:

	//Stage Data
	int m_uid = -1;

	std::string m_stageName = "Unknown Area ???";

	int m_enemiesLimit = 5;
	int m_unitsLimit = 5;

	int m_enemiesCount = 0;
	int m_unitsCount = 0;

	std::shared_ptr<DataLoader> m_dataLoader;
	std::vector<std::shared_ptr<EnemyStageData>> m_enemyStageDatas;

	//Entities
	using BattleEnemiesMap_t  = std::map<int, std::vector<std::shared_ptr<BattleEntity>>>;
	using BattleUnitsMap_t  = std::map<int, std::vector<std::shared_ptr<BattleEntity>>>;
	BattleEnemiesMap_t m_enemies; //enemies are stocked in layer order (helps with rendering)
	BattleUnitsMap_t m_units;


	//Bases
	std::shared_ptr<BattleBase> m_enemyBase;
	std::shared_ptr<BattleBase> m_fishBase;

	//Rendering
	sf::Texture m_backgroundTexture;
	sf::Sprite m_backgroundSprite;

	//Other
	float m_elapsedTime = 0.f;
	bool m_isLoaded = false;

private:

	void spawn_bases(float health, std::string texture);
};