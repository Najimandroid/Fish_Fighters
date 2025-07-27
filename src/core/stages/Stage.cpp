#include "Stage.h"

#include "../BattleEnemy.h"

#include <iostream>

void Stage::init(std::shared_ptr<DataLoader> dataLoader)
{
	m_dataLoader = dataLoader;
}

Stage::Stage():
	m_baseHitbox(sf::FloatRect({0, 0}, {1, 720})), m_fishBaseHitbox(sf::FloatRect({ 0, 0 }, { 1, 720 })),

	m_baseTexture(sf::Texture()), m_baseSprite(sf::Sprite(m_baseTexture)), 
	m_backgroundTexture(sf::Texture()), m_backgroundSprite(sf::Sprite(m_backgroundTexture))
{}

void Stage::load(int uid)
{
	//Loading stage with the StageData uid

	std::shared_ptr<const StageData> stageJson = m_dataLoader->get_stage_data(uid);

	//Clear previous data
	m_enemies.clear();
	m_enemyStageDatas.clear();

	//setup stage data
	m_uid = uid;
	m_stageName = stageJson->name;

	m_enemiesLimit = stageJson->enemiesLimit;
	m_unitsLimit = stageJson->unitsLimit;

	m_enemiesCount = 0;
	m_unitsCount = 0;

	m_enemyStageDatas = stageJson->enemies;

	//Setup base sprite
	if (!m_baseTexture.loadFromFile(stageJson->baseTexture)) { bool loadDefault = m_baseTexture.loadFromFile("assets/images/textures/bases/defaultBase.png"); }
	m_baseSprite.setTexture(m_baseTexture, true);
	m_baseSprite.setPosition({ 0.0f, 360.0f - m_baseTexture.getSize().y / 2}); //the base is placed in the center of the screen
	m_baseHitbox.position = m_baseSprite.getPosition();

	m_fishBaseSprite.setPosition({ 1280.0f - m_fishBaseTexture.getSize().x, 360.0f - m_fishBaseTexture.getSize().y / 2});
	m_fishBaseHitbox.position = m_fishBaseSprite.getPosition();

	//Setup background
	bool bg = m_backgroundTexture.loadFromFile(stageJson->backgroundTexture); //to do: adding default bground
	m_backgroundSprite.setTexture(m_backgroundTexture, true);
	m_backgroundSprite.setPosition({ 0.0f, 0.0f });

	m_isLoaded = true;
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
			spawn_enemy(data, spawnEnemyData->layer, spawnEnemyData->isBoss);

			spawnEnemyData->currentTimer = 0.f;
			spawnEnemyData->spawnedCount++;
		}
	}

	update_enemies(deltaTime);
}

void Stage::update_enemies(float deltaTime)
{
	if (m_enemies.empty()) return;
	for (auto& [layer, enemy] : m_enemies)
	{

		enemy->updatePosition();

		bool isFishBaseReached = enemy->attackRangeZone.findIntersection(m_fishBaseHitbox).has_value();
		if (isFishBaseReached)
		{
			enemy->state = enemy->IDLE;
		}
		else
		{
			enemy->state = enemy->WALK;
		}


		if (enemy->state == enemy->WALK)
		{
			enemy->velocity = { enemy->data->movementSpeed * 10.0f * deltaTime, 0.0f }; //*10.0f to make them more speedy
			enemy->position += enemy->velocity;
			enemy->velocity = { 0.0f, 0.0f };
		}
	}
}

void Stage::render(sf::RenderWindow& window)
{
	window.draw(m_backgroundSprite);
	window.draw(m_baseSprite);
	window.draw(m_fishBaseSprite);

	for (auto& [layer, enemy] : std::ranges::reverse_view(m_enemies))
	{

		enemy->sprite.setPosition(enemy->position);
		window.draw(enemy->sprite);

#ifdef DEBUG_MODE
		//Rendering the hitboxes in debug mode
		window.draw(enemy->rDamageZone);
		window.draw(enemy->rAttackRangeZone);
		window.draw(enemy->rHitbox);
#endif
	}
}

void Stage::spawn_enemy(std::shared_ptr<EnemyData> enemyData,int layer = -1, bool bypassLimit = false)
{
	if (m_enemiesCount >= m_enemiesLimit && bypassLimit == false) return;

	std::shared_ptr<BattleEnemy> battleEnemy = std::make_shared<BattleEnemy>(enemyData);

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
