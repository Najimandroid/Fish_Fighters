#pragma once

#include <SFML/System/Vector2.hpp>

#include "BattleEntity.h"
#include "../data/EnemyData.h"

#include <memory>
#include <vector>

//class EnemyData;
class BattleUnit;

using WeakBattleUnitPtr = std::weak_ptr<BattleUnit>;

/*
* The BattleUnit represents the spawned unit in a stage
*/

class BattleEnemy: public BattleEntity
{
public:

	BattleEnemy(std::shared_ptr<EnemyData> _data);

	//Overrides
	virtual void updatePosition() override;

	//Data
	std::shared_ptr<EnemyData> data = nullptr;

	//Targets
	std::vector<WeakBattleUnitPtr> targets;
};