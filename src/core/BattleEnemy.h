#pragma once

#include <SFML/System/Vector2.hpp>

#include "BattleEntity.h"
#include "../data/EnemyData.h"

#include <memory>
#include <vector>

//class EnemyData;
//class BattleUnit;

/*
* The BattleEnemy represents the spawned enemy in a stage
*/

class BattleEnemy: public BattleEntity
{
public:

	BattleEnemy(std::shared_ptr<EnemyData> data_, sf::Vector2f magnification_);

	//Overrides
	virtual void update(float deltaTime, const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList) override;
	virtual void update_position() override;

	//Data
	std::shared_ptr<EnemyData> data = nullptr;
};