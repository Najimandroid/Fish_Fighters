#include "BattleBase.h"

#include <iostream>

BattleBase::BattleBase(float health_, std::string texture_)
{
	//Init core datas
	maxHealth = health_;
	currentHealth = maxHealth;

	state = State::IDLE;

	position = { 0.0f, 360.0f };

	currentLayer = 51;


	//Init battle zones
	attackRangeZone.size = { 0.0f , 720.0f };
	damageZone.size = { 0.0f  , 720.0f };

	//Init position
	hitbox.position = position;
	attackRangeZone.position = position;
	damageZone.position = position;

	//Init sprite
	bool isTextureLoaded = texture.loadFromFile(texture_);
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

void BattleBase::update(float deltaTime)
{
	if (currentHealth < 0.0f)
	{
		//std::cout << "Base destroyed x_x\n";
	}

#ifdef DEBUG_MODE
	//std::cout << "[Fish Base Health] = " << currentHealth << "\n";
#endif

	update_position();
}

void BattleBase::update_position()
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
