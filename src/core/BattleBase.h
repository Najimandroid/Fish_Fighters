#pragma once

#include <SFML/System/Vector2.hpp>

#include "BattleEntity.h"
#include "../data/EnemyData.h"

#include <memory>
#include <vector>

//class EnemyData;
//class BattleUnit;

/*
* The BattleBase represents a base in a stage
*/

class BattleBase : public BattleEntity
{
public:

	BattleBase(float health_, std::string texture_);

	//Overrides
	virtual void update(float deltaTime, const std::map<int, std::shared_ptr<BattleEntity>>& entityList) override;

public:

	float maxHealth = 1.0f;

private:
	//Private overrides
	virtual void update_position() override;
};