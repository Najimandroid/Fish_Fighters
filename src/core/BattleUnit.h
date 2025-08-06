#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

#include "BattleEntity.h"
#include "../data/UnitData.h"

#include <memory>
#include <vector>

//class UnitData;
//class BattleEnemy;

/*
* The BattleUnit represents the spawned unit in a stage
*/

class BattleUnit: public BattleEntity
{
public:

	BattleUnit(std::shared_ptr<UnitData> data_);

	//Overrides
	virtual void update(float deltaTime, const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList) override;
	virtual void update_position() override;
	virtual void update_sprite() override;

	//Data
	std::shared_ptr<UnitData> data = nullptr;
};