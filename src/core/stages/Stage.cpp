#include "Stage.h"

#include "../BattleEnemy.h"
#include "../BattleUnit.h"

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
	m_units.clear();
	m_enemyStageDatas.clear();

	m_enemyBase = nullptr;
	m_fishBase = nullptr;

	m_isLoaded = false;
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
	update_units(deltaTime);
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
			std::cout << "Enemy dead\n";
			it = m_enemies.erase(it); //Todo: create remove_enemy();
			m_enemiesCount--;
			continue;
		}

		bool isFishBaseReached = enemy->attackRangeZone.findIntersection(m_fishBase->hitbox).has_value();
		if (isFishBaseReached)
		{
			enemy->targets.insert(m_fishBase);
		}

		for (auto& pair : m_units)
		{
			auto unit = pair.second;
			bool isUnitInAttackRange = enemy->attackRangeZone.findIntersection(unit->hitbox).has_value();
			if (isUnitInAttackRange && unit->state != unit->KNOCKBACK)
			{
				//std::cout << "UNIT FOUND >:(\n";
				enemy->targets.insert(unit);
			}
		}

		//std::cout << "State: " << enemy->state << "\n";

		enemy->update(deltaTime);
		it++;
	}
}

void Stage::update_units(float deltaTime)
{
	if (m_units.empty()) return;

	for (auto it = m_units.begin(); it != m_units.end();)
	{
		auto unit = it->second;

		if (unit->isDead)
		{
			//remove enemy from map
			std::cout << "Unit dead\n";
			it = m_units.erase(it); //Todo: create remove_enemy();
			m_unitsCount--;
			continue;
		}

		bool isFishBaseReached = unit->attackRangeZone.findIntersection(m_enemyBase->hitbox).has_value();
		if (isFishBaseReached)
		{
			unit->targets.insert(m_enemyBase);
		}

		for (auto& pair : m_enemies)
		{
			auto enemy = pair.second;
			bool isEnemyInAttackRange = unit->attackRangeZone.findIntersection(enemy->hitbox).has_value();
			if (isEnemyInAttackRange && enemy->state != enemy->KNOCKBACK)
			{
				unit->targets.insert(enemy);
			}
		}

		unit->update(deltaTime);
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

	//Drawing enemies
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

	//Drawing units
	for (auto& [layer, unit] : std::ranges::reverse_view(m_units))
	{
		window.draw(unit->sprite);

#ifdef DEBUG_MODE
		//Rendering the hitboxes in debug mode
		window.draw(unit->rDamageZone);
		window.draw(unit->rAttackRangeZone);
		window.draw(unit->rHitbox);
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
	battleEnemy->update_position();

	m_enemiesCount++;
	m_enemies[battleEnemy->currentLayer] = battleEnemy;
}

void Stage::spawn_unit(std::shared_ptr<UnitData> unitData)
{
	if (m_unitsCount >= m_unitsLimit) return;

	std::shared_ptr<BattleUnit> battleUnit = std::make_shared<BattleUnit>(unitData);
	battleUnit->currentLayer = generate_random_spawn_layer();

	battleUnit->position.y -= battleUnit->currentLayer;
	battleUnit->update_position();

	m_unitsCount++;
	m_units[battleUnit->currentLayer] = battleUnit;

}

void Stage::remove_enemy(BattleEnemy battleEnemy)
{
	m_enemies.erase(battleEnemy.currentLayer);
}

void Stage::remove_unit(BattleUnit battleUnit)
{
}

int Stage::generate_random_spawn_layer()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(0, 50);

	return distrib(gen);
}
