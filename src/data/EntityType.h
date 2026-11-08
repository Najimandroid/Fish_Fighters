#pragma once

/*
 * EntityType
 * ----------
 * Represents an entity type.
 * Types are used as targets for abilities. (for example, a fish could deal more damage to a specific enemy type)
 * In the JSON, types are written as numbers: SHELLED = 0, CRUSTACEAN = 1.
 */

enum class EntityType
{
	NONE = -1,
	SHELLED,
	CRUSTACEAN,

	COUNT // Not a type: keep it last, the loader uses it to reject unknown type numbers
};

// Display name of a type (used by the debug tools)
inline const char* to_string(EntityType type)
{
	switch (type)
	{
	case EntityType::SHELLED:    return "SHELLED";
	case EntityType::CRUSTACEAN: return "CRUSTACEAN";
	default:                     return "NONE";
	}
}
