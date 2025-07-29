#include "BattleUnit.h"
#include "../data/UnitData.h"

#include <iostream>

//BattleEnemy::BattleEnemy() = default;

BattleUnit::BattleUnit(std::shared_ptr<UnitData> data_) :
	data(data_)
{
	//Init core datas
	//magnification = magnification_;
	currentHealth = data->health; //* magnification.x;
	healthLeftBeforeNextKnockback = data->health - (data->health / data->knockbackCount);

	state = IDLE;
	position = { 1080.0f, 720.f * 2/3 };

	currentAttackCooldown = data->attackFrequency; //Set current attack cooldown to attack frequency to make them attack instantly

	currentKnockbackCooldown = 0.f;

	//Init sprite
	bool isTextureLoaded = texture.loadFromFile(data->texture);
	sprite.setTexture(texture, true);
	sprite.setOrigin({ 0.f, static_cast<float>(texture.getSize().y) });

	//Init battle zones
	hitbox.size = { static_cast<float>(texture.getSize().x), 720.0f };
	attackRangeZone.size = { data->attackRange + static_cast<float>(texture.getSize().x) , 720.0f };
	damageZone.size = { data->attackRange + static_cast<float>(texture.getSize().x) , 720.0f };

	//Init position
	hitbox.position = position;
	attackRangeZone.position = {position.x - data->attackRange + static_cast<float>(texture.getSize().x), position.y};
	damageZone.position = { position.x - data->attackRange + static_cast<float>(texture.getSize().x), position.y };;

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

void BattleUnit::update(float deltaTime, const std::map<int, std::shared_ptr<BattleEntity>>& entityList)
{
	//Simple state machine
	if (state == IDLE)
	{
		velocity = { 0.0f, 0.0f };
		if (currentHealth < 0.0f) { state = KNOCKBACK; goto endidle; }
		if (currentHealth <= healthLeftBeforeNextKnockback) { state = KNOCKBACK; goto endidle; }

		//Check the attack range
		for (auto& pair : entityList)
		{
			auto unit = pair.second;
			bool isUnitInAttackRange = attackRangeZone.findIntersection(unit->hitbox).has_value();
			if (isUnitInAttackRange)
			{
				isEntityOnRange = true;
				break;
			}
		}

		if (isEntityOnRange == false && targets.empty()) state = WALK;

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
		position -= velocity;
		if (currentHealth < 0.0f) state = KNOCKBACK;
		else state = IDLE;
	}
	if (state == ATTACK)
	{
		//Get targets in damage zone
		for (auto& pair : entityList)
		{
			auto enemy = pair.second;
			bool isUnitInAttackRange = damageZone.findIntersection(enemy->hitbox).has_value();
			if (isUnitInAttackRange && enemy->state != enemy->KNOCKBACK)
			{
				//std::cout << "ENEMY FOUND >:(\n";
				targets.insert(enemy);
			}
		}

		if (targets.empty())
		{
			state = IDLE; //No targets to attack
			goto endattack;
		}

		//std::cout << "Nom!\n";

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
		currentAttackCooldown = 0.0f;
		state = IDLE;
	}
	if (state == KNOCKBACK)
	{
		velocity = { 200.0f * deltaTime, 0.0f };
		position += velocity;

		currentKnockbackCooldown += deltaTime;

		if (currentKnockbackCooldown >= 0.5f) //Knockback lasts 0.5 seconds
		{
			if (currentHealth < 0.0f)
			{
				state = DEAD;
			}
			else
			{
				state = IDLE; //After knockback, the unit dies
				currentKnockbackCooldown = 0.0f; //Reset cooldown
				healthLeftBeforeNextKnockback -= (data->health / data->knockbackCount); //Update healthLeftBeforeNextKnockback
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

void BattleUnit::update_position()
{
	sprite.setPosition(position);

	hitbox.position = position;
	attackRangeZone.position = { position.x - data->attackRange, position.y };
	damageZone.position = { position.x - data->attackRange, position.y };

#ifdef DEBUG_MODE
	//Update debug rectangles' position
	rHitbox.setPosition(hitbox.position);
	rAttackRangeZone.setPosition(attackRangeZone.position);
	rDamageZone.setPosition(damageZone.position);
#endif
}
