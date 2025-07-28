#include "Stage.h"

#include "../BattleEnemy.h"

#include <iostream>

void Stage::init(std::shared_ptr<DataLoader> dataLoader)
{
	m_dataLoader = dataLoader;
	unload();
}

Stage::Stage():
	m_enemyBase(nullptr), m_fishBase(nullptr),

	//m_baseTexture(sf::Texture()), m_baseSprite(sf::Sprite(m_baseTexture)), 
	m_backgroundTexture(sf::Texture()), m_backgroundSprite(sf::Sprite(m_backgroundTexture))
{}

void Stage::load(int uid)
{
	if (m_uid != -1) unload();

	//Loading stage with the StageData uid

	std::shared_ptr<const StageData> stageJson = m_dataLoader->get_stage_data(uid);

	//setup stage data
	m_uid = uid;
	m_stageName = stageJson->name;

	m_enemiesLimit = stageJson->enemiesLimit;
	m_unitsLimit = stageJson->unitsLimit;

	m_enemiesCount = 0;
	m_unitsCount = 0;

	spawn_bases(stageJson->baseHealth, stageJson->baseTexture);

	m_enemyStageDatas = stageJson->enemies;

	/*
	//Setup base sprite
	if (!m_baseTexture.loadFromFile(stageJson->baseTexture)) { bool loadDefault = m_baseTexture.loadFromFile("assets/images/textures/bases/defaultBase.png"); }
	m_baseSprite.setTexture(m_baseTexture, true);
	m_baseSprite.setPosition({ 0.0f, 360.0f - m_baseTexture.getSize().y / 2}); //the base is placed in the center of the screen
	m_baseHitbox.position = m_baseSprite.getPosition();

	m_fishBaseSprite.setPosition({ 1280.0f - m_fishBaseTexture.getSize().x, 360.0f - m_fishBaseTexture.getSize().y / 2});
	m_fishBaseHitbox.position = m_fishBaseSprite.getPosition();
	*/

	//Setup background
	bool bg = m_backgroundTexture.loadFromFile(stageJson->backgroundTexture); //to do: adding default bground
	m_backgroundSprite.setTexture(m_backgroundTexture, true);
	m_backgroundSprite.setPosition({ 0.0f, 0.0f });

	m_isLoaded = true;
}

void Stage::unload()
{
	m_uid = -1;

	m_enemies.clear();
	m_enemyStageDatas.clear();

	m_enemyBase = nullptr;
	m_fishBase = nullptr;
}

void Stage::update(float deltaTime)
{
	if (!m_isLoaded) return;

	m_elapsedTime += deltaTime;

	for (auto& spawnEnemyData : m_enemyStageDatas)
	{
		//Checks if elapsed time >= spawnStart
		if (!spawnEnemyData->hasStarted && m_elapsedTime >= spawnEnemyData->spawnStart) { spawnEnemyData->hasStarted = true; }

		//Checks if started
		if (!spawnEnemyData->hasStarted) continue;

		//Checks if the spawnEnemyData already spawned all enemies it was supposed to make spawn (-1 == infinite spawn)
		if (spawnEnemyData->amount != -1 && spawnEnemyData->spawnedCount >= spawnEnemyData->amount ) continue;

		//Updates the enemy internal clock
		spawnEnemyData->currentTimer += deltaTime;

		if (spawnEnemyData->currentTimer >= spawnEnemyData->respawnTime)
		{
			//Spawn enemy
			auto data = m_dataLoader->get_enemy_data(spawnEnemyData->UID);

			//std::cout << "Spawned enemy: " << data->name << "\n";*
			spawn_enemy(data, spawnEnemyData->magnification, spawnEnemyData->layer, spawnEnemyData->isBoss);

			spawnEnemyData->currentTimer = 0.f;
			spawnEnemyData->spawnedCount++;
		}
	}

	update_bases(deltaTime);
	update_enemies(deltaTime);
}

void Stage::update_enemies(float deltaTime)
{
	if (m_enemies.empty()) return;

	for (auto it = m_enemies.begin(); it != m_enemies.end();)
	{
		auto enemy = it->second;

		if (enemy->isDead)
		{
			//remove enemy from map
			m_enemies.erase(it);
			continue;
		}

		bool isFishBaseReached = enemy->attackRangeZone.findIntersection(m_fishBase->hitbox).has_value();
		if (isFishBaseReached)
		{
			enemy->targets.insert(m_fishBase);
			enemy->state = enemy->IDLE;
		}
		else
		{
			enemy->state = enemy->WALK;
		}

		enemy->update(deltaTime);
		it++;
	}
}

void Stage::update_bases(float deltaTime)
{
	m_enemyBase->update(deltaTime);
	m_fishBase->update(deltaTime);
}

void Stage::render(sf::RenderWindow& window)
{
	window.draw(m_backgroundSprite);
	window.draw(m_enemyBase->sprite);
	window.draw(m_fishBase->sprite);

#ifdef DEBUG_MODE
	//Rendering the hitboxes in debug mode
	window.draw(m_enemyBase->rHitbox);
	window.draw(m_fishBase->rHitbox);
#endif

	for (auto& [layer, enemy] : std::ranges::reverse_view(m_enemies))
	{

		window.draw(enemy->sprite);

#ifdef DEBUG_MODE
		//Rendering the hitboxes in debug mode
		window.draw(enemy->rDamageZone);
		window.draw(enemy->rAttackRangeZone);
		window.draw(enemy->rHitbox);
#endif
	}
}

void Stage::spawn_bases(float health, std::string texture)
{
	m_enemyBase = std::make_unique<BattleBase>(health, texture);
	m_enemyBase->position = { 0.0f, 360.0f - m_enemyBase->texture.getSize().y / 2 };
	m_enemyBase->position.y -= m_enemyBase->currentLayer;

	m_fishBase = std::make_unique<BattleBase>(350.0f, "assets/images/textures/bases/fishBaseTEST.png");
	m_fishBase->position = { 1280.0f - m_fishBase->texture.getSize().x, 360.0f - m_fishBase->texture.getSize().y / 2 };
	m_fishBase->position.y -= m_fishBase->currentLayer;
}

void Stage::spawn_enemy(std::shared_ptr<EnemyData> enemyData, sf::Vector2f magnification, int layer = -1, bool bypassLimit = false)
{
	if (m_enemiesCount >= m_enemiesLimit && bypassLimit == false) return;

	std::shared_ptr<BattleEnemy> battleEnemy = std::make_shared<BattleEnemy>(enemyData, magnification);

	//spawn in corresponding layer
	if (layer > 0)
	{
		battleEnemy->currentLayer = layer;
	}
	else
	{
		battleEnemy->currentLayer = generate_random_spawn_layer();
	}

	battleEnemy->position.y -= battleEnemy->currentLayer;

	m_enemiesCount++;
	m_enemies[battleEnemy->currentLayer] = battleEnemy;
}

int Stage::generate_random_spawn_layer()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(0, 50);

	return distrib(gen);
}
