#include "BattleEnemy.h"
#include "../data/EnemyData.h"

#include <iostream>

//BattleEnemy::BattleEnemy() = default;

BattleEnemy::BattleEnemy(std::shared_ptr<EnemyData> data_, sf::Vector2f magnification_):
	data(data_)
{
	//Init core datas
	magnification = magnification_;

	currentHealth = data->health * magnification.x;
	healthLeftBeforeNextKnockback = data->health - (data->health / data->knockbackCount);

	state = IDLE;
	position = { 0.0f, 720.f * 2 / 3 };

	currentAttackCooldown = data->attackFrequency; //Set current attack cooldown to attack frequency to make them attack instantly

	currentKnockbackCooldown = 0.f;

	//Init sprite
	bool isTextureLoaded = texture.loadFromFile(data->texture);
	sprite.setTexture(texture, true);
	sprite.setOrigin({ static_cast<float>(texture.getSize().x), static_cast<float>(texture.getSize().y) });

	//Init battle zones
	hitbox.size = { static_cast<float>(texture.getSize().x), 720.0f};
	attackRangeZone.size = { data->attackRange + static_cast<float>(texture.getSize().x) , 720.0f };
	damageZone.size = { data->attackRange + static_cast<float>(texture.getSize().x) , 720.0f };

	//Init position
	hitbox.position = position;
	attackRangeZone.position = position;
	damageZone.position = position;

#ifdef DEBUG_MODE
	//Init debug rectangles
	rHitbox.setSize(hitbox.size);
	rHitbox.setPosition(position);

	rAttackRangeZone.setSize(attackRangeZone.size);
	rAttackRangeZone.setPosition(position);

	rDamageZone.setSize(damageZone.size);
	rDamageZone.setPosition(position);
#endif
}

void BattleEnemy::update(float deltaTime, const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList)
{
	//Simple state machine
	if (state == IDLE)
	{
		velocity = { 0.0f, 0.0f };
		if (currentHealth < 0.0f) state = KNOCKBACK;
		if (currentHealth <= healthLeftBeforeNextKnockback) state = KNOCKBACK;

		//Check the attack range
		for (auto& pair : entityList)
		{
			auto& unitList = pair.second;

			for (auto& unit : unitList)
			{
				bool isUnitInAttackRange = attackRangeZone.findIntersection(unit->hitbox).has_value();
				if (isUnitInAttackRange)
				{
					isEntityOnRange = true;
					break;
				}
			}
			if (isEntityOnRange) break;
		}

		if (isEntityOnRange == false && targets.empty()) state = WALK;

		if (currentAttackCooldown >= data->attackFrequency && isEntityOnRange == true)
		{
			state = ATTACK;
		}

		isEntityOnRange = false;
	}
	if (state == WALK)
	{
		velocity = { data->movementSpeed * 10.0f * deltaTime, 0.0f }; //*10.0f to make them more speedy
		position += velocity;
		if (currentHealth < 0.0f) state = KNOCKBACK;
		else state = IDLE;
	}
	if (state == ATTACK)
	{
		//Get targets in damage zone
		for (auto& pair : entityList)
		{
			auto& unitList = pair.second;

			for (auto& unit : unitList)
			{
				bool isUnitInAttackRange = damageZone.findIntersection(unit->hitbox).has_value();
				if (isUnitInAttackRange && unit->state != unit->KNOCKBACK)
				{
					//std::cout << "UNIT FOUND >:(\n";
					targets.insert(unit);
				}
			}
		}

		if (targets.empty())
		{
			state = IDLE; //No targets to attack
			goto attackend;
		}

		//std::cout << "SLASH!\n";

		if (data->attackType == 1) //if attackType => single
		{
			auto target = targets.begin().operator*().lock();
			if (target)
			{
				target->currentHealth -= data->attackPower * magnification.y; //Deal damage to the first target
			}
		}
		else
		{
			for (auto& targetPtr : targets)
			{
				if (auto target = targetPtr.lock())
				{
					target->currentHealth -= data->attackPower * magnification.y;
					//std::cout << "Dealt " << data->attackPower * magnification.y << "damage.\n";
				}
			}
		}

	attackend:
		targets.clear();
		currentAttackCooldown = 0.0f;
		state = IDLE;
	}
	if (state == KNOCKBACK)
	{
		if (enteredKnockback)
		{
			tweenX = tweeny::from(position.x).to(position.x -= knockbackDistancePx).during(60.0f * knockbackDuration).via(tweeny::easing::quadraticOut);
			tweenY = tweeny::from(position.y - static_cast<float>(currentLayer)).to(position.y - static_cast<float>(currentLayer) - 50.0f)
				.during(30.0f * knockbackDuration).via(tweeny::easing::quadraticOut)
				.to(position.y - static_cast<float>(currentLayer)).during(30.0f * knockbackDuration).via(tweeny::easing::bounceOut);
			//position.x -= knockbackDistancePx;
			enteredKnockback = false;
		}

		currentKnockbackCooldown += deltaTime;

		if (currentKnockbackCooldown >= knockbackDuration)
		{
			if (currentHealth < 0.0f)
			{
				state = DEAD;
				//std::cout << "Knockback led to death.\n";
			}
			else
			{
				state = IDLE; //After knockback, the unit dies
				currentKnockbackCooldown = 0.0f; //Reset cooldown
				healthLeftBeforeNextKnockback -= (data->health / data->knockbackCount); //Update healthLeftBeforeNextKnockback
				enteredKnockback = true;
			}
		}
		else
			state = KNOCKBACK; //Continue knockback
	}
	if (state == DEAD)
	{
		isDead = true;
		return;
	}

	update_position();

	currentAttackCooldown += deltaTime;
}

void BattleEnemy::update_position()
{
	if (state == KNOCKBACK)
	{
		if (tweenX.progress() < 1.0f && tweenY.progress() < 1.0f)
		{
			sprite.setPosition({ tweenX.step(1) + static_cast<float>(texture.getSize().x), tweenY.step(1) });
		}
	}
	else
	{
		sprite.setPosition({ position.x + static_cast<float>(texture.getSize().x), position.y - static_cast<float>(currentLayer) });
	}


	hitbox.position = position;
	attackRangeZone.position = position;
	damageZone.position = position;

#ifdef DEBUG_MODE
	//Update debug rectangles' position
	rHitbox.setPosition(hitbox.position);
	rAttackRangeZone.setPosition(attackRangeZone.position);
	rDamageZone.setPosition(damageZone.position);
#endif
}
