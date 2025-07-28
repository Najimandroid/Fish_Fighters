#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

#include "BattleEntity.h"

#include <memory>
#include <vector>

class UnitData;
class BattleEnemy;

using WeakBattleEnemyPtr = std::weak_ptr<BattleEnemy>;

/*
* The BattleUnit represents the spawned unit in a stage
*/

class BattleUnit: public BattleEntity
{
public:

	//Overrides
	virtual void update(float deltaTime) override {};

	//Data
	std::shared_ptr<const UnitData> data;

	//Targets
	std::vector<WeakBattleEnemyPtr> targets;

private:
	//Private overrides
	virtual void update_position() override {};
};