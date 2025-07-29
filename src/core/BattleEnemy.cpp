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
	position = { 0.0f, 360.0f };
	currentAttackCooldown = data->attackFrequency; //Set current attack cooldown to attack frequency to make them attack instantly
	currentKnockbackCooldown = 0.f;

	//Init battle zones
	hitbox.size = { 200.0f, 720.0f };
	attackRangeZone.size = { data->attackRange , 720.0f };
	damageZone.size = { data->attackRange , 720.0f };

	//Init position
	hitbox.position = position;
	attackRangeZone.position = position;
	damageZone.position = position;

	//Init sprite
	bool isTextureLoaded = texture.loadFromFile(data->texture);
	sprite.setTexture(texture, true);

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

void BattleEnemy::update(float deltaTime)
{
	//Simple state machine
	if (state == IDLE)
	{
		velocity = { 0.0f, 0.0f };
		if (currentHealth < 0.0f) state = KNOCKBACK;
		if (currentHealth <= healthLeftBeforeNextKnockback) state = KNOCKBACK;
		//Todo: add knockback
		if (targets.empty()) state = WALK;

		if (currentAttackCooldown >= data->attackFrequency)
		{
			state = ATTACK;
		}
	}
	if (state == WALK)
	{
		velocity = { data->movementSpeed * 10.0f * deltaTime, 0.0f }; //*10.0f to make them more speedy
		position += velocity;
		state = IDLE;
	}
	if (state == ATTACK)
	{
		if (targets.empty())
		{
			state = IDLE; //No targets to attack
			goto attackend;
		}

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
		velocity = { 200.0f * deltaTime, 0.0f };
		position -= velocity;

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

void BattleEnemy::update_position()
{
	sprite.setPosition(position);

	hitbox.position = position;
	attackRangeZone.position = position;
	damageZone.position = position;

#ifdef DEBUG_MODE
	//Update debug rectangles' position
	rHitbox.setPosition(position);
	rAttackRangeZone.setPosition(position);
	rDamageZone.setPosition(position);
#endif
}
