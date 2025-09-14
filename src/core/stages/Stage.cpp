#include "Stage.h"

#include "../entities/BattleEnemy.h"
#include "../entities/BattleUnit.h"

#include "../entities/machine/states/KnockbackState.h"
#include "../entities/machine/states/IdleState.h"

#include "../../uis/UIManager.h"

#include <iostream>

/*
* Initializes the stage with the DataLoader and UIManager
*/
void Stage::init(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<UIManager> uiManager)
{
	m_dataLoader = dataLoader;
	m_uiManager = uiManager;
	unload();
}

/*
* Constructor: initialize pointers and textures
*/
Stage::Stage():
	m_enemyBase(nullptr), m_unitBase(nullptr),

	//m_baseTexture(sf::Texture()), m_baseSprite(sf::Sprite(m_baseTexture)), 
	m_backgroundTexture(sf::Texture()), m_backgroundSprite(sf::Sprite(m_backgroundTexture))
{}

/*
* Load a stage by its UIDs
*/
void Stage::load(int uid)
{
	if (m_uid != -1) unload(); // Unload previous stage

	std::shared_ptr<const StageData> stageJson = m_dataLoader->get_stage_data(uid);

	// Setup stage properties
	m_uid = uid;
	m_stageName = stageJson->name;
	m_enemiesLimit = stageJson->enemiesLimit;
	m_unitsLimit = stageJson->unitsLimit;
	m_enemiesCount = 0;
	m_unitsCount = 0;
	m_currentCash = 0;

	// Spawn bases with provided health and texture
	spawn_bases(stageJson->baseHealth, stageJson->baseTexture);

	// Copy enemy spawn data from stage JSON
	m_enemyStageDatas.clear();
	for (auto& enemy : stageJson->enemies)
	{
		auto enemyCopy = std::make_shared<EnemyStageData>(*enemy);
		enemyCopy->currentTimer = enemyCopy->respawnTime; // initialize spawn timer
		m_enemyStageDatas.push_back(enemyCopy);
	}

	// Load background texture and scale to 1920x1080
	bool bg = m_backgroundTexture.loadFromFile(stageJson->backgroundTexture);
	m_backgroundSprite.setTexture(m_backgroundTexture, true);
	m_backgroundSprite.setPosition({ 0.0f, 0.0f });

	sf::Vector2u texSize = m_backgroundTexture.getSize();
	if (texSize.x > 0 && texSize.y > 0)
	{
		m_backgroundSprite.setScale({
			1920.f / texSize.x,
			1080.f / texSize.y
			});
	}

	m_isLoaded = true;
}

/*
* Unload all stage data and entities
*/
void Stage::unload()
{
	m_uid = -1;

	m_currentCash = 0;

	m_enemies.clear();
	m_units.clear();
	m_enemyStageDatas.clear();

	m_enemyBase = nullptr;
	m_unitBase = nullptr;

	m_isLoaded = false;
}

/*
* Main update function: spawn enemies, update entities, check win/loss
*/
void Stage::update(float deltaTime)
{
	if (!m_isLoaded) return;

	m_elapsedTime += deltaTime;

	//std::cout << "[Base Health]: " << m_enemyBase->currentHealth << " / " << m_enemyBase->maxHealth << "\n";

	for (auto& spawnEnemyData : m_enemyStageDatas)
	{

		// Checks if the base health correspond to the baseHealthThreshold data
		if (m_enemyBase->maxHealth * (spawnEnemyData->baseHealthThreshold/100.0f) < m_enemyBase->currentHealth) continue;

		// Checks if elapsed time >= spawnStart
		if (m_elapsedTime >= spawnEnemyData->spawnStart) { spawnEnemyData->hasStarted = true; }

		// Checks if started
		if (!spawnEnemyData->hasStarted) continue;

		// Checks if the spawnEnemyData already spawned all enemies it was supposed to make spawn (-1 == infinite spawn)
		if (spawnEnemyData->amount != -1 && spawnEnemyData->spawnedCount >= spawnEnemyData->amount ) continue;

		// Updates the enemy internal clock
		spawnEnemyData->currentTimer += deltaTime;

		if (spawnEnemyData->currentTimer >= spawnEnemyData->respawnTime)
		{
			// Spawn enemy
			auto data = m_dataLoader->get_enemy_data(spawnEnemyData->UID);

			//std::cout << "Spawned enemy: " << data->name << "\n";*
			spawn_enemy(data, spawnEnemyData->magnification, spawnEnemyData->layer, spawnEnemyData->isBoss, spawnEnemyData->isBoss);

			spawnEnemyData->currentTimer = 0.f;
			spawnEnemyData->spawnedCount++;
		}
	}

	update_cash();

	update_bases(deltaTime);
	update_enemies(deltaTime);
	update_units(deltaTime);

	// Checks if the Player has won or lost
	if (is_enemy_base_destroyed())
	{
		// Player has won
		// TODO: add a winning screen
		if (auto player = m_dataLoader->get_player_data().lock())
		{
			auto stageData = m_dataLoader->get_stage_data(m_uid);
			if (stageData)
				player->complete_stage(stageData);
		}

		unload(); //unload stage
		m_uiManager.lock()->generate_fish_tank_uis();
		return;
	}

	if (is_unit_base_destroyed())
	{
		// Player has lost
		// TODO: add a losing screen
		unload();
		m_uiManager.lock()->generate_fish_tank_uis();
		return;
	}
}

/*
* Increment cash over time
*/
void Stage::update_cash()
{
	if (m_currentCash < m_maxCash)
		m_currentCash += 1; // TEMPORARY: increase 1 cash per update

	//std::cout << "[BANK]: " << m_currentCash << "$ / " << m_MAX_CASH << "$\n";
}

/*
* Update all enemies: remove dead, check base reach, calls update() individually
*/
void Stage::update_enemies(float deltaTime)
{
	if (m_enemies.empty()) return;

	for (auto it = m_enemies.begin(); it != m_enemies.end();)
	{
		auto& enemyList = it->second;

		for (auto itvec = enemyList.begin(); itvec != enemyList.end();)
		{
			auto enemy = itvec.operator*();
			if (enemy->isDead)
			{
				//remove enemy from map
				std::cout << "Enemy dead\n";
				itvec = enemyList.erase(itvec); // TODO: create remove_enemy();
				m_enemiesCount--;
				continue;
			}

			bool isFishBaseReached = enemy->attackRangeZone.findIntersection(m_unitBase->hitbox).has_value();
			if (isFishBaseReached && enemy->stateMachine->get_active_state_id() != "KNOCKBACK")
			{
				enemy->targets.insert(m_unitBase);
			}

			enemy->update(deltaTime, m_units);
			itvec++;
		}
		it++;
	}
}

/*
* Update all units: remove dead, check base reach, calls update() individually
*/
void Stage::update_units(float deltaTime)
{
	if (m_units.empty()) return;

	for (auto it = m_units.begin(); it != m_units.end();)
	{
		auto& unitList = it->second;

		for (auto itvec = unitList.begin(); itvec != unitList.end();)
		{
			auto unit = itvec.operator*();
			if (unit->isDead)
			{
				// Remove unit from map
				std::cout << "Unit dead\n";
				itvec = unitList.erase(itvec); //Todo: create remove_unit();
				m_unitsCount--;
				continue;
			}

			bool isEnemyBaseReached = unit->attackRangeZone.findIntersection(m_enemyBase->hitbox).has_value();
			if (isEnemyBaseReached && unit->stateMachine->get_active_state_id() != "KNOCKBACK")
			{
				unit->targets.insert(m_enemyBase);
			}

			unit->update(deltaTime, m_enemies);
			itvec++;
		}
		it++;
	}
}

/*
* Update both bases
*/
void Stage::update_bases(float deltaTime)
{
	m_enemyBase->update(deltaTime, m_units);
	m_unitBase->update(deltaTime, m_enemies);
}

/*
* Upgrade cash capacity if player has enough
*/
bool Stage::upgrade_cash(int level, int cost)
{
	if (m_currentCash < cost) return false;

	m_currentCash -= cost;
	m_maxCash += 150 * level;

	return true;
}

/*
* Check if player base is destroyed
*/
bool Stage::is_unit_base_destroyed()
{
	return m_unitBase->currentHealth <= 0.f;
}

/*
* Check if enemy base is destroyed
*/
bool Stage::is_enemy_base_destroyed()
{
	return m_enemyBase->currentHealth <= 0.f;
}

BattleEntitiesMap_t& Stage::get_enemies()
{
	return m_enemies;
}

BattleEntitiesMap_t& Stage::get_units()
{
	return m_units;
}

std::weak_ptr<BattleBase> Stage::get_enemy_base() const
{
	return m_enemyBase;
}

std::weak_ptr<BattleBase> Stage::get_unit_base() const
{
	return m_unitBase;
}

int Stage::get_cash() const
{
	return m_currentCash;
}

int Stage::get_max_cash() const
{
	return m_maxCash;
}

bool Stage::is_loaded() const
{
	return m_isLoaded;
}

/*
* Render all stage elements
*/
void Stage::render(sf::RenderWindow& window)
{
	if (!m_isLoaded) return;

	// Draw background and bases
	window.draw(m_backgroundSprite);
	window.draw(m_enemyBase->sprite);
	window.draw(m_unitBase->sprite);

#ifdef DEBUG_MODE
	// Draw hitboxes for debugging
	window.draw(m_enemyBase->rHitbox);
	window.draw(m_unitBase->rHitbox);
#endif

	// Draw enemies (back to front)
	for (auto& [layer, enemyList] : std::ranges::reverse_view(m_enemies))
	{

		for (auto& enemy : enemyList)
		{
			window.draw(enemy->sprite);

#ifdef DEBUG_MODE
			// Draw hitboxes for debugging
			window.draw(enemy->rDamageZone);
			window.draw(enemy->rAttackRangeZone);
			window.draw(enemy->rHitbox);
#endif
		}
	}

	// Draw units (back to front)
	for (auto& [layer, unitList] : std::ranges::reverse_view(m_units))
	{

		for (auto& unit : unitList)
		{
			window.draw(unit->sprite);

#ifdef DEBUG_MODE
			// Draw hitboxes for debugging
			window.draw(unit->rDamageZone);
			window.draw(unit->rAttackRangeZone);
			window.draw(unit->rHitbox);
#endif
		}
	}
}

/*
* Spawn the two bases in their positions
*/
void Stage::spawn_bases(float health, std::string texture)
{
	m_enemyBase = std::make_unique<BattleBase>(health, texture);
	m_enemyBase->position = { 20.0f, 540.0f - m_enemyBase->texture.getSize().y / 2 }; //20px to the right

	m_unitBase = std::make_unique<BattleBase>(350.0f, "assets/images/textures/bases/fishBaseTEST.png");
	m_unitBase->position = { 1900.0f - m_unitBase->texture.getSize().x, 540.0f - m_unitBase->texture.getSize().y / 2 }; //20px to the left
}

/*
* Spawn a new enemy
*/
void Stage::spawn_enemy(std::shared_ptr<EntityData> enemyData, sf::Vector2f magnification, int layer = -1, bool isBoss = false, bool bypassLimit = false)
{
	if (m_enemiesCount >= m_enemiesLimit && !bypassLimit) return;

	std::shared_ptr<BattleEnemy> battleEnemy = std::make_shared<BattleEnemy>(enemyData, magnification);
	battleEnemy->set_current_stage(shared_from_this());
	battleEnemy->init_state_machine();
	battleEnemy->stateMachine->change_state(std::make_unique<IdleState>(battleEnemy->stateMachine));

	// Assign layer
	battleEnemy->currentLayer = (layer > 0) ? layer : generate_random_spawn_layer();
	battleEnemy->update_position();
	battleEnemy->update_sprite();

	if (isBoss) generate_boss_shockwave();

	m_enemiesCount++;
	m_enemies[battleEnemy->currentLayer].push_back(battleEnemy);
}

/*
* Spawn a new unit
*/
void Stage::spawn_unit(std::shared_ptr<EntityData> unitData)
{
	if (m_unitsCount >= m_unitsLimit) return;
	if (unitData->cost > m_currentCash) return;

	sf::Vector2f unitMagnification = { 1.f, 1.f };

	// Adjust unit stats based on player level
	if (auto player = m_dataLoader->get_player_data().lock())
	{
		if (player->is_unit_owned(unitData->UID))
		{
			int level = player->get_unit_level(unitData->UID);
			if (level > 1)
			{
				float growth = 0.15f;
				float multiplier = std::pow(1.f + growth, level - 1);
				unitMagnification.x = multiplier;
				unitMagnification.y = multiplier;
			}
		}
	}

	std::shared_ptr<BattleUnit> battleUnit = std::make_shared<BattleUnit>(unitData, unitMagnification);
	battleUnit->set_current_stage(shared_from_this());
	battleUnit->init_state_machine();
	battleUnit->stateMachine->change_state(std::make_unique<IdleState>(battleUnit->stateMachine));

	battleUnit->currentLayer = generate_random_spawn_layer();
	battleUnit->update_position();
	battleUnit->update_sprite();

	m_unitsCount++;
	m_currentCash -= unitData->cost;
	m_units[battleUnit->currentLayer].push_back(battleUnit);
}

/*
* Remove enemy from stage
*/
void Stage::remove_enemy(BattleEnemy battleEnemy)
{
	m_enemies.erase(battleEnemy.currentLayer);
}

/*
* Remove unit from stage
*/
void Stage::remove_unit(BattleUnit battleUnit)
{
	m_units.erase(battleUnit.currentLayer);
}

/*
* Generate random spawn layer for entities
*/
int Stage::generate_random_spawn_layer()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(0, 50);

	return distrib(gen);
}

/*
* Apply shockwave effect to all units when a boss spawns
*/
void Stage::generate_boss_shockwave()
{
	for (auto& pair : m_units)
	{
		for (auto& unit : pair.second)
		{
			auto activeId = unit->stateMachine->get_active_state_id();
			if (activeId == "KNOCKBACK" || activeId == "DEAD")
				continue;

			unit->isOnShockwave = true;
			unit->stateMachine->change_state(std::make_unique<KnockbackState>(unit->stateMachine));
		}
	}
}
