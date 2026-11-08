#pragma once

/*
 * EntityType
 * ----------
 * Represents an entity type.
 * Types are used as targets for abilities. (for example, a fish could deal more damage to a specific enemy type)
 * In the JSON, types are written as numbers: SHELLED = 0, CRUSTACEAN = 1, FISH = 2.
 *
 * FISH is hidden: every unit gets it automatically (units.json doesn't list types), and it's the
 * default target of enemy abilities, so an enemy ability without "targetTypes" affects every fish.
 */

enum class EntityType
{
	NONE = -1,
	SHELLED,
	CRUSTACEAN,
	FISH,       // Hidden: every unit has it, enemies can't

	COUNT // Not a type: keep it last, the loader uses it to reject unknown type numbers
};

// Display name of a type (used by the debug tools)
inline const char* to_string(EntityType type)
{
	switch (type)
	{
	case EntityType::SHELLED:    return "SHELLED";
	case EntityType::CRUSTACEAN: return "CRUSTACEAN";
	case EntityType::FISH:       return "FISH";
	default:                     return "NONE";
	}
}
