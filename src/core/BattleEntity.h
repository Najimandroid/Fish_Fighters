#pragma once

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

class BattleEntity
{
public:

	BattleEntity();
	virtual ~BattleEntity() = default;

	virtual void updatePosition() = 0;

public:
	//Data submembers
	float currentHealth = 1.0f;
	float attackCooldown = 0.0f;
	int currentLayer = 0;

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