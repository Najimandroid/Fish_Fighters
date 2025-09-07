#include "PlayerData.h"

void PlayerData::unlock_unit(int unitUid)
{
	ownedUnits[unitUid] = 1;
}

void PlayerData::upgrade_unit(int unitUid)
{
	auto it = ownedUnits.find(unitUid);

	if (it != ownedUnits.end())
	{
		it->second += 1;
	}
}

void PlayerData::equip_unit(int unitUid, int slot)
{
	if (slot < 0 || slot > 9) return;

	auto it = ownedUnits.find(unitUid);
	if (it != ownedUnits.end())
	{
		equipedUnits[slot] = unitUid;
	}
}

void PlayerData::unequip_unit(int slot)
{
	if (slot < 0 || slot > 9) return;

	equipedUnits[slot] = -1;
}

void PlayerData::gain_shells(int amount)
{
	shells += amount;
}

bool PlayerData::spend_shells(int amount)
{
	if (shells >= amount)
	{
		shells -= amount;
		return true;
	}

	return false;
}

void PlayerData::complete_stage(int stageUid)
{
	completedStages.insert(stageUid);
}