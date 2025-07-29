#pragma once

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include <set>

class BattleEntity
{
public:

	BattleEntity();
	virtual ~BattleEntity() = default;

	virtual void update(float deltaTime) = 0; //called each frame by the stage instance
	virtual void update_position() = 0; //used to change position of all float_rects and sprites (called inside of update)

public:
	//Data submembers
	float currentHealth = 1.0f;
	float healthLeftBeforeNextKnockback = 0.0f; //used to calculate knockback, if currentHealth is less than this value, the entity will be knocked back
	float currentAttackCooldown = 0.0f; //attack cooldown
	float currentKnockbackCooldown = 0.0f;
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
	std::set<std::weak_ptr<BattleEntity>, std::owner_less<std::weak_ptr<BattleEntity>>> targets;

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