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
	sprite.setOrigin({ static_cast<float>(texture.getSize().x / data->frameCount / 2), static_cast<float>(texture.getSize().y) });
	currentFrameIndex = 0;
	sprite.setTextureRect({ {static_cast<int>(texture.getSize().x / data->frameCount * currentFrameIndex), 0},
	{static_cast<int>(texture.getSize().x / data->frameCount), static_cast<int>(texture.getSize().y)}
		});

	//std::cout << "Spawned rect size x: " << sprite.getTextureRect().size.x << '\n';

	//Init battle zones
	hitbox.size = { static_cast<float>(texture.getSize().x / data->frameCount / 2), 720.0f };
	attackRangeZone.size = { data->attackRange + static_cast<float>(texture.getSize().x / data->frameCount / 2) , 720.0f };
	damageZone.size = { data->attackRange + static_cast<float>(texture.getSize().x / data->frameCount / 2) , 720.0f };

	//std::cout << "hitbox size x: " << static_cast<float>(texture.getSize().x / data->frameCount / 2) << '\n';

	//Init position
	hitbox.position = position;
	attackRangeZone.position = { position.x + data->attackRange + hitbox.size.x, position.y };;
	damageZone.position = { position.x + data->attackRange + hitbox.size.x, position.y };;

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
	state = nextState;

	//Simple state machine
	if (state == IDLE)
	{
		velocity = { 0.0f, 0.0f };
		if (currentHealth < 0.0f) { state = KNOCKBACK; goto endidle; }
		if (currentHealth <= healthLeftBeforeNextKnockback) { state = KNOCKBACK; goto endidle; }

		//Check the attack range
		for (auto& pair : entityList)
		{
			if (!targets.empty()) { isEntityOnRange = true; break; } //If there are already targets, no need to check the range

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

		if (isEntityOnRange == false) state = WALK;

		if (currentAttackCooldown >= data->attackFrequency && isEntityOnRange == true)
		{
			state = ATTACK;
		}

	endidle:
		isEntityOnRange = false;
	}
	if (state == WALK)
	{
		velocity = { data->movementSpeed * 10.0f * deltaTime, 0.0f }; //*10.0f to make them more speedy
		position += velocity;
		if (currentHealth < 0.0f) state = KNOCKBACK;
		else nextState = IDLE;
	}
	if (state == ATTACK)
	{

		if (currentAttackSwingTime <= data->foreswingTime + data->backswingTime)
		{
			if (currentAttackSwingTime >= data->foreswingTime && hasAttacked == true) isAttackReady = false;
			else if (currentAttackSwingTime >= data->foreswingTime) isAttackReady = true;
			else isAttackReady = false;

			currentAttackSwingTime += deltaTime;
			nextState = ATTACK; //Continue attack animation
		}
		else
		{
			hasAttacked = false;
			isAttackReady = false;
			currentAttackSwingTime = 0.0f; //Reset attack swing time
			currentAttackCooldown = 0.0f;
			nextState = IDLE;
		}

		if (isAttackReady == false || hasAttacked == true) goto endattack;
		hasAttacked = true;

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
			nextState = IDLE; //No targets to attack
			goto endattack;
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

	endattack:
		targets.clear();
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

			isAttackReady = false;
			hasAttacked = false;
			currentAttackSwingTime = 0.0f; //Reset attack swing time

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
				nextState = IDLE; //After knockback, the unit dies
				currentKnockbackCooldown = 0.0f; //Reset cooldown

				enteredKnockback = true;
				if (isOnShockwave == false)
					healthLeftBeforeNextKnockback -= (data->health / data->knockbackCount); //Update healthLeftBeforeNextKnockback

				isOnShockwave = false;
			}
		}
		else
			nextState = KNOCKBACK; //Continue knockback
	}
	if (state == DEAD)
	{
		isDead = true;
		return;
	}

	update_position();
	update_sprite();

	currentAttackCooldown += deltaTime;
	currentFrameCooldown += deltaTime;
}

void BattleEnemy::update_position()
{
	if (state == KNOCKBACK)
	{
		if (tweenX.progress() < 1.0f && tweenY.progress() < 1.0f)
		{
			sprite.setPosition({ tweenX.step(1) + static_cast<float>(texture.getSize().x / data->frameCount / 2), tweenY.step(1) });
		}
	}
	else
	{
		sprite.setPosition({ position.x + static_cast<float>(texture.getSize().x / data->frameCount / 2), position.y - static_cast<float>(currentLayer) });
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

void BattleEnemy::update_sprite()
{
	//std::cout << "[Current State: " << state << "]\n";

	if (currentFrameCooldown >= timeUntilNextFrame || state == KNOCKBACK) currentFrameCooldown = 0.0f; //the knockback animation will bypass the frame cooldown
	else return;

	//Updating to the corresponding frame
	switch (state)
	{
	case IDLE:
		//if (startedAttackAnimation = true)
		currentFrameIndex = 0;
		break;
	case WALK:
		if (currentFrameIndex < data->knockbackFrameIndex - 1) currentFrameIndex++;
		else currentFrameIndex = 0; //Reset to the first frame
		break;
	case ATTACK:
		if (currentFrameIndex <= data->knockbackFrameIndex) currentFrameIndex = data->knockbackFrameIndex;
		if (currentFrameIndex * sprite.getTextureRect().size.x < texture.getSize().x - sprite.getTextureRect().size.x) currentFrameIndex++;
		else currentFrameIndex = 0;//Reset to the idle frame
		break;
	case KNOCKBACK:
		currentFrameIndex = data->knockbackFrameIndex;
		break;
	default:
		currentFrameIndex = 0;
		break;
	}

	//std::cout << "[Current Frame Index: " << currentFrameIndex << "]\n\n\n";

	sprite.setTextureRect({ {static_cast<int>(texture.getSize().x / data->frameCount * currentFrameIndex), 0},
		{static_cast<int>(texture.getSize().x / data->frameCount), static_cast<int>(texture.getSize().y)}
		});
}
