#include "BattleEnemy.h"

#include <iostream>

//BattleEnemy::BattleEnemy() = default;

BattleEnemy::BattleEnemy(std::shared_ptr<EntityData> data_, sf::Vector2f magnification_)
{
	//Init data
	data = data_;

	//Init core datas
	magnification = magnification_;

	currentHealth = data->health * magnification.x;
	healthLeftBeforeNextKnockback = data->knockbackCount > 0 ? data->health - (data->health / data->knockbackCount) : 0.f;

	position = { 0.0f, 1080.0f * 2 / 3 };

	currentAttackCooldown = data->attackFrequency; //Set current attack cooldown to attack frequency to make them attack instantly

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
	hitbox.size = { static_cast<float>(texture.getSize().x / data->frameCount / 2), 1080.0f };
	attackRangeZone.size = { data->attackRange + static_cast<float>(texture.getSize().x / data->frameCount / 2) , 1080.0f };
	damageZone.size = { data->attackRange + static_cast<float>(texture.getSize().x / data->frameCount / 2) , 1080.0f };

	//std::cout << "hitbox size x: " << static_cast<float>(texture.getSize().x / data->frameCount / 2) << '\n';

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

BattleEnemy::~BattleEnemy()
{
	//std::cout << "BattleEnemy Destructor called\n";
}

void BattleEnemy::update(float deltaTime, const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList)
{
	stateMachine->update_state(deltaTime);

	update_position();
	update_sprite();

	currentAttackCooldown += deltaTime;
	currentFrameCooldown += deltaTime;
}

void BattleEnemy::update_position()
{
	if (stateMachine->get_active_state_id() == "KNOCKBACK")
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

