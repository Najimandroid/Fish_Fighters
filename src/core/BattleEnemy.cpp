#include "BattleEnemy.h"
#include "../data/EnemyData.h"

//BattleEnemy::BattleEnemy() = default;

BattleEnemy::BattleEnemy(std::shared_ptr<EnemyData> _data):
	data(_data)
{
	//Init core datas
	currentHealth = data->health;
	state = State::IDLE;
	position = { 0.0f, 360.0f };


	//Init battle zones
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

void BattleEnemy::updatePosition()
{
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
