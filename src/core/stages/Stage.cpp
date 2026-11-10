#include "Stage.h"

#include "../entities/BattleEnemy.h"
#include "../entities/BattleUnit.h"

#include "../entities/machine/states/KnockbackState.h"
#include "../entities/machine/states/IdleState.h"

#include "../../uis/UIManager.h"
#include "../../utils/ImageTools.h"
#include "../../audio/AudioManager.h"

#include <iostream>

namespace
{
	const std::string DEFAULT_ATTACK_SOUND = "attack_slap";   // Fish and enemies without an "attackSound"
}

/*
* Initializes the stage with the DataLoader, UIManager and AudioManager
*/
void Stage::init(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<UIManager> uiManager, std::shared_ptr<AudioManager> audio, sf::View* stageCamera)
{
	m_dataLoader = dataLoader;
	m_uiManager = uiManager;
	m_audio = audio;
	m_stageCamera = stageCamera;
	m_statusManager.init(dataLoader);

	// A status that lands makes its sound (zap, goo, sad "wah")
	m_statusManager.set_on_status_landed([this](StatusType status)
		{
			switch (status)
			{
			case StatusType::PARALYZED: play_sound("status_paralyze"); break;
			case StatusType::SLOW:      play_sound("status_slow"); break;
			case StatusType::WEAKEN:    play_sound("status_weaken"); break;
			default: break;
			}
		});

	if (!m_shadowTexture.loadFromFile(SHADOW_TEXTURE))
		std::cerr << "Stage: can't load " << SHADOW_TEXTURE << "\n";
	m_shadowTexture.setSmooth(true);

	unload();
}

/*
* Constructor: initialize pointers and textures
*/
Stage::Stage():
	m_enemyBase(nullptr), m_unitBase(nullptr),

	//m_baseTexture(sf::Texture()), m_baseSprite(sf::Sprite(m_baseTexture)), 
	m_backgroundTexture(sf::Texture())
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
	m_maxCash = BASE_MAX_CASH;               // Cash Up upgrades only last for one stage
	m_cashPerSecond = BASE_CASH_PER_SECOND;
	m_cashRemainder = 0.f;
	m_length = stageJson->length;

	// Spawn bases with provided health and texture
	spawn_bases(stageJson->baseHealth, stageJson->playerBaseHealth, stageJson->baseTexture);

	// Copy enemy spawn data from stage JSON
	m_enemyStageDatas.clear();
	for (auto& enemy : stageJson->enemies)
	{
		auto enemyCopy = std::make_shared<EnemyStageData>(*enemy);
		enemyCopy->currentTimer = enemyCopy->respawnTime; // initialize spawn timer
		m_enemyStageDatas.push_back(enemyCopy);
	}

	// Load the background (any size) and repeat it along the stage
	const BackgroundData background = m_dataLoader->get_background_data(stageJson->backgroundTexture);
	load_background_texture(background);
	init_background(background.groundRow);

	focus_on_player_base(); // Focus camera

	// The stage's theme, and the one it switches to when the enemy base gets low (FishEditor, Music tab)
	m_musicChange = stageJson->musicChange;
	m_musicChangeBaseHealth = stageJson->musicChangeBaseHealth;
	m_hasMusicChanged = false;
	if (m_audio) m_audio->queue_music(stageJson->music);   // After the battle start jingle

	m_isLoaded = true;
}

/*
* Switches to the second theme once the enemy base is at or below its health % (like The Battle Cats' boss music)
*/
void Stage::update_music()
{
	if (m_hasMusicChanged || m_musicChange.empty() || !m_audio || !m_enemyBase) return;
	if (m_enemyBase->currentHealth > m_enemyBase->maxHealth * (m_musicChangeBaseHealth / 100.f)) return;

	m_audio->play_music(m_musicChange, 1.5f);
	m_hasMusicChanged = true;
}

void Stage::reward_kill(const BattleEntity& enemy)
{
	const int amount = enemy.data ? enemy.data->cashDrop : 0;
	if (amount <= 0) return;

	m_currentCash = std::min(m_maxCash, m_currentCash + amount);

	const sf::FloatRect bounds = enemy.sprite.getGlobalBounds();
	m_cashPopups.push_back({ { bounds.position.x + bounds.size.x / 2.f, bounds.position.y }, amount, 0.f });
	play_sound("cash_drop");
}

const std::vector<CashPopup>& Stage::get_cash_popups() const
{
	return m_cashPopups;
}

bool Stage::is_unit_limit_reached() const
{
	return m_unitsCount >= m_unitsLimit;
}

void Stage::play_sound(const std::string& name, float volumeScale) const
{
	if (m_audio) m_audio->play_sound(name, volumeScale);
}

void Stage::on_attack_landed(const BattleEntity& attacker, bool hasHitBase) const
{
	const std::string& sound = attacker.data && !attacker.data->attackSound.empty() ? attacker.data->attackSound : DEFAULT_ATTACK_SOUND;
	play_sound(sound);
	if (hasHitBase) play_sound("base_hit");
}

/*
* Unload all stage data and entities
*/
void Stage::unload()
{
	m_uid = -1;

	m_currentCash = 0;
	m_cashRemainder = 0.f;
	m_cashPopups.clear();
	m_nextEntityId = 0;

	m_enemies.clear();
	m_units.clear();
	m_enemyStageDatas.clear();
	m_backgroundSprites.clear();
	m_statusManager.clear_log();             // Statuses themselves are removed with their entities
	m_combatLog.clear();

	m_enemyBase = nullptr;
	m_unitBase = nullptr;

	m_elapsedTime = 0.f;

	apply_zoom(1.f / m_currentZoom);
	m_currentZoom = 1.f;

	m_isLoaded = false;
	m_isOnEndScreen = false;
}

/*
* Main update function: spawn enemies, update entities, check win/loss
*/
void Stage::update(float deltaTime)
{
	if (!m_isLoaded) return;

	m_elapsedTime += deltaTime;

	// The "+X$" popups rise and fade, then go
	for (auto& popup : m_cashPopups) popup.age += deltaTime;
	std::erase_if(m_cashPopups, [](const CashPopup& popup) { return popup.age >= CASH_POPUP_SECONDS; });

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
			spawn_enemy(data, spawnEnemyData->magnification, spawnEnemyData->layer, spawnEnemyData->isBoss, spawnEnemyData->bypassEnemyLimit);

			spawnEnemyData->currentTimer = 0.f;
			spawnEnemyData->spawnedCount++;
		}
	}

	update_cash(deltaTime);

	// Count down statuses before entities act, so an expired status no longer affects them this frame
	m_statusManager.update(deltaTime, m_units);
	m_statusManager.update(deltaTime, m_enemies);

	update_bases(deltaTime);
	update_enemies(deltaTime);
	update_units(deltaTime);

	// Doesn't need to check the bases if the player is on an end screen
	if (m_isOnEndScreen) return;

	update_music();

	// Checks if the Player has won or lost
	if (is_enemy_base_destroyed())
	{
		// Player has won
		if (auto player = m_dataLoader->get_player_data().lock())
		{
			auto stageData = m_dataLoader->get_stage_data(m_uid);
			if (stageData)
			{
				// A first win opens the map on the newest stage; a replay keeps it here (farming)
				if (!player->has_completed_stage(m_uid))
					player->lastStage = -1;
				player->complete_stage(stageData);
			}

			m_dataLoader->save_progress();
		}
		
		m_enemies.clear();

		// The enemy base blows up, the victory jingle plays
		play_sound("base_destroyed");
		if (m_audio) m_audio->play_jingle("jingle_victory");

		m_isOnEndScreen = true;
		m_uiManager.lock()->generate_victory_uis();
		return;
	}

	if (is_unit_base_destroyed())
	{
		// Player has lost (the enemies met are still saved)
		m_dataLoader->save_progress();

		m_units.clear();

		// The fish base blows up, the defeat jingle plays
		play_sound("base_destroyed");
		if (m_audio) m_audio->play_jingle("jingle_defeat");

		m_isOnEndScreen = true;
		m_uiManager.lock()->generate_defeat_uis();
		return;
	}
}

/*
* Increment cash over time
*/
void Stage::update_cash(float deltaTime)
{
	if (deltaTime <= 0.0f) return;
	if (m_currentCash >= m_maxCash) return;

	// Accumulate fractions of cash and only add whole units,
	// so income is exactly m_cashPerSecond whatever the frame rate
	m_cashRemainder += m_cashPerSecond * deltaTime;

	int wholeCash = static_cast<int>(m_cashRemainder);
	m_currentCash += wholeCash;
	m_cashRemainder -= static_cast<float>(wholeCash);

	if(m_currentCash > m_maxCash)
		m_currentCash = m_maxCash;

	//std::cout << "[CURRENT BANK]: " << m_currentCash << "$ / " << m_maxCash << "$\n";
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
				reward_kill(*enemy);

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
				itvec = unitList.erase(itvec); // TODO: create remove_unit();
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

	// Increase cash per second by 50% per level
	m_cashPerSecond = 50.0f * std::pow(1.5f, static_cast<float>(level));

	return true;
}

void Stage::apply_zoom(float zoom)
{
	if (!m_stageCamera) return;

	float newZoom = m_currentZoom * zoom;
	newZoom = std::clamp(newZoom, 0.9f, 1.5f);

	float appliedFactor = newZoom / m_currentZoom;

	m_stageCamera->zoom(appliedFactor);
	m_currentZoom = newZoom;

	clamp_camera();
}

void Stage::focus_on_player_base()
{
	if (!m_stageCamera) return;
	if (!m_unitBase || !m_enemyBase) return;

	// Max Zoom
	apply_zoom(0.9f);

	// Move to player base
	sf::FloatRect baseBounds = m_unitBase->sprite.getGlobalBounds();
	float baseCenterX = baseBounds.position.x + baseBounds.size.x * 0.5f;

	sf::Vector2f center = m_stageCamera->getCenter();
	float viewWidth = m_stageCamera->getSize().x;

	center.x = baseCenterX - viewWidth * 0.25f;
	
	m_stageCamera->setCenter(center);
	
	clamp_camera();
}

void Stage::clamp_camera()
{
	if (!m_stageCamera) return;
	if (!m_unitBase || !m_enemyBase) return;

	sf::Vector2f center = m_stageCamera->getCenter();
	float viewWidth = m_stageCamera->getSize().x;

	const float margin = 0.25f;
	const float baseWidth = 200.f; // Width of a base in pixels

	float minCenterX = m_enemyBase->position.x + baseWidth + (0.5f - margin) * viewWidth;
	float maxCenterX = m_unitBase->position.x - (0.5f - margin) * viewWidth;

	if (minCenterX > maxCenterX)
	{
		minCenterX = maxCenterX = (m_enemyBase->position.x + m_unitBase->position.x) * 0.5f;
	}

	center.x = std::clamp(center.x, minCenterX, maxCenterX);

	m_stageCamera->setCenter(center);
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

int Stage::get_current_uid() const
{
	return m_uid;
}

const DamageCalculator& Stage::get_damage_calculator() const
{
	return m_damageCalculator;
}

StatusManager& Stage::get_status_manager()
{
	return m_statusManager;
}

CombatLog& Stage::get_combat_log()
{
	return m_combatLog;
}

void Stage::set_cash(int cash)
{
	m_currentCash = std::clamp(cash, 0, m_maxCash);
}

float Stage::get_elapsed_time() const
{
	return m_elapsedTime;
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
	for (auto& bgSprite : m_backgroundSprites)
		window.draw(bgSprite);
	draw_base_shadow(window, *m_enemyBase);
	draw_base_shadow(window, *m_unitBase);
	window.draw(m_enemyBase->sprite);
	window.draw(m_unitBase->sprite);

	// Shadows on the ground, under every unit and enemy
	for (auto& [layer, enemyList] : m_enemies)
		for (auto& enemy : enemyList)
			draw_entity_shadow(window, *enemy, false);
	for (auto& [layer, unitList] : m_units)
		for (auto& unit : unitList)
			draw_entity_shadow(window, *unit, true);

	// Draw enemies (back to front)
	for (auto& [layer, enemyList] : std::ranges::reverse_view(m_enemies))
	{

		for (auto& enemy : enemyList)
		{
			window.draw(enemy->sprite);

		}
	}

	// Draw units (back to front)
	for (auto& [layer, unitList] : std::ranges::reverse_view(m_units))
	{

		for (auto& unit : unitList)
		{
			window.draw(unit->sprite);

		}
	}
}

/*
* Spawn the two bases in their positions.
* They stand on the last layer (BACK_LAYER_Y), behind every unit and enemy,
* on the ground of the background.
*/
void Stage::spawn_bases(float health, float playerBaseHealth, const std::string& texture)
{
	m_enemyBase = std::make_unique<BattleBase>(health, texture);
	m_enemyBase->position = { 0.0f, BACK_LAYER_Y - m_enemyBase->texture.getSize().y };
	m_enemyBase->uniqueId = m_nextEntityId++;
	m_enemyBase->update_position(1/60.f);

	m_unitBase = std::make_unique<BattleBase>(playerBaseHealth, "assets/images/textures/bases/fishBaseTEST.png");
	m_unitBase->position = { m_length, BACK_LAYER_Y - m_unitBase->texture.getSize().y };
	m_unitBase->uniqueId = m_nextEntityId++;
	m_unitBase->update_position(1 / 60.f);
}

/*
* Loads the background image into m_backgroundTexture.
* With a seamless overlap (backgrounds.json), its edges are blended so the copies join without a seam
* (the file itself isn't changed). The rows don't move, so the ground row stays valid.
*/
void Stage::load_background_texture(const BackgroundData& background)
{
	sf::Image image;
	if (!image.loadFromFile(background.texture))
	{
		std::cerr << "Stage: can't load the background " << background.texture << "\n";
		return;
	}

	if (background.seamlessOverlap > 0)
		image = ImageTools::make_seamless(image, static_cast<unsigned>(background.seamlessOverlap));

	if (!m_backgroundTexture.loadFromImage(image))
		std::cerr << "Stage: can't create the background texture of " << background.texture << "\n";
}

/*
* Copies of the background side by side along the whole stage.
* Works with any image size: a copy every (image width x BACKGROUND_SCALE) pixels, and the image's
* ground row (backgrounds.json, or the template's ratio) on the line units stand on.
* With the 1280 x 1120 template: a copy every 1920 px, top at y = -200.
*/
void Stage::init_background(float groundRow)
{
	// Rebuilt from scratch on every load
	m_backgroundSprites.clear();

	const sf::Vector2u textureSize = m_backgroundTexture.getSize();
	if (textureSize.x == 0 || textureSize.y == 0) return;

	const float tileWidth = textureSize.x * BACKGROUND_SCALE;
	const float top = BATTLE_GROUND_Y - background_ground_row(groundRow, static_cast<float>(textureSize.y)) * BACKGROUND_SCALE;
	const int numberOfSprites = static_cast<int>(m_length / tileWidth) + 2; // One more past each base

	for (int i = 0; i <= numberOfSprites; i++)
	{
		sf::Sprite backgroundSprite(m_backgroundTexture);
		backgroundSprite.setScale({ BACKGROUND_SCALE, BACKGROUND_SCALE });
		backgroundSprite.setPosition({ -tileWidth + i * tileWidth, top });
		m_backgroundSprites.push_back(backgroundSprite);
	}
}

/*
* Spawn a new enemy
*/
void Stage::spawn_enemy(std::shared_ptr<EntityData> enemyData, sf::Vector2f magnification, int layer = -1, bool isBoss = false, bool bypassLimit = false)
{
	if (m_enemiesCount >= m_enemiesLimit && !bypassLimit) return;
	if (m_isOnEndScreen == true) return;

	std::shared_ptr<BattleEnemy> battleEnemy = std::make_shared<BattleEnemy>(enemyData, magnification);
	battleEnemy->set_current_stage(shared_from_this());
	battleEnemy->init_state_machine();
	battleEnemy->stateMachine->change_state(std::make_unique<IdleState>(battleEnemy->stateMachine.get()));

	// Assign layer
	battleEnemy->uniqueId = m_nextEntityId++;
	battleEnemy->currentLayer = (layer > 0) ? layer : generate_random_spawn_layer();
	battleEnemy->position.x = 0.f;
	battleEnemy->update_position(1.f/60.f);
	battleEnemy->update_sprite();

	if (isBoss) generate_boss_shockwave();

	// Met: the chapter map and the encyclopedia show it from now on (saved with the progress)
	if (auto player = m_dataLoader->get_player_data().lock())
		player->see_enemy(enemyData->UID);

	m_enemiesCount++;
	m_enemies[battleEnemy->currentLayer].push_back(battleEnemy);
}

/*
* Spawn a new unit
*/
bool Stage::spawn_unit(std::shared_ptr<EntityData> unitData, bool ignoreCostAndLimit)
{
	if (m_unitsCount >= m_unitsLimit && !ignoreCostAndLimit) return false;
	if (unitData->cost > m_currentCash && !ignoreCostAndLimit) return false;
	if (m_isOnEndScreen == true) return false;

	sf::Vector2f unitMagnification = { 1.f, 1.f };

	// Adjust unit stats based on player level
	if (auto player = m_dataLoader->get_player_data().lock())
	{
		if (player->is_unit_owned(unitData->UID))
		{
			const float multiplier = PlayerData::stat_multiplier(player->get_unit_level(unitData->UID));
			unitMagnification.x = multiplier;
			unitMagnification.y = multiplier;
		}
	}

	std::shared_ptr<BattleUnit> battleUnit = std::make_shared<BattleUnit>(unitData, unitMagnification);
	battleUnit->set_current_stage(shared_from_this());
	battleUnit->init_state_machine();
	battleUnit->stateMachine->change_state(std::make_unique<IdleState>(battleUnit->stateMachine.get()));

	battleUnit->uniqueId = m_nextEntityId++;
	battleUnit->currentLayer = generate_random_spawn_layer();
	battleUnit->position.x = m_length;
	battleUnit->update_position(1.f / 60.f);
	battleUnit->update_sprite();

	m_unitsCount++;
	if (!ignoreCostAndLimit) m_currentCash -= unitData->cost;
	m_units[battleUnit->currentLayer].push_back(battleUnit);

	play_sound("deploy");
	return true;
}

/*
* Generate random spawn layer for entities
*/
int Stage::generate_random_spawn_layer()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(0, MAX_BATTLE_LAYER);

	return distrib(gen);
}

/*
* Apply shockwave effect to all units when a boss spawns
*/
void Stage::generate_boss_shockwave()
{
	play_sound("boss_shockwave");

	for (auto& pair : m_units)
	{
		for (auto& unit : pair.second)
		{
			auto activeId = unit->stateMachine->get_active_state_id();
			if (activeId == "KNOCKBACK" || activeId == "DEAD")
				continue;

			unit->isOnShockwave = true;
			unit->stateMachine->change_state(std::make_unique<KnockbackState>(unit->stateMachine.get()));
		}
	}
}


/*
* Soft round shadow on the ground (like The Battle Cats), drawn before what stands on it.
*/
void Stage::draw_shadow(sf::RenderWindow& window, const ShadowShape& shape) const
{
	const sf::Vector2f textureSize(m_shadowTexture.getSize());
	if (textureSize.x <= 0.f || textureSize.y <= 0.f || shape.width <= 0.f) return;

	sf::Sprite shadow(m_shadowTexture);
	shadow.setOrigin(textureSize / 2.f);
	shadow.setScale({ shape.width / textureSize.x, shape.height / textureSize.y });
	shadow.setPosition({ shape.centerX, shape.centerY });
	window.draw(shadow);
}

/*
* Under the body of a unit or an enemy (BattlefieldLayout's entity_shadow), its width set by EntityData::shadowScale,
* moved forward by EntityData::shadowOffset.
* It follows the sprite along x, so it slides with a knockback, but stays on the ground (position.y raised by the layer).
* The body is the back half of the frame: right of the sprite center for units (they face left), left for enemies.
*/
void Stage::draw_entity_shadow(sf::RenderWindow& window, const BattleEntity& entity, bool isUnit) const
{
	const float bodyWidth = entity.hitbox.size.x;
	const float shadowOffset = entity.data ? entity.data->shadowOffset : 0.f;
	const float shadowScale = entity.data ? entity.data->shadowScale : 1.f;
	const float centerX = entity_shadow_x(entity.sprite.getPosition().x, bodyWidth, isUnit, shadowOffset);
	const float feetY = entity.position.y - static_cast<float>(entity.currentLayer);
	draw_shadow(window, entity_shadow(centerX, feetY, bodyWidth, shadowScale));
}

/*
* Under the whole base image, centered on its bottom edge (bases stand on the last layer).
*/
void Stage::draw_base_shadow(sf::RenderWindow& window, const BattleBase& base) const
{
	const sf::FloatRect bounds = base.sprite.getGlobalBounds();
	draw_shadow(window, base_shadow(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y, bounds.size.x));
}
