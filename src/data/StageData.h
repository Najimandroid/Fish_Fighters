#pragma once

#include <vector>
#include <memory>
#include <string>

class EnemyStageData;

struct StageData
{
	int UID = -1;

	std::string name = "Unknown Stage";

	int enemiesLimit = 10;
	int unitsLimit = 10;

	std::vector<std::shared_ptr<EnemyStageData>> enemies;

	std::string baseTexture;
	std::string backgroundTexture;
};