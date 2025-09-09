#pragma once
#include <string>

struct RewardData
{
	std::string type; //"shells", "units"
	int amount = 0; //used for quantifiable items/money
	int unitUID = -1; //if we unlocked a new unit
};