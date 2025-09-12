#pragma once
#include <string>

/*
 * RewardData
 * ----------
 * Represents a single reward granted when the player completes a stage.
 * A stage can have multiple rewards (e.g., shells + a new unit).
 */

struct RewardData
{
    std::string type;   // The type of reward:
    // - "shells" => currency
    // - "unit"   => unlocks a new unit

    int amount = 0;     // Used for quantifiable rewards (e.g., shells gained)

    int unitUID = -1;   // UID of the unit to unlock (only relevant if type == "unit")
};
