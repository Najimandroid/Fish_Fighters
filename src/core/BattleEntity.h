#pragma once

#include "tweeny.h"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <set>
//#include <vector>
#include <map>

class BattleEntity
{
public:

	BattleEntity();
	virtual ~BattleEntity() = default;

	virtual void update(float deltaTime, const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList) = 0; //called each frame by the stage instance
	virtual void update_position() = 0; //used to change position of all float_rects and sprites (called inside of update)

public:
	//Data submembers
	float currentHealth = 1.0f;


	tweeny::tween<float> tweenX; //Tweens used to play the knockback animation
	tweeny::tween<float> tweenY;
	float knockbackDuration = 1.0f; //knockback duration in seconds
	float knockbackDistancePx = 150.0f; //knockback distance in pixels
	float currentKnockbackCooldown = 0.0f;
	bool enteredKnockback = true; //used to check if the entity has entered the knockback state (to setup the tweens once)
	bool isOnShockwave = false; //used to check if the entity is on a shockwave (preventing to update healthLeftBeforeNextKnockback)
	float healthLeftBeforeNextKnockback = 0.0f; //used to calculate knockback, if currentHealth is less than this value, the entity will be knocked back

	float currentAttackCooldown = 0.0f; //attack cooldown
	bool isEntityOnRange = false; //used to check if an entity is on range of the attack range zone (starts the attack state)

	int currentLayer = 0;

	bool isDead = false;

	//State
	enum State
	{
		IDLE = 1,
		WALK,
		ATTACK,
		KNOCKBACK,
		DEAD,
	};
	State state = static_cast<State>(1);

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
	struct BattleEntityRangeComparator
	{
		bool operator()(const std::weak_ptr<BattleEntity>& a, const std::weak_ptr<BattleEntity>& b) const;
	};
	std::set<std::weak_ptr<BattleEntity>, BattleEntityRangeComparator> targets;

	//Render
	sf::Texture texture;
	sf::Sprite sprite;

#ifdef DEBUG_MODE
	//Debug Rendering
	sf::RectangleShape rHitbox;
	sf::RectangleShape rAttackRangeZone;
	sf::RectangleShape rDamageZone;
#endif
};