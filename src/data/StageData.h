#pragma once

#include <vector>
#include <memory>
#include <string>

#include "RewardData.h"

class EnemyStageData;

struct StageData
{
	int UID = -1;

	std::string name = "Unknown Stage";

	int enemiesLimit = 10;
	int unitsLimit = 10;

	float baseHealth = 100.0f;

	std::vector<std::shared_ptr<EnemyStageData>> enemies;

	std::string baseTexture;
	std::string backgroundTexture;

	std::vector<std::shared_ptr<RewardData>> rewards;
};