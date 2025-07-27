#pragma once

#include "../../data/EnemyStageData.h"
#include "../DataLoader.h"

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
class EnemyData;

class Stage
{
public:

	Stage();

	void init(std::shared_ptr<DataLoader> dataLoader);

	void load(int uid);
	void update(float deltaTime);
	void render(sf::RenderWindow& window);

	void spawn_enemy(std::shared_ptr<EnemyData> enemyData, int layer, bool bypassLimit);
	int generate_random_spawn_layer();

	void update_enemies(float deltaTime);

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
	std::map<int, std::shared_ptr<BattleEnemy>> m_enemies; //enemies are stocked in layer order (helps with rendering)

	//Bases
	float baseHealth = 1.0f;
	float fishBaseHealth = 50.0f;
	sf::FloatRect m_baseHitbox;
	sf::FloatRect m_fishBaseHitbox;

	//Rendering
	sf::Texture m_baseTexture;
	sf::Sprite m_baseSprite;

	sf::Texture m_fishBaseTexture = sf::Texture("assets/images/textures/bases/fishBaseTEST.png");
	sf::Sprite m_fishBaseSprite = sf::Sprite(m_fishBaseTexture);

	sf::Texture m_backgroundTexture;
	sf::Sprite m_backgroundSprite;

	//Other
	float m_elapsedTime = 0.f;
	bool m_isLoaded = false;
};