#pragma once

#include "tweeny.h"
#include "machine/StateMachine.h"
#include "../../data/EntityData.h"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <set>
#include <memory>
//#include <vector>
#include <map>

class Stage;

class BattleEntity : public std::enable_shared_from_this<BattleEntity>
{
public:

	BattleEntity();
	virtual ~BattleEntity();

	virtual void update(float deltaTime, const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList) = 0; //called each frame by the stage instance
	virtual void update_position() = 0; //used to change position of all float_rects and sprites (called inside of update)
	virtual void update_sprite();

	void init_state_machine();

	void set_current_stage(std::shared_ptr<Stage> stage);

public:

	//Data
	std::shared_ptr<EntityData> data = nullptr;

	//State machine
	std::shared_ptr<StateMachine> stateMachine;

	//Current Stage
	std::weak_ptr<Stage> currentStage;

	//Data submembers
	float currentHealth = 1.0f;

	tweeny::tween<float> tweenX; //Tweens used to play the knockback animation
	tweeny::tween<float> tweenY;

	bool isOnShockwave = false; //used to check if the entity is on a shockwave (preventing to update healthLeftBeforeNextKnockback)
	float healthLeftBeforeNextKnockback = 0.0f; //used to calculate knockback, if currentHealth is less than this value, the entity will be knocked back

	float currentAttackCooldown = 0.0f; //attack cooldown

	int currentLayer = 0;

	bool isDead = false;

	//Boosts
	sf::Vector2f magnification = { 1.f, 1.f }; // x => hp | y => attack (multiplier)

	//Position
	sf::Vector2f position;
	sf::Vector2f velocity;

	//Battle
	sf::FloatRect hitbox;
	sf::FloatRect attackRangeZone;
	sf::FloatRect damageZone;

	//Targets
	struct WeakPtrPosXLess {
		bool operator()(const std::weak_ptr<BattleEntity>& a, const std::weak_ptr<BattleEntity>& b) const;
	};
	std::set<std::weak_ptr<BattleEntity>, WeakPtrPosXLess > targets;

	//Render
	sf::Texture texture;
	sf::Sprite sprite;
	float timeUntilNextFrame = 0.1f; //used to calculate the time until the next frame is rendered (in seconds)
	float currentFrameCooldown = 0.0f;
	int currentFrameIndex = 0; //used to calculate the current frame of the sprite animation

#ifdef DEBUG_MODE
	//Debug Rendering
	sf::RectangleShape rHitbox;
	sf::RectangleShape rAttackRangeZone;
	sf::RectangleShape rDamageZone;
#endif
};