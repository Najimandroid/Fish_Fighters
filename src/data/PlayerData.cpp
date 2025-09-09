#include "PlayerData.h"

#include "../core/DataLoader.h"

#include <iostream>

void PlayerData::unlock_unit(int unitUid)
{
	if (unitsWaitingToBeUnlocked.find(unitUid) != unitsWaitingToBeUnlocked.end())
	{
		unitsWaitingToBeUnlocked.erase(unitUid);
		ownedUnits[unitUid] = 1;
	}
}

void PlayerData::wait_to_be_unlocked(int unitUid)
{
	unitsWaitingToBeUnlocked.insert(unitUid);
}

void PlayerData::upgrade_unit(int unitUid)
{
	auto it = ownedUnits.find(unitUid);

	if (it != ownedUnits.end())
		it->second += 1;
}

void PlayerData::equip_unit(int unitUid, int slot)
{
	if (slot < 0 || slot > 9) return;

	auto it = ownedUnits.find(unitUid);
	if (it != ownedUnits.end())
		equippedUnits[slot] = unitUid;
}

void PlayerData::unequip_unit(int slot)
{
	if (slot < 0 || slot > 9) return;

	equippedUnits[slot] = -1;
}

bool PlayerData::is_unit_owned(int unitUid) const
{
	return ownedUnits.find(unitUid) != ownedUnits.end();
}

int PlayerData::get_unit_level(int unitUid) const
{
	auto it = ownedUnits.find(unitUid);

	if (it != ownedUnits.end())
	{
		return it->second;
	}

	return 0;
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

void PlayerData::complete_stage(std::weak_ptr<StageData> stageData)
{
	if (auto stage = stageData.lock())
	{
		if (completedStages.find(stage->UID) != completedStages.end())
		{
			//Stage already completed, only giving some shells for completing the level

			for (const auto& reward : stage->rewards)
			{
				if (reward->type == "shells")
				{
					gain_shells(reward->amount);
					break;
				}
			}
			return;
		}

		completedStages.insert(stage->UID);

		for (const auto& reward : stage->rewards)
		{
			if (reward->type == "shells")
			{
				gain_shells(reward->amount);
			}
			else if (reward->type == "unit" && reward->unitUID > 0)
			{
				if (!is_unit_owned(reward->unitUID))
				{
					wait_to_be_unlocked(reward->unitUID);
				}
			}
		}
	}
	else
	{
		std::cerr << "Issue while completing the stage." << std::endl;
	}
}