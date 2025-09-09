#pragma once

#include <unordered_map>
#include <unordered_set>
#include <array>

class PlayerData
{
public:

	//Units
	void unlock_unit(int unitUid);
	void upgrade_unit(int unitUid);
	void equip_unit(int unitUid, int slot); //slot: 0-9
	void unequip_unit(int slot); //slot: 0-9

	bool is_unit_owned(int unitUid) const;
	int get_unit_level(int unitUid) const;

	//Shells
	void gain_shells(int amount);
	bool spend_shells(int amount); //returns false if not enough shells

	//Progression
	void complete_stage(int stageUid);

public:
	//Money
	int shells = 0;

	//Units
	std::unordered_map<int, int> ownedUnits; // first: unit uid, second: level
	std::array<int, 10> equippedUnits; // max 10 units can be equipped at once, -1 means empty slot. It stores the unit uid.
	
	//Progression
	std::unordered_set<int> completedStages; //Stage uids of completed stages
};